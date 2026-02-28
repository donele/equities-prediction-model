#include <HLearn/HBinSigRead.h>
#include <HLearnObj/InputInfo.h>
#include <HLearnObj/Signal.h>
#include <HLearnObj/Sample.h>
#include "optionlibs/TickData.h"
#include "HLib.h"
#include <jl_lib.h>
#include "TFile.h"
#include <algorithm>
#include <functional>
#include <vector>
#include <fstream>
using namespace std;

HBinSigRead::HBinSigRead(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
iMarket_(0),
nInput_(0),
verbose_(1),
normalize_(false),
norm_file_(""),
input_dir_(""),
now_normalizing_(false),
write_root_(true),
do_hist_(false),
threadSampleRead_(false),
targetIndx_(-1),
targetName_("resid")
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("normalize") )
		normalize_ = true;
	if( conf.count("normFile") )
		norm_file_ = conf.find("normFile")->second;
	//if( conf.count("nInput") )
	//	nInput_ = atoi( conf.find("nInput")->second.c_str() );
	if( conf.count("inputDir") )
		input_dir_ = conf.find("inputDir")->second;
	if( conf.count("predDir") )
		pred_dir_ = conf.find("predDir")->second;
	if( conf.count("threadSampleRead") )
		threadSampleRead_ = conf.find("threadSampleRead")->second == "true";
	if( conf.count("writeRoot") )
		write_root_ = conf.find("writeRoot")->second == "true";

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

	// Target Name.
	if( conf.count("targetName") )
		targetName_ = conf.find("targetName")->second;

	// Spectator Names.
	if( conf.count("spectator") )
	{
		pair<mmit, mmit> spectators = conf.equal_range("spectator");
		for( mmit mi = spectators.first; mi != spectators.second; ++mi )
		{
			string name = mi->second;
			spectatorNames_.push_back(name);
		}
	}
	nSpectator_ = spectatorNames_.size();

	if( HEnv::Instance()->multiThreadTicker() && !threadSampleRead_ )
	{
		cerr << "Error HBinSigRead::threadSampleRead_ is false when HEnv::multiThreadTicker is true." << endl;
		exit(5);
	}

	swThreadRead_.Reset();
}

HBinSigRead::~HBinSigRead()
{}

void HBinSigRead::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	// Create SignalInfo with number of input only.
	hnn::InputInfo nii(nInput_);
	HEvent::Instance()->add<hnn::InputInfo>("", "input_info", nii);
	HEvent::Instance()->add<vector<string> >("", "inputNames", inputNames_);
	HEvent::Instance()->add<vector<string> >("", "spectatorNames", spectatorNames_);

	TFile* f = HEnv::Instance()->outfile();
	f->cd();
	{
		boost::mutex::scoped_lock lock(HEnv::Instance()->root_mutex());
		f->mkdir(moduleName_.c_str());
	}
	f->cd(moduleName_.c_str());
	{
		boost::mutex::scoped_lock lock(HEnv::Instance()->root_mutex());
		gDirectory->mkdir("corr");
		gDirectory->mkdir("input");
	}

	return;
}

void HBinSigRead::beginMarket()
{
	// First epoch of training with input normalization turned on.
	if( normalize_ && 0 == iMarket_ )
	{
		if( norm_file_ == "" ) // Calculate the normalization parameters.
		{
			now_normalizing_ = true;
			input_n_ = 0;
			input_sum_ = vector<double>(nInput_);
			input_sum2_ = vector<double>(nInput_);
		}
		else // Read the parameters from file.
		{
			hnn::InputInfo nii(norm_file_);
			// Add to the event.
			HEvent::Instance()->add<hnn::InputInfo>("", "input_info", nii);
		}
	}

	// Calculate the target vs input correlation.
	if( write_root_ && 
		( (!normalize_ && 0 == iMarket_)
		|| (normalize_ && norm_file_.empty() && 1 == iMarket_)
		|| (normalize_ && !norm_file_.empty() && 0 == iMarket_) ) )
		do_hist_ = true;

	if( do_hist_ )
	{
		// One TH2F for each input variable.
		h_corr_ = vector<TH2F>(nInput_);
		for( int i=0; i<nInput_; ++i )
		{
			char name[40];
			sprintf(name, "h%d", i);
			char title[100];
			sprintf(title, "Target vs. Input %d", i);

			h_corr_[i] = TH2F(name, title, 40, 0, 1e-5, 40, 0, 1e-5);
			h_corr_[i].SetBit(TH1::kCanRebin);
		}

		// One TH1F for each input variable.
		h_input_ = vector<TH1F>(nInput_);
		for( int i=0; i<nInput_; ++i )
		{
			char name[40];
			sprintf(name, "d%d", i);
			char title[100];
			sprintf(title, "Input %d", i);

			h_input_[i] = TH1F(name, title, 40, 0, 1e-5);
			h_input_[i].SetBit(TH1::kCanRebin);
		}
	}

	return;
}

void HBinSigRead::beginMarketDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();

	set_ticker_list(market, idate);
	read_pred(market, idate);

	string filename = get_sigName(market, idate);
	ifs_.open(filename.c_str(), ios::in | ios::binary);

	if( ifs_.is_open() )
	{
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

	}

	// threadSignalRead
	sampleReadLog_.clear();
	if( threadSampleRead_ )
		thread_ = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&HBinSigRead::startThreadSampleRead, this)));

	if( verbose_ >= 1 )
	{
		vector<string> tickers = HEnv::Instance()->tickers();
		int ts = tickers.size();
		{
			cout << "\n";
			cout << "HBinSigRead::beginDay " << idate << ". " << "ticker(s): ";
			if( ts > 10 )
			{
				copy(tickers.begin(), tickers.begin() + 10, ostream_iterator<string>(cout, " "));
				cout << "... (" << ts << " tickers)";
			}
			else
				copy(tickers.begin(), tickers.end(), ostream_iterator<string>(cout, " "));
			cout << endl;
		}
	}

	return;
}

void HBinSigRead::beginTicker(const string& ticker)
{
	int idate = HEnv::Instance()->idate();
	if( threadSampleRead_ )
	{
		while(1)
		{
			bool sample_ready = false;
			{
				boost::mutex::scoped_lock lock(private_mutex_);
				sample_ready = sampleReadLog_.count(ticker);
			}
			if( sample_ready )
				break;
#ifdef _WIN32
			Sleep(100);
#else
			usleep(100);
#endif
		}
	}
	else // Read sample.
	{
		read_sample(ticker, idate);
	}

	return;
}

void HBinSigRead::endTicker(const string& ticker)
{
	HEvent::Instance()->remove<vector<hnn::Sample> >(ticker, "sample");
	if( threadSampleRead_ )
	{
		boost::mutex::scoped_lock lock(private_mutex_);
		sampleReadLog_.erase(ticker);
	}
	return;
}

void HBinSigRead::endMarketDay()
{
	ifs_.close();
	ifs_.clear();
	if( threadSampleRead_ && thread_ )
		thread_->join();
	return;
}

void HBinSigRead::endMarket()
{
	// Finalize input normalization.
	if( now_normalizing_ )
	{
		const hnn::InputInfo* pnii = static_cast<const hnn::InputInfo*>(HEvent::Instance()->get("", "input_info"));
		hnn::InputInfo nii = *pnii; // make a copy.

		for( int i=0; i<nInput_; ++i )
		{
			double mean = 0;
			double mean2 = 0;
			if( input_n_ >= 1 )
			{
				mean = input_sum_[i] / input_n_;
				mean2 = input_sum2_[i] / input_n_;
			}

			nii.mean[i] = mean;
			nii.stdev[i] = sqrt(mean2 - mean * mean);
		}

		// Add mean and stdev to the event.
		HEvent::Instance()->add<hnn::InputInfo>("", "input_info", nii);

		ofstream ofs("normalization.txt");
		ofs << nii;

		now_normalizing_ = false;
	}

	// Write the correlation calculation result.
	if( do_hist_ )
	{
		// Name and Title
		char name[40];
		char title[100];
		for( int i=0; i<nInput_; ++i )
		{
			sprintf(name, "%s", inputNames_[i].c_str());
			h_corr_[i].SetName(name);

			sprintf(title, "Target vs. Input %s", inputNames_[i].c_str());
			h_corr_[i].SetTitle(title);

			sprintf(name, "d_%s", inputNames_[i].c_str());
			h_input_[i].SetName(name);

			sprintf(title, "Input %s", inputNames_[i].c_str());
			h_input_[i].SetTitle(title);
		}

		// Write.
		TFile* f = HEnv::Instance()->outfile();
		f->cd();
		f->cd(moduleName_.c_str());
		gDirectory->cd("corr");
		for( int i=0; i<nInput_; ++i )
			h_corr_[i].Write();
		f->cd();
		f->cd(moduleName_.c_str());
		gDirectory->cd("input");
		for( int i=0; i<nInput_; ++i )
			h_input_[i].Write();

		do_hist_ = false;
	}

	++iMarket_;
	return;
}

void HBinSigRead::endJob()
{
	char buf[200];
	sprintf(buf, "\n%20s %19s %13s\n", moduleName_.c_str(), "Real Time", "CPU Time");
	cout << buf;

	printTime("threadRead", swThreadRead_.RealTime(), swThreadRead_.CpuTime());
	return;
}

void HBinSigRead::set_ticker_list(string market, int idate)
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

void HBinSigRead::read_pred(string market, int idate)
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

string HBinSigRead::get_sigName(string market, int idate)
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

string HBinSigRead::get_sigTxtName(string market, int idate)
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

string HBinSigRead::get_predName(string market, int idate)
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

void HBinSigRead::read_sample(string ticker, int idate)
{
	vector<hnn::Sample> sample;

	map<string, int>& mTickerCnt = mDateTickerCnt_[idate];
	vector<Pred>& pred = mTickerPred_[ticker];
	if( mTickerCnt.count(ticker) )
	{
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

			// target
			float target = 0;
			if( targetName_ == "target" )
				target = pred[i].target;
			else if( targetName_ == "resid" )
				target = pred[i].resid;

			if( verbose_ > 3 )
			{
				printf("%10.4f\t%10.4f\n", aSignal.input[22], pred[i].target);
			}

			// spectator
			vector<float> vs(nSpectator_);
			int j = 0;
			for( vector<string>::iterator it = spectatorNames_.begin(); it != spectatorNames_.end(); ++it, ++j )
			{
				string name = *it;
				double val = 0;
				if( name == "target" )
					val = pred[i].target;
				else if( name == "tbpred" )
					val = pred[i].tbpred;
				else if( name == "resid" )
					val = pred[i].resid;
				vs[j] = val;
			}

			sample.push_back(hnn::Sample(vi, target, vs));
		}
	}
	else
	{
		cerr << "HBinSigRead::read_sample read '" << ticker << "' is not found" << endl;
		exit(5);
	}

	// Normalize the sample.
	if( normalize_ && !now_normalizing_ )
		normalize_sample(sample);

	// Update the correlations.
	if( do_hist_ )
	{
		for( vector<hnn::Sample>::iterator it = sample.begin(); it != sample.end(); ++it )
		{
			vector<float>& input = it->input;
			double target = it->target;

			{
				boost::mutex::scoped_lock lock(HEnv::Instance()->root_mutex());
				for( int i=0; i<nInput_; ++i )
				{
					h_corr_[i].Fill(input[i], target);
					h_input_[i].Fill(input[i]);
				}
			}
		}
	}

	if( now_normalizing_ ) // Sum up the input variables.
	{
		for( vector<hnn::Sample>::iterator it = sample.begin(); it != sample.end(); ++it )
		{
			++input_n_;
			vector<float>& input = it->input;
			for( int i=0; i<nInput_; ++i )
			{
				input_sum_[i] += input[i];
				input_sum2_[i] += input[i] * input[i];
			}
		}
	}
	else // Add input to the event.
	{
		HEvent::Instance()->add<vector<hnn::Sample> >(ticker, "sample", sample);
	}

	return;
}

void HBinSigRead::normalize_sample(vector<hnn::Sample>& sample)
{
	const hnn::InputInfo* nii = static_cast<const hnn::InputInfo*>(HEvent::Instance()->get("", "input_info"));

	for( vector<hnn::Sample>::iterator it = sample.begin(); it != sample.end(); ++it )
	{
		vector<float>& input = it->input;
		for( int i=0; i<nInput_; ++i )
		{
			double input_normalized = input[i];
			if( nii->stdev[i] > 0 )
				input_normalized = (input[i] - nii->mean[i]) / nii->stdev[i];
			else if( fabs(nii->mean[i]) > 0 )
				input_normalized = (input[i] - nii->mean[i]) / nii->mean[i];
			else
				input_normalized = 0;
			input[i] = input_normalized;
		}
	}

	return;
}

void HBinSigRead::startThreadSampleRead()
{
	swThreadRead_.Start(kFALSE);
	int idate = HEnv::Instance()->idate();
	std::vector<std::string> tickers = HEnv::Instance()->tickers();
	for( std::vector<std::string>::iterator it = tickers.begin(); it != tickers.end(); ++it )
	{
		while( 0 )
		{
			int nLoaded = sampleReadLog_.size();
			if( nLoaded < 20 )
				break;
#ifdef _WIN32
			Sleep(1000);
#else 
			usleep(1000);
#endif
			cerr << "sleep1" << endl;
		}

		// Read
		std::string ticker = *it;
		read_sample(ticker, idate);
		{
			boost::mutex::scoped_lock lock(private_mutex_);
			sampleReadLog_.insert(ticker);
		}
	}
	swThreadRead_.Stop();
	return;
}
