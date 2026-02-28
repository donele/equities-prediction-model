#include <HLearn/HNNSampleCommodity.h>
#include <HLearnObj/Sample.h>
#include <HLearnObj/InputInfo.h>
#include "optionlibs/TickData.h"
#include "HLib.h"
#include <jl_lib.h>
#include "TFile.h"
#include <algorithm>
#include <functional>
#include <vector>
#include <fstream>
using namespace std;

HNNSampleCommodity::HNNSampleCommodity(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
nInput_(0),
targetTime_(60),
sampleInterval_(60),
hedge_inputs_(false),
hedge_target_(true)
{
	if( conf.count("targetTime") )
		targetTime_ = atoi( conf.find("targetTime")->second.c_str() );
	if( conf.count("sampleInterval") )
		sampleInterval_ = atoi( conf.find("sampleInterval")->second.c_str() );
	if( conf.count("hedgeInputs") )
		hedge_inputs_ = conf.find("hedgeInputs")->second == "true";
	if( conf.count("hedgeTarget") )
		hedge_target_ = conf.find("hedgeTarget")->second == "true";

	if( conf.count("northFieldSector") )
	{
		pair<mmit, mmit> sectors = conf.equal_range("northFieldSector");
		for( mmit mi = sectors.first; mi != sectors.second; ++mi )
		{
			int sector = atoi(mi->second.c_str());
			vNorthFieldSector_.push_back(sector);
		}
	}
	if( conf.count("leadingTicker") )
	{
		leadingTicker_ = conf.find("leadingTicker")->second;
	}
	if( conf.count("sampleDir") )
		sample_dir_ = conf.find("sampleDir")->second;
	if( conf.count("pastRtnTime") )
	{
		pair<mmit, mmit> values = conf.equal_range("pastRtnTime");
		for( mmit mi = values.first; mi != values.second; ++mi )
		{
			int second = atoi(mi->second.c_str());
			pastRtnTime_.insert(second);
		}
	}
	for( set<int>::iterator it = pastRtnTime_.begin(); it != pastRtnTime_.end(); ++it )
	{
		char iname[100];
		sprintf( iname, "rtn_%ds", *it );
		inputNames_.push_back(iname);
	}

	nInput_ = inputNames_.size();
	if( HEnv::Instance()->multiThreadTicker() )
	{
		cerr << "HNNSampleCommodity:Error multiThreadTicker is not supported." << endl;
		exit(5);
	}
}

HNNSampleCommodity::~HNNSampleCommodity()
{}

void HNNSampleCommodity::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	// Create InputInfo with number of input only.
	hnn::InputInfo nii(nInput_);
	HEvent::Instance()->add<hnn::InputInfo>("", "input_info", nii);

	TFile* f = HEnv::Instance()->outfile();
	f->cd();
	f->mkdir(moduleName_.c_str());
	f->cd(moduleName_.c_str());
	gDirectory->mkdir("corr");

	return;
}

void HNNSampleCommodity::beginMarket()
{
	string market = HEnv::Instance()->market();

	h_corr_ = vector<TH2F*>(nInput_);
	for( int i=0; i<nInput_; ++i )
	{
		char name[40];
		sprintf(name, "%s", inputNames_[i].c_str());
		char title[100];
		sprintf(title, "Target vs. Input %s", inputNames_[i].c_str());

		h_corr_[i] = new TH2F(name, title, 40, 0, 1e-5, 40, 0, 1e-5);
		h_corr_[i]->SetBit(TH1::kCanRebin);
	}

	return;
}

void HNNSampleCommodity::beginDay()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	sessions_ = mto::sessions(market, 30, 0, 10, idate);
	msecOpen_ = mto::msecOpen(market, idate);
	msecClose_ = mto::msecClose(market, idate);
	vector<string> tickers = HEnv::Instance()->tickers();

	const TickSeries<ReturnData>* ptsR = static_cast<const TickSeries<ReturnData>*>(HEvent::Instance()->get(mto::retName(market), "returns"));
	int nSecInterval = (msecClose_ - msecOpen_)/1000 + 1;
	secRetCum_ = vector<double>(nSecInterval, 0);
	vector<double> secRet = vector<double>(nSecInterval, 0);

	int ntr = ptsR->NTicks();
	const_cast<TickSeries<ReturnData>*>(ptsR)->StartRead();
	ReturnData r;
	for( int n=0; n<ntr; ++n )
	{
		const_cast<TickSeries<ReturnData>*>(ptsR)->Read(&r);
		int rIndx = (r.msecs - msecOpen_)/1000;
		if( rIndx < nSecInterval )
			secRet[rIndx] = r.ret;
	}

	secRetCum_[0] = 1.0;
	for( int n=1; n<nSecInterval; ++n )
		secRetCum_[n] = secRetCum_[n-1] * secRet[n] + secRetCum_[n-1];

	// Open file to write the sample.
	char filename[400];
	sprintf(filename, "%s\\%d.bin", sample_dir_.c_str(), idate);
	ofs_.open( filename, ios::out | ios::binary );

	// Write SampleSummary.
	hnn::SampleSummary ss(inputNames_, tickers);
	ofs_ << ss;

	return;
}

void HNNSampleCommodity::beginTicker(const string& ticker)
{
	// Access tick data.
	const vector<double>* msecQ1s = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "msecQ1s"));
	const vector<double>* midQ1s = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "midQ1s"));
	const vector<double>* msecQ1sL = static_cast<const vector<double>*>(HEvent::Instance()->get("", leadingTicker_ + "_msecQ1s"));
	const vector<double>* midQ1sL = static_cast<const vector<double>*>(HEvent::Instance()->get("", leadingTicker_ + "_midQ1s"));

	// Make sample.
	vector<hnn::Sample> sample;
	get_sample(ticker, msecQ1s, midQ1s, msecQ1sL, midQ1sL, sample);

	// Save sample.
	hnn::SampleHeader sh(ticker, sample.size());
	ofs_ << sh;
	for( vector<hnn::Sample>::iterator it = sample.begin(); it != sample.end(); ++it )
		ofs_ << *it;

	// correlation.
	for( vector<hnn::Sample>::iterator it = sample.begin(); it != sample.end(); ++it )
	{
		vector<float>& input = it->input;
		double target = it->target;

		{
			boost::mutex::scoped_lock lock(HEnv::Instance()->root_mutex());
			for( int i=0; i<nInput_; ++i )
				h_corr_[i]->Fill(input[i], target);
		}
	}

	return;
}

void HNNSampleCommodity::endTicker(const string& ticker)
{
	return;
}

void HNNSampleCommodity::endDay()
{
	ofs_.close();
	ofs_.clear();
	return;
}

void HNNSampleCommodity::endMarket()
{
	// Write.
	TFile* f = HEnv::Instance()->outfile();
	f->cd();
	f->cd(moduleName_.c_str());
	gDirectory->cd("corr");
	for( int i=0; i<nInput_; ++i )
		h_corr_[i]->Write();

	// Delete.
	h_corr_ = vector<TH2F*>(nInput_);
	for( int i=0; i<nInput_; ++i )
	{
		TH2F* h = h_corr_[i];
		delete h;
	}
	return;
}

void HNNSampleCommodity::endJob()
{
	return;
}

void HNNSampleCommodity::get_sample(string ticker, const vector<double>* msecQ1s, const vector<double>* midQ1s,
		const vector<double>* msecQ1sL, const vector<double>* midQ1sL, vector<hnn::Sample>& sample)
{
	int N = msecQ1s->size();
	int NL = msecQ1sL->size();
	if( N > 0 && NL > 0 )
	{
		int offset = (*msecQ1s)[0]/1000 - (*msecQ1sL)[0]/1000;
		for( int i=0; i<N; i += sampleInterval_ )
		{
			int iL = i + offset;
			if( i + targetTime_ + 1 <= N - 1 && iL > 120 )
			{
				vector<float> vPastRet;
				int cumPastRtnTime = 0;
				for( set<int>::iterator it = pastRtnTime_.begin(); it != pastRtnTime_.end(); ++it )
				{
					int length = *it;
					cumPastRtnTime += length;
					double rtn = 0;

					int iFromTemp = iL - cumPastRtnTime;
					int iTo = iFromTemp + length;
					if( iTo >= 0 )
					{
						int iFrom = max(0, iFromTemp);
						if( (*midQ1sL)[iFrom] < 1e-100 )
						{
							cerr << "ERROR: price is zero.\n";
							exit(4);
						}
						if( hedge_inputs_ )
						{
							double p0 = (*midQ1sL)[iFrom] / secRetCum_[iFrom];
							double p1 = (*midQ1sL)[iTo] / secRetCum_[iTo];
							rtn = (p1 - p0) / p0;
						}
						else
						{
							rtn = ((*midQ1sL)[iTo] - (*midQ1sL)[iFrom]) / (*midQ1sL)[iFrom];
						}
					}
					vPastRet.push_back(rtn);
				}

				double targetPrc = (*midQ1s)[i + targetTime_ + 1];
				double oneSecPrc = (*midQ1s)[i + 1];
				if( hedge_target_ )
				{
					targetPrc /= secRetCum_[i + targetTime_ + 1];
					oneSecPrc /= secRetCum_[i + 1];
				}
				float target = (targetPrc - oneSecPrc) / oneSecPrc;

				sample.push_back(hnn::Sample(vPastRet, target));
			}
		}
	}

	return;
}