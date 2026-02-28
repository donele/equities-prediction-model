#include <HLearn/HNNSampleRead.h>
#include <HLearnObj/InputInfo.h>
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

HNNSampleRead::HNNSampleRead(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
iMarket_(0),
nInput_(0),
verbose_(1),
normalize_(false),
norm_file_(""),
sample_dir_(""),
now_normalizing_(false),
write_root_(false),
do_hist_(false),
threadSampleRead_(false)
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("normalize") )
		normalize_ = true;
	if( conf.count("normFile") )
		norm_file_ = conf.find("normFile")->second;
	if( conf.count("nInput") )
		nInput_ = atoi( conf.find("nInput")->second.c_str() );
	if( conf.count("sampleDir") )
		sample_dir_ = conf.find("sampleDir")->second;
	if( conf.count("threadSampleRead") )
		threadSampleRead_ = conf.find("threadSampleRead")->second == "true";

	if( HEnv::Instance()->multiThreadTicker() && !threadSampleRead_ )
	{
		cerr << "Error HNNSampleRead::threadSampleRead_ is false when HEnv::multiThreadTicker is true." << endl;
		exit(5);
	}

	swThreadRead_.Reset();
}

HNNSampleRead::~HNNSampleRead()
{}

void HNNSampleRead::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	// Create InputInfo with number of input only.
	hnn::InputInfo nii(nInput_);
	HEvent::Instance()->add<hnn::InputInfo>("", "input_info", nii);

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

void HNNSampleRead::beginMarket()
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

void HNNSampleRead::beginDay()
{
	int idate = HEnv::Instance()->idate();
 
	// Open file.
	char filename[400];
	sprintf(filename, "%s\\%d.bin", sample_dir_.c_str(), idate);
	ifs_.open(filename, ios::in | ios::binary);

	// Read tickers.
	if( ifs_.is_open() )
	{
		hnn::SampleSummary st;
		ifs_ >> st;
		HEnv::Instance()->tickers(st.tickers);
		if( inputNames_.empty() )
		{
			inputNames_ = st.inputNames;
			HEvent::Instance()->add<vector<string> >("", "inputNames", inputNames_);
		}
	}
	else
		HEnv::Instance()->tickers(vector<string>());

	// threadSampleRead
	sampleReadLog_.clear();
	if( threadSampleRead_ )
		thread_ = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&HNNSampleRead::startThreadSampleRead, this)));

	if( verbose_ >= 1 )
	{
		vector<string> tickers = HEnv::Instance()->tickers();
		int ts = tickers.size();
		{
			cout << "\n";
			cout << "HNNSampleRead::beginDay " << idate << ". " << "ticker(s): ";
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

void HNNSampleRead::beginTicker(const string& ticker)
{
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
			//cerr << "sleep4" << endl;
		}
	}
	else // Read sample.
	{
		read_sample(ticker);
	}

	return;
}

void HNNSampleRead::endTicker(const string& ticker)
{
	HEvent::Instance()->remove<vector<hnn::Sample> >(ticker, "sample");
	if( threadSampleRead_ )
	{
		boost::mutex::scoped_lock lock(private_mutex_);
		sampleReadLog_.erase(ticker);
	}
	return;
}

void HNNSampleRead::endDay()
{
	ifs_.close();
	ifs_.clear();
	if( threadSampleRead_ && thread_ )
		thread_->join();
	return;
}

void HNNSampleRead::endMarket()
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

void HNNSampleRead::endJob()
{
	char buf[200];
	sprintf(buf, "\n%20s %19s %13s\n", moduleName_.c_str(), "Real Time", "CPU Time");
	cout << buf;

	printTime("threadRead", swThreadRead_.RealTime(), swThreadRead_.CpuTime());
	return;
}

void HNNSampleRead::read_sample(string ticker)
{
	vector<hnn::Sample> sample;
	hnn::SampleHeader sh;
	ifs_ >> sh;

	if( sh.ticker == ticker )
	{
		for( int i=0; i<sh.nEntries; ++i )
		{
			hnn::Sample aSample;
			ifs_ >> aSample;
			sample.push_back(aSample);
		}
	}
	else
	{
		cerr << "HNNSampleRead::read_sample read '" << sh.ticker << "' while expecting '" << ticker << "'" << endl;
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

void HNNSampleRead::normalize_sample(vector<hnn::Sample>& sample)
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

void HNNSampleRead::startThreadSampleRead()
{
	swThreadRead_.Start(kFALSE);
	int idate = HEnv::Instance()->idate();
	std::vector<std::string> tickers = HEnv::Instance()->tickers();
	for( std::vector<std::string>::iterator it = tickers.begin(); it != tickers.end(); ++it )
	{
		//
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
		read_sample(ticker);
		{
			boost::mutex::scoped_lock lock(private_mutex_);
			sampleReadLog_.insert(ticker);
		}
	}
	swThreadRead_.Stop();
	return;
}
