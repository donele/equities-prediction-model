#include <HLearn/HSignalNewsBin.h>
#include <HLearnObj/Signal.h>
#include <HLearnObj/Sample.h>
#include "optionlibs/TickData.h"
#include <HLib.h>
#include <jl_lib.h>
#include "TFile.h"
#include <algorithm>
#include <functional>
#include <vector>
#include <fstream>
using namespace std;

HSignalNewsBin::HSignalNewsBin(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
nInput_(0),
minMsecs_(0),
maxMsecs_(24*60*60*1000),
write_signal_(true),
market_(""),
minNews_(0),
maxNews_(100),
newsCutName_("sent1"),
reqNewsIndx_(-1),
mrTarget_("resid"),
input_dir_("")
{
	if( conf.count("writeSignal") )
		write_signal_ = conf.find("writeSignal")->second == "true";
	if( conf.count("inputDir") )
		input_dir_ = conf.find("inputDir")->second;
	if( conf.count("newsDir") )
		news_dir_ = conf.find("newsDir")->second;
	if( conf.count("predDir") )
		pred_dir_ = conf.find("predDir")->second;
	if( conf.count("minMsecs") )
		minMsecs_ = atoi( conf.find("minMsecs")->second.c_str() );
	if( conf.count("maxMsecs") )
		maxMsecs_ = atoi( conf.find("maxMsecs")->second.c_str() );
	if( conf.count("minNews") )
		minNews_ = atoi( conf.find("minNews")->second.c_str() );
	if( conf.count("maxNews") )
		maxNews_ = atoi( conf.find("maxNews")->second.c_str() );
	if( conf.count("newsCutName") )
		newsCutName_ = conf.find("newsCutName")->second;
	if( conf.count("outputDir") )
		output_dir_ = conf.find("outputDir")->second;
	if( conf.count("market") )
		market_ = conf.find("market")->second;

	// Input Names.
	if( conf.count("input") )
	{
		pair<mmit, mmit> inputs = conf.equal_range("input");
		for( mmit mi = inputs.first; mi != inputs.second; ++mi )
		{
			string name = mi->second;
			inputNames_.push_back(name);
		}
	}
	nInput_ = inputNames_.size();
	signalNames_ = inputNames_;
	signalNames_.push_back("target");
	signalNames_.push_back("tbpred");
	signalNames_.push_back("resid");

	vector<string> newsNames = news_.get_names();
	for( vector<string>::iterator it = newsNames.begin(); it != newsNames.end(); ++it )
		signalNames_.push_back(*it);

	mr_labels_ = signalNames_;
	mr_labels_.push_back("mrTaret");

	int i = 0;
	for( vector<string>::iterator it = signalNames_.begin(); it != signalNames_.end(); ++it, ++i )
	{
		string name = *it;
		if( name == newsCutName_ )
		{
			reqNewsIndx_ = i;
			break;
		}
	}

	if( HEnv::Instance()->multiThreadTicker() )
	{
		cerr << "HSignalNewsBin:Error multiThreadTicker is not supported." << endl;
		exit(5);
	}
}

HSignalNewsBin::~HSignalNewsBin()
{}

void HSignalNewsBin::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	TFile* f = HEnv::Instance()->outfile();
	f->cd();
	f->mkdir(moduleName_.c_str());

	// h_signal_all_
	h_signal_all_ = vector<TH1F*>(nInput_);
	for( int i=0; i<nInput_; ++i )
	{
		char name[40];
		sprintf(name, "h1_%s", signalNames_[i].c_str());
		char title[100];
		sprintf(title, "Signal %s", signalNames_[i].c_str());

		h_signal_all_[i] = new TH1F(name, title, 200, 0, 1e-5);
		h_signal_all_[i]->SetBit(TH1::kCanRebin);
	}

	GTH::Instance()->init(market_);

	return;
}

void HSignalNewsBin::beginMarket()
{
	// h_signal_
	h_signal_ = vector<TH1F*>(nInput_);
	for( int i=0; i<nInput_; ++i )
	{
		char name[40];
		sprintf(name, "h1_%s_%s", market_.c_str(), signalNames_[i].c_str());
		char title[100];
		sprintf(title, "Signal %s %s", market_.c_str(), signalNames_[i].c_str());

		h_signal_[i] = new TH1F(name, title, 200, 0, 1e-5);
		h_signal_[i]->SetBit(TH1::kCanRebin);
	}

	// MultiRegress
	mr_.Reset(mr_labels_);
	mr_ = MultiRegress(mr_labels_);

	return;
}

void HSignalNewsBin::beginDay()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	sessions_ = mto::sessions(market_, 0, 0, 0, idate);
	msecOpen_ = mto::msecOpen(market_, idate);
	msecClose_ = mto::msecClose(market_, idate);

	//vector<string> tickers;
	//get_tickers(tickers, idate);
	set_ticker_list(market_, idate);
	read_pred(market_, idate);
	vector<string> tickers = HEnv::Instance()->tickers();

	string filename = get_sigName(market_, idate);
	ifs_.open(filename.c_str(), ios::in | ios::binary);

	if( ifs_.is_open() )
	{
		ic_ = InputCounter("#tickers:", 10);

		hnn::BinSigHeader bsh;
		ifs_ >> bsh;
		ncols_ = bsh.ncols;
		inputIndx_.clear();

		for( vector<string>::iterator it = inputNames_.begin(); it != inputNames_.end(); ++it )
		{
			string inputName = *it;
			int indx = -1;
			vector<string>::iterator it2 = find(bsh.labels.begin(), bsh.labels.end(), inputName);
			if( it2 != bsh.labels.end() )
				indx = distance(bsh.labels.begin(), it2);
			inputIndx_.push_back(indx);
		}

		news_.read_news(market_, idate, news_dir_);

		if( write_signal_ )
		{
			// Open file to write the signal.
			char filename[400];
			sprintf(filename, "%s\\%s\\%d.bin", output_dir_.c_str(), market_.c_str(), idate);
			ofs_.open( filename, ios::out | ios::binary );

			// Write SignalSummary.
			hnn::SignalSummary ss(signalNames_, tickers);
			ofs_ << ss;
		}
	}
	//string predFile = get_predFile(idate);
	//string sigFile = get_sigFile(idate);
	//ifstream ifsPred(predFile.c_str());
	//ifstream ifsSig(sigFile.c_str());
	//if( ifsPred.is_open() && ifsSig.is_open() )
	//{
	//	// Read news binary
	//	news_.read_news(market_, idate);

	//	if( write_signal_ )
	//	{
	//		// Open file to write the signal.
	//		char filename[400];
	//		sprintf(filename, "%s\\%s\\%d.bin", output_dir_.c_str(), market_.c_str(), idate);
	//		ofs_.open( filename, ios::out | ios::binary );

	//		// Write SignalSummary.
	//		hnn::SignalSummary ss(signalNames_, tickers);
	//		ofs_ << ss;
	//	}

	//	ic_ = InputCounter("#tickers:", 10);

	//	string linePred;
	//	string lineSig;
	//	string ticker = "";
	//	string lastTicker = "";
	//	vector<hnn::Signal> vSignal;
	//	while( getline(ifsPred, linePred) && getline(ifsSig, lineSig) )
	//	{
	//		// Input names and indexes.
	//		vector<string> v = split(lineSig, '\t');
	//		if( v[0] == "uid" )
	//			find_index(v);
	//		else
	//		{
	//			// Pred file.
	//			vector<string> vPred = split(linePred, '\t');
	//			float target = atof(vPred[4].c_str());
	//			float tbpred = atof(vPred[6].c_str());
	//			float resid = target - tbpred;

	//			// Sig file.
	//			ticker = get_ticker(v);
	//			if( ticker != lastTicker && !lastTicker.empty() )
	//				write_signal(vSignal, lastTicker);
	//			lastTicker = ticker;

	//			// news.
	//			int msecs = atoi(v[2].c_str()) + msecOpen_;
	//			vector<float> newsSignals = news_.get_signals(ticker, msecs);

	//			// Add.
	//			add_signal(v, vSignal, ticker, target, tbpred, resid, newsSignals);
	//		}
	//	}
	//	write_signal(vSignal, ticker);
	//}

	return;
}

void HSignalNewsBin::beginTicker(const string& ticker)
{
	//string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	//read_sample(ticker, idate);

	string news_ticker = GTH::Instance()->get(market_)->UniqueToTicker(ticker, idate);

	map<string, int>& mTickerCnt = mDateTickerCnt_[idate];
	vector<Pred>& pred = mTickerPred_[ticker];
	if( mTickerCnt.count(ticker) )
	{
		vector<hnn::Signal> vSignal;
		int nentries = mTickerCnt[ticker];
		for( int i=0; i<nentries; ++i )
		{
			hnn::BinSig aSignal(ncols_);
			ifs_ >> aSignal;

			// inputs
			vector<float> vi(nInput_);
			for( int j=0; j<nInput_; ++j )
			{
				if( inputIndx_[j] >= 0 )
					vi[j] = aSignal.input[inputIndx_[j]];
			}

			vi.push_back(pred[i].target);
			vi.push_back(pred[i].tbpred);
			vi.push_back(pred[i].resid);

			// news.
			int msecs = aSignal.input[0] + msecOpen_;
			vector<float> newsSignals = news_.get_signals(news_ticker, msecs);

			for( vector<float>::iterator it = newsSignals.begin(); it != newsSignals.end(); ++it )
				vi.push_back(*it);

			vSignal.push_back(hnn::Signal(vi));

			// mr_
			vector<double> dataPt = vector<double>(vi.begin(), vi.end());
			if( mrTarget_ == "target" )
				dataPt.push_back(pred[i].target);
			else if( mrTarget_ == "resid" )
				dataPt.push_back(pred[i].resid);
			mr_.AddPoint(&dataPt[0]);
		}
		write_signal(vSignal, ticker);
	}
	else
	{
		cerr << "HBinSigRead::read_sample read '" << ticker << "' is not found" << endl;
		exit(5);
	}
	return;
}

void HSignalNewsBin::endTicker(const string& ticker)
{
	HEvent::Instance()->remove<vector<hnn::Sample> >(ticker, "sample");
	//if( threadSampleRead_ )
	//{
	//	boost::mutex::scoped_lock lock(private_mutex_);
	//	sampleReadLog_.erase(ticker);
	//}
	return;
}

void HSignalNewsBin::endDay()
{
	ifs_.close();
	ifs_.clear();
	if( write_signal_ )
	{
		ofs_.close();
		ofs_.clear();
	}
	return;
}

void HSignalNewsBin::endMarket()
{
	// Write.
	TFile* f = HEnv::Instance()->outfile();
	f->cd();
	f->cd(moduleName_.c_str());
	gDirectory->mkdir(market_.c_str());
	gDirectory->cd(market_.c_str());
	for( int i=0; i<nInput_; ++i )
	{
		h_signal_[i]->Write();
	}

	// Delete.
	for( int i=0; i<nInput_; ++i )
	{
		TH1F* h1 = h_signal_[i];
		delete h1;
	}
	print_mr(mr_, market_);
	return;
}

void HSignalNewsBin::endJob()
{
	// Write.
	TFile* f = HEnv::Instance()->outfile();

	{
		f->cd();
		f->cd(moduleName_.c_str());
		gDirectory->mkdir("all");
		gDirectory->cd("all");
		for( int i=0; i<nInput_; ++i )
		{
			h_signal_all_[i]->Write();
		}
	}

	// Delete.
	for( int i=0; i<nInput_; ++i )
	{
		TH1F* h1 = h_signal_all_[i];
		delete h1;
	}
	return;
}

string HSignalNewsBin::get_sigName(string market, int idate)
{
	string piece = "";
	if( market == "US" )
		piece = "U";
	else if( market == "AS" )
		piece = "S";

	char filename[400];
	sprintf(filename, "%s\\sig%d%stm.bin", input_dir_.c_str(), idate, piece.c_str());
	return filename;
}

string HSignalNewsBin::get_sigTxtName(string market, int idate)
{
	string piece = "";
	if( market == "US" )
		piece = "U";
	else if( market == "AS" )
		piece = "S";

	char filename[400];
	sprintf(filename, "%s\\sig%d%stm.txt", input_dir_.c_str(), idate, piece.c_str());
	return filename;
}

string HSignalNewsBin::get_predName(string market, int idate)
{
	char filename[400];
	if( market == "US" )
	{
		int ntree = (idate <= 20090811)? 13: 20;
		sprintf(filename, "%s\\%dPred%dU.txt", pred_dir_.c_str(), ntree, idate);
	}
	else if( market == "AS" )
		sprintf(filename, "%s\\9Pred%dS.txt", pred_dir_.c_str(), idate);

	return filename;
}

//string HSignalNewsBin::get_ticker(vector<string>& v)
//{
//	string ret = v[1];
//	if( "EU" == market_ )
//		ret = v[0].substr(1, 1) + ":" + ret;
//	return ret;
//}

void HSignalNewsBin::get_quotes(const TickSeries<QuoteInfo>* ptsQ, vector<QuoteInfo>& vq)
{
	int ntq = ptsQ->NTicks();
	vq.reserve(ntq);

	const_cast<TickSeries<QuoteInfo>*>(ptsQ)->StartRead();
	QuoteInfo quote;
	double last_mid = -1;
	for( int n=0; n<ntq; ++n )
	{
		const_cast<TickSeries<QuoteInfo>*>(ptsQ)->Read(&quote);
		int msecs = quote.msecs;
		bool inrange = find_if(sessions_.begin(), sessions_.end(), bind2nd(inRange(), msecs)) != sessions_.end();
		if(inrange)
		{
			double mid = (quote.ask + quote.bid) / 2.0;
			bool valid_mid = mid > 0 && (last_mid < 0 || mid/last_mid < 1.05);

			if( valid_mid )
			{
				double sprd = (quote.ask - quote.bid) / mid;
				if( sprd > 0 && sprd < 0.05 )
				{
					vq.push_back(quote);
					last_mid = mid;
				}
			}
		}
	}

	return;
}

//void HSignalNewsBin::read_sample(string ticker, int idate)
//{
//	//vector<hnn::Sample> sample;
//
//	string news_ticker = GTH::Instance()->get(market
//
//
//	map<string, int>& mTickerCnt = mDateTickerCnt_[idate];
//	vector<Pred>& pred = mTickerPred_[ticker];
//	if( mTickerCnt.count(ticker) )
//	{
//		vector<hnn::Signal> vSignal;
//		int nentries = mTickerCnt[ticker];
//		for( int i=0; i<nentries; ++i )
//		{
//			hnn::BinSig aSignal(ncols_);
//			ifs_ >> aSignal;
//
//			// inputs
//			vector<float> vi(nInput_);
//			for( int j=0; j<nInput_; ++j )
//			{
//				if( inputIndx_[j] >= 0 )
//					vi[j] = aSignal.input[inputIndx_[j]];
//			}
//
//			vi.push_back(pred[i].target);
//			vi.push_back(pred[i].tbpred);
//			vi.push_back(pred[i].resid);
//
//			// news.
//			int msecs = aSignal.input[0] + msecOpen_;
//			vector<float> newsSignals = news_.get_signals(ticker, msecs);
//
//			for( vector<float>::iterator it = newsSignals.begin(); it != newsSignals.end(); ++it )
//				vi.push_back(*it);
//
//			vSignal.push_back(hnn::Signal(vi));
//
//			// target
//			//float target = 0;
//			//if( targetName_ == "target" )
//			//	target = pred[i].target;
//			//else if( targetName_ == "resid" )
//			//	target = pred[i].resid;
//
//			//if( verbose_ > 3 )
//			//{
//			//	printf("%10.4f\t%10.4f\n", aSignal.input[22], pred[i].target);
//			//}
//
//			// spectator
//			//vector<float> vs(nSpectator_);
//			//int j = 0;
//			//for( vector<string>::iterator it = spectatorNames_.begin(); it != spectatorNames_.end(); ++it, ++j )
//			//{
//			//	string name = *it;
//			//	double val = 0;
//			//	if( name == "target" )
//			//		val = pred[i].target;
//			//	else if( name == "tbpred" )
//			//		val = pred[i].tbpred;
//			//	else if( name == "resid" )
//			//		val = pred[i].resid;
//			//	vs[j] = val;
//			//}
//
//			//sample.push_back(hnn::Sample(vi, target, vs));
//		}
//		write_signal(vSignal, ticker);
//	}
//	else
//	{
//		cerr << "HBinSigRead::read_sample read '" << ticker << "' is not found" << endl;
//		exit(5);
//	}
//	return;
//}


//void HSignalNewsBin::find_index(vector<string>& v)
//{
//	indx_.clear();
//	int N = v.size();
//	for( vector<string>::iterator it = inputNames_.begin(); it != inputNames_.end(); ++it )
//	{
//		string name = *it;
//		int indx = -1;
//		for( int i = 0; i < N; ++i )
//		{
//			string field = v[i];
//			if( name == field )
//			{
//				indx = i;
//				break;
//			}
//		}
//
//		if( indx < 0 )
//		{
//			cout << "input " << name << " is not found.\n";
//			exit(5);
//		}
//		else
//			indx_.push_back(indx);
//	}
//	return;
//}

void HSignalNewsBin::write_signal(vector<hnn::Signal>& vSignal, string ticker)
{
	if( !ticker.empty() )
	{
		hnn::SignalLabel sh(ticker, vSignal.size());
		ofs_ << sh;
		for( vector<hnn::Signal>::iterator it = vSignal.begin(); it != vSignal.end(); ++it )
			ofs_ << *it;

		for( vector<hnn::Signal>::iterator it = vSignal.begin(); it != vSignal.end(); ++it )
		{
			vector<float>& input = it->input;
			double target = 0;

			{
				boost::mutex::scoped_lock lock(HEnv::Instance()->root_mutex());
				for( int i=0; i<nInput_; ++i )
				{
					h_signal_[i]->Fill(input[i]);
					h_signal_all_[i]->Fill(input[i]);
				}
			}
		}
		++ic_;
	}
	vSignal.clear();

	return;
}

//void HSignalNewsBin::add_signal(vector<string>& v, vector<hnn::Signal>& vSignal, string ticker, float target, float tbpred, float resid,
//								 float sent5d, float sent20d, float cosent5d, float cosent20d, float autocosent4d, float autocosent19d)
//{
//	vector<float> input;
//	for( vector<int>::iterator it = inputIndx_.begin(); it != inputIndx_.end(); ++it )
//	{
//		double val = atof(v[*it].c_str());
//		input.push_back(val);
//	}
//	input.push_back(target);
//	input.push_back(tbpred);
//	input.push_back(resid);
//	input.push_back(sent5d);
//	input.push_back(sent20d);
//	input.push_back(cosent5d);
//	input.push_back(cosent20d);
//	input.push_back(autocosent4d);
//	input.push_back(autocosent19d);
//	vSignal.push_back(hnn::Signal(input));
//	return;
//}
//
//void HSignalNewsBin::add_signal(vector<string>& v, vector<hnn::Signal>& vSignal, string ticker, float target, float tbpred, float resid, vector<float>& newsSignals)
//{
//	vector<float> input;
//	for( vector<int>::iterator it = inputIndx_.begin(); it != inputIndx_.end(); ++it )
//	{
//		double val = atof(v[*it].c_str());
//		input.push_back(val);
//	}
//
//	input.push_back(target);
//	input.push_back(tbpred);
//	input.push_back(resid);
//
//	for( vector<float>::iterator it = newsSignals.begin(); it != newsSignals.end(); ++it )
//		input.push_back(*it);
//
//	bool passReqNews = minNews_ <= input[reqNewsIndx_] && input[reqNewsIndx_] <= maxNews_;
//	bool pass = passReqNews;
//	if( pass )
//	{
//		vSignal.push_back(hnn::Signal(input));
//
//		// mr_
//		vector<double> dataPt = vector<double>(input.begin(), input.end());
//		if( mrTarget_ == "target" )
//			dataPt.push_back(target);
//		else if( mrTarget_ == "resid" )
//			dataPt.push_back(resid);
//		mr_.AddPoint(&dataPt[0]);
//	}
//	return;
//}

//void HSignalNewsBin::get_tickers(vector<string>& tickers, int idate)
//{
//	string predFile = get_predFile(idate);
//	ifstream ifsPred(predFile.c_str());
//	if( ifsPred.is_open() )
//	{
//		string lastTicker = "";
//		string linePred;
//		while( getline(ifsPred, linePred) )
//		{
//			vector<string> v = split(linePred, '\t');
//			if( v[0] != "uid" )
//			{
//				//string ticker = v[1];
//				string ticker = get_ticker(v);
//				if( ticker != lastTicker )
//				{
//					tickers.push_back(ticker);
//					lastTicker = ticker;
//				}
//			}
//		}
//	}
//	return;
//}
//
void HSignalNewsBin::set_ticker_list(string market, int idate)
{
	if( mDateVTicker_.count(idate) )
	{
		vector<string>& vTicker = mDateVTicker_[idate];
		HEnv::Instance()->tickers(vTicker);
	}
	else
	{
		string filename = get_sigTxtName(market, idate);
		ifstream ifs(filename.c_str());
		vector<string> vTicker;
		map<string, int> mTickerCnt;

		int cnt = 0;
		string line = "";
		string last_uid = "";
		while( getline(ifs, line) )
		{
			vector<string> vs = split(line, '\t');
			if( vs.size() == 2 )
			{
				if( vs[0] != "uid" )
				{
					string uid = vs[0];
					if( uid != last_uid && !last_uid.empty() )
					{
						mTickerCnt.insert( make_pair(last_uid, cnt) );
						cnt = 0;
						vTicker.push_back(last_uid);
					}
					++cnt;
					last_uid = uid;
				}
			}
		}
		if( !last_uid.empty() )
		{
			mTickerCnt.insert( make_pair(last_uid, cnt) );
			cnt = 0;
			vTicker.push_back(last_uid);
		}
		mDateTickerCnt_[idate] = mTickerCnt;
		mDateVTicker_[idate] = vTicker;
		HEnv::Instance()->tickers(vTicker);
	}

	return;
}

void HSignalNewsBin::read_pred(string market, int idate)
{
	string filename = get_predName(market, idate);
	ifstream ifs(filename.c_str());
	string line;
	map<string, int>& mTickerCnt = mDateTickerCnt_[idate];
	mTickerPred_.clear();

	getline(ifs, line); // first line.

	vector<string> tickers = HEnv::Instance()->tickers();
	for( vector<string>::iterator it = tickers.begin(); it != tickers.end(); ++it )
	{
		string ticker = *it;
		int N = mTickerCnt[ticker];
		mTickerPred_[ticker] = vector<Pred>(N);
		vector<Pred>& v = mTickerPred_[ticker];
		for( int i=0; i<N; ++i )
		{
			getline(ifs, line);
			vector<string> vs = split(line, '\t');
			if( !vs.empty() )
			{
				float target = atof(vs[0].c_str());
				float tbpred = atof(vs[2].c_str());
				Pred pred(target, tbpred);
				v[i] = pred;
			}
		}
	}

	return;
}

//string HSignalNewsBin::get_predFile(int idate)
//{
//	string piece = "";
//	if( market_ == "CA" )
//		piece = "J";
//	else if( market_ == "EU" )
//		piece = "E";
//
//	char predFile[200];
//	sprintf(predFile, "%s\\pred%d%stm.txt", input_dir_.c_str(), idate, piece.c_str());
//	return predFile;
//}
//
//string HSignalNewsBin::get_sigFile(int idate)
//{
//	string piece = "";
//	if( market_ == "CA" )
//		piece = "J";
//	else if( market_ == "EU" )
//		piece = "E";
//
//	char sigFile[200];
//	sprintf(sigFile, "%s\\sigTB%d%stm.txt", input_dir_.c_str(), idate, piece.c_str());
//	return sigFile;
//}

void HSignalNewsBin::print_mr(MultiRegress& mr, string market, string flag)
{
	cout << "------------------------------------------------------------\n";
	cout << moduleName_ << " R2 summary " << market << " " << flag << "\n";
	cout << "------------------------------------------------------------\n";

	int ndim = mr.NDim();
	vector<double> coeffs = mr.ForecastCoeffs(ndim - 1);
	vector<double> sensi = mr.Sensitivities(ndim - 1);

	cout << "ndim: " << ndim << endl;
	cout << "coeffs: ";
	copy(coeffs.begin(), coeffs.end(), ostream_iterator<double>(cout, "\t"));
	cout << endl;

	cout << "sensis: ";
	copy(sensi.begin(), sensi.end(), ostream_iterator<double>(cout, "\t"));
	cout << "\n\n";

	cout << "variables:\n";
	cout << "\t";
	copy(mr_labels_.begin(), mr_labels_.end(), ostream_iterator<string>(cout, "\n\t"));
	cout << endl;

	double r2 = mr.R2(ndim - 1);
	cout << "nPoints: " << mr.NPoints() << endl;
	cout << "R^2: " << r2 << endl;

	for( int i=0; i<ndim; ++i )
	{
		cout << "\ndim: " << i << " " << mr_labels_[i] << endl;
		cout << "\tmean: " << mr.Mean(i) << endl;
		cout << "\tvariance: " << mr.Delta2(i) << endl;

		if( i < ndim - 1 )
		{
			vector<unsigned> inOut;
			for( int j=0; j<ndim; ++j )
				if( j != i )
					inOut.push_back(j);
			cout << "\tPartial_R^2: " << r2 - mr.R2(inOut) << endl;
		}
	}

	cout << "\n";
	for( int i=0; i<ndim; ++i )
	{
		cout << "corr_dim_" << i << ":";
		{
			for( int j=0; j<ndim; ++j )
			{
				char buf[100];
				sprintf( buf, "%9.3f", mr.Corr(i,j) );
				cout << buf;
			}
			cout << endl;
		}
	}
	cout << endl;

	return;
}
