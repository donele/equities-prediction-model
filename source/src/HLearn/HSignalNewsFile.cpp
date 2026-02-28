#include <HLearn/HSignalNewsFile.h>
#include <HLearnObj/Signal.h>
#include "optionlibs/TickData.h"
#include <HLib.h>
#include <jl_lib.h>
#include "TFile.h"
#include <algorithm>
#include <functional>
#include <vector>
#include <fstream>
using namespace std;

HSignalNewsFile::HSignalNewsFile(const string& moduleName, const multimap<string, string>& conf)
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
		cerr << "HSignalNewsFile:Error multiThreadTicker is not supported." << endl;
		exit(5);
	}
}

HSignalNewsFile::~HSignalNewsFile()
{}

void HSignalNewsFile::beginJob()
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


	return;
}

void HSignalNewsFile::beginMarket()
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

void HSignalNewsFile::beginDay()
{
	int idate = HEnv::Instance()->idate();
	sessions_ = mto::sessions(market_, 0, 0, 0, idate);
	msecOpen_ = mto::msecOpen(market_, idate);
	msecClose_ = mto::msecClose(market_, idate);

	vector<string> tickers;
	get_tickers(tickers, idate);

	string predFile = get_predFile(idate);
	string sigFile = get_sigFile(idate);
	ifstream ifsPred(predFile.c_str());
	ifstream ifsSig(sigFile.c_str());
	if( ifsPred.is_open() && ifsSig.is_open() )
	{
		// Read news binary
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

		ic_ = InputCounter("#tickers:", 10);

		string linePred;
		string lineSig;
		string ticker = "";
		string lastTicker = "";
		vector<hnn::Signal> vSignal;
		while( getline(ifsPred, linePred) && getline(ifsSig, lineSig) )
		{
			// Input names and indexes.
			vector<string> v = split(lineSig, '\t');
			if( v[0] == "uid" )
				find_index(v);
			else
			{
				// Pred file.
				vector<string> vPred = split(linePred, '\t');
				float target = atof(vPred[4].c_str());
				float tbpred = atof(vPred[6].c_str());
				float resid = target - tbpred;

				// Sig file.
				ticker = get_ticker(v);
				if( ticker != lastTicker && !lastTicker.empty() )
					write_signal(vSignal, lastTicker);
				lastTicker = ticker;

				// news.
				int msecs = atoi(v[2].c_str()) + msecOpen_;
				vector<float> newsSignals = news_.get_signals(ticker, msecs);

				// Add.
				add_signal(v, vSignal, ticker, target, tbpred, resid, newsSignals);
			}
		}
		write_signal(vSignal, ticker);
	}

	return;
}

void HSignalNewsFile::beginTicker(const string& ticker)
{
	return;
}

void HSignalNewsFile::endTicker(const string& ticker)
{
	return;
}

void HSignalNewsFile::endDay()
{
	if( write_signal_ )
	{
		ofs_.close();
		ofs_.clear();
	}
	return;
}

void HSignalNewsFile::endMarket()
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

void HSignalNewsFile::endJob()
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

string HSignalNewsFile::get_ticker(vector<string>& v)
{
	string ret = v[1];
	if( "EU" == market_ )
		ret = v[0].substr(1, 1) + ":" + ret;
	return ret;
}

void HSignalNewsFile::get_quotes(const TickSeries<QuoteInfo>* ptsQ, vector<QuoteInfo>& vq)
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


void HSignalNewsFile::find_index(vector<string>& v)
{
	indx_.clear();
	int N = v.size();
	for( vector<string>::iterator it = inputNames_.begin(); it != inputNames_.end(); ++it )
	{
		string name = *it;
		int indx = -1;
		for( int i = 0; i < N; ++i )
		{
			string field = v[i];
			if( name == field )
			{
				indx = i;
				break;
			}
		}

		if( indx < 0 )
		{
			cout << "input " << name << " is not found.\n";
			exit(5);
		}
		else
			indx_.push_back(indx);
	}
	return;
}

void HSignalNewsFile::write_signal(vector<hnn::Signal>& vSignal, string ticker)
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

void HSignalNewsFile::add_signal(vector<string>& v, vector<hnn::Signal>& vSignal, string ticker, float target, float tbpred, float resid,
								 float sent5d, float sent20d, float cosent5d, float cosent20d, float autocosent4d, float autocosent19d)
{
	vector<float> input;
	for( vector<int>::iterator it = indx_.begin(); it != indx_.end(); ++it )
	{
		double val = atof(v[*it].c_str());
		input.push_back(val);
	}
	input.push_back(target);
	input.push_back(tbpred);
	input.push_back(resid);
	input.push_back(sent5d);
	input.push_back(sent20d);
	input.push_back(cosent5d);
	input.push_back(cosent20d);
	input.push_back(autocosent4d);
	input.push_back(autocosent19d);
	vSignal.push_back(hnn::Signal(input));
	return;
}

void HSignalNewsFile::add_signal(vector<string>& v, vector<hnn::Signal>& vSignal, string ticker, float target, float tbpred, float resid, vector<float>& newsSignals)
{
	vector<float> input;
	for( vector<int>::iterator it = indx_.begin(); it != indx_.end(); ++it )
	{
		double val = atof(v[*it].c_str());
		input.push_back(val);
	}

	input.push_back(target);
	input.push_back(tbpred);
	input.push_back(resid);

	for( vector<float>::iterator it = newsSignals.begin(); it != newsSignals.end(); ++it )
		input.push_back(*it);

	bool passReqNews = minNews_ <= input[reqNewsIndx_] && input[reqNewsIndx_] <= maxNews_;
	bool pass = passReqNews;
	if( pass )
	{
		vSignal.push_back(hnn::Signal(input));

		// mr_
		vector<double> dataPt = vector<double>(input.begin(), input.end());
		if( mrTarget_ == "target" )
			dataPt.push_back(target);
		else if( mrTarget_ == "resid" )
			dataPt.push_back(resid);
		mr_.AddPoint(&dataPt[0]);
	}
	return;
}

void HSignalNewsFile::get_tickers(vector<string>& tickers, int idate)
{
	string predFile = get_predFile(idate);
	ifstream ifsPred(predFile.c_str());
	if( ifsPred.is_open() )
	{
		string lastTicker = "";
		string linePred;
		while( getline(ifsPred, linePred) )
		{
			vector<string> v = split(linePred, '\t');
			if( v[0] != "uid" )
			{
				//string ticker = v[1];
				string ticker = get_ticker(v);
				if( ticker != lastTicker )
				{
					tickers.push_back(ticker);
					lastTicker = ticker;
				}
			}
		}
	}
	return;
}

string HSignalNewsFile::get_predFile(int idate)
{
	string piece = "";
	if( market_ == "CA" )
		piece = "J";
	else if( market_ == "EU" )
		piece = "E";

	char predFile[200];
	sprintf(predFile, "%s\\pred%d%stm.txt", input_dir_.c_str(), idate, piece.c_str());
	return predFile;
}

string HSignalNewsFile::get_sigFile(int idate)
{
	string piece = "";
	if( market_ == "CA" )
		piece = "J";
	else if( market_ == "EU" )
		piece = "E";

	char sigFile[200];
	sprintf(sigFile, "%s\\sigTB%d%stm.txt", input_dir_.c_str(), idate, piece.c_str());
	return sigFile;
}

void HSignalNewsFile::print_mr(MultiRegress& mr, string market, string flag)
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
