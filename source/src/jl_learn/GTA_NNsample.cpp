#include "GTA_NNsample.h"
#include "optionlibs/TickData.h"
#include "GTEvent.h"
#include "GEX.h"
#include "GODBC.h"
#include "NNobj.h"
#include "GTEnv.h"
#include "TFile.h"
#include "mto.h"
#include "jlutil.h"
#include <algorithm>
#include <functional>
#include <vector>
#include <fstream>
using namespace std;

GTA_NNsample::GTA_NNsample(const string& moduleName, const multimap<string, string>& conf)
:GTModule(moduleName),
iMarket_(0),
nInput_(0),
targetTime_(60000),
sampleInterval_(60000),
normalize_(false),
norm_file_(""),
now_normalizing_(false),
do_corr_(false),
adjust_market_(false),
log_share_(false),
lotSize_(1),
input_bidSize_(false),
input_askSize_(false),
input_spread_(false),
input_quoteImb_(false),
input_volume_(false)
{
	if( conf.count("targetTime") )
		targetTime_ = atoi( conf.find("targetTime")->second.c_str() );
	if( conf.count("sampleInterval") )
		sampleInterval_ = atoi( conf.find("sampleInterval")->second.c_str() );
	if( conf.count("normalize") )
		normalize_ = true;
	if( conf.count("normFile") )
		norm_file_ = conf.find("normFile")->second;
	if( conf.count("adjustMarket") )
		adjust_market_ = conf.find("adjustMarket")->second == "true";
	if( conf.count("logShare") )
		log_share_ = conf.find("logShare")->second == "true";

	if( conf.count("pastRtnTime") )
	{
		pair<mmit, mmit> values = conf.equal_range("pastRtnTime");
		for( mmit mi = values.first; mi != values.second; ++mi )
		{
			int minute = atoi(mi->second.c_str());
			pastRtnTime_.insert(minute);
		}
	}
	for( set<int>::reverse_iterator it = pastRtnTime_.rbegin(); it != pastRtnTime_.rend(); ++it )
	{
		char iname[100];
		sprintf( iname, "rtn_%dm", *it );
		inputNames_.push_back(iname);
	}
	nPastRtn_ = pastRtnTime_.size();
	nInput_ += nPastRtn_;
	if( conf.count("input") )
	{
		pair<mmit, mmit> values = conf.equal_range("input");
		for( mmit mi = values.first; mi != values.second; ++mi )
		{
			++nInput_;
			string name = mi->second;
			if( "bidSize" == name )
			{
				input_bidSize_ = true;
				inputNames_.push_back("bidSize");
			}
			else if( "askSize" == name )
			{
				input_askSize_ = true;
				inputNames_.push_back("askSize");
			}
			else if( "spread" == name )
			{
				input_spread_ = true;
				inputNames_.push_back("spread");
			}
			else if( "quoteImb" == name )
			{
				input_quoteImb_ = true;
				inputNames_.push_back("quoteImb");
			}
			else if( "volume" == name )
			{
				input_volume_ = true;
				inputNames_.push_back("volume");
			}
			else
				--nInput_;
		}
	}
}

GTA_NNsample::~GTA_NNsample()
{}

void GTA_NNsample::beginJob()
{
	NNinputInfo nii(nInput_);
	GTEvent::Instance()->add<NNinputInfo>("input_info", nii);

	TFile* f = GTEnv::Instance()->outfile();
	f->cd();
	f->mkdir(moduleName_.c_str());
	f->cd(moduleName_.c_str());
	gDirectory->mkdir("corr");

	return;
}

void GTA_NNsample::beginMarket()
{
	string market = GTEvent::Instance()->market();
	sessions_ = mto::sessions(market, 30, 0, 10);
	msecOpen_ = mto::msecOpen(market);
	msecClose_ = mto::msecClose(market);
	if( market == "US" )
		lotSize_ = 100;

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
			NNinputInfo nii(norm_file_);
			// Add mean and stdev to the event.
			GTEvent::Instance()->add<NNinputInfo>("input_info", nii);
		}
	}

	// Calculate the target vs input correlation.
	if( (!normalize_ && 0 == iMarket_)
		|| (normalize_ && norm_file_.empty() && 1 == iMarket_)
		|| (normalize_ && !norm_file_.empty() && 0 == iMarket_) )
		do_corr_ = true;

	if( do_corr_ )
	{
		// One TH2F for each input variable.
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
	}

	return;
}

void GTA_NNsample::beginDay()
{
	// Read volume from stockCharacteristics table.
	if( input_volume_ )
	{
		mVol_.clear();
		string market = GTEvent::Instance()->market();
		int idate = GTEvent::Instance()->idate();
		int idate_prev = (int)GEX::Instance()->get(market)->PrevClose(QuoteTime(idate, 040000, mto::tz(market))).Date();

		string cmd = (string)"select ticker, medVolume "
			+ " from stockCharacteristics "
			+ " where " + mto::selChara(market, idate_prev)
			+ " and medVolume is not null ";
		vector<vector<string> > vv;
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			if( !ticker.empty() )
			{
				double medVol = atof( (*it)[1].c_str() );
				mVol_.insert( make_pair(ticker, medVol) );
			}
		}
	}

	// Market Adjust
	if( adjust_market_ )
	{
		TickSeries<ReturnData>* ptsR = static_cast<TickSeries<ReturnData>*>(GTEvent::Instance()->get("returns"));
		int nSecInterval = (msecClose_ - msecOpen_)/1000 + 1;
		secRetCum_ = vector<double>(nSecInterval, 0);
		vector<double> secRet = vector<double>(nSecInterval, 0);

		int ntr = ptsR->NTicks();
		ptsR->StartRead();
		ReturnData r;
		for( int n=0; n<ntr; ++n )
		{
			ptsR->Read(&r);
			int rIndx = (r.msecs - msecOpen_)/1000;
			secRet[rIndx] = r.ret;
		}

		secRetCum_[0] = 1.0;
		for( int n=1; n<nSecInterval; ++n )
		{
			secRetCum_[n] = secRetCum_[n-1] * secRet[n] + secRetCum_[n-1];
		}
	}

	return;
}

void GTA_NNsample::doTicker()
{
	// Access tick data.
	TickSeries<QuoteInfo>* ptsQ = static_cast<TickSeries<QuoteInfo>*>(GTEvent::Instance()->get("quotes"));

	// Make sample.
	vector<NNsample> sample;
	get_sample(ptsQ, sample);

	// Normalize the sample.
	if( normalize_ && !now_normalizing_ )
		normalize_sample(sample);

	// Update the correlations.
	if( do_corr_ )
	{
		for( vector<NNsample>::iterator it = sample.begin(); it != sample.end(); ++it )
		{
			vector<double>& input = it->input;
			double target = it->target;

			for( int i=0; i<nInput_; ++i )
				h_corr_[i]->Fill(input[i], target);
		}
	}

	if( now_normalizing_ ) // Sum up the input variables.
	{
		for( vector<NNsample>::iterator it = sample.begin(); it != sample.end(); ++it )
		{
			++input_n_;
			vector<double>& input = it->input;
			for( int i=0; i<nInput_; ++i )
			{
				input_sum_[i] += input[i];
				input_sum2_[i] += input[i] * input[i];
			}
		}
	}
	else // Add input to the event.
	{
		GTEvent::Instance()->add<vector<NNsample> >("sample", sample);
	}

	return;
}

void GTA_NNsample::endDay()
{
	return;
}

void GTA_NNsample::endMarket()
{
	// Finalize input normalization.
	if( now_normalizing_ )
	{
		NNinputInfo* pnii = static_cast<NNinputInfo*>(GTEvent::Instance()->get("input_info"));
		NNinputInfo nii = *pnii;

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
		GTEvent::Instance()->add<NNinputInfo>("input_info", nii);

		ofstream ofs("normalization.txt");
		ofs << nii;

		now_normalizing_ = false;
	}

	// Write the correlation calculation result.
	if( do_corr_ )
	{
		// Write.
		TFile* f = GTEnv::Instance()->outfile();
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
		do_corr_ = false;
	}

	++iMarket_;
	return;
}

void GTA_NNsample::endJob()
{
	return;
}


void GTA_NNsample::get_sample(TickSeries<QuoteInfo>* ptsQ, vector<NNsample>& sample)
{
	vector<QuoteInfo> vq;
	get_quotes(ptsQ, vq); // Read tick data.
	get_sample(vq, sample); // Make sample.

	return;
}

void GTA_NNsample::get_quotes(TickSeries<QuoteInfo>* ptsQ, vector<QuoteInfo>& vq)
{
	int ntq = ptsQ->NTicks();
	vq.reserve(ntq);

	ptsQ->StartRead();
	QuoteInfo quote;
	double last_mid = -1;
	for( int n=0; n<ntq; ++n )
	{
		ptsQ->Read(&quote);
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

void GTA_NNsample::normalize_sample(vector<NNsample>& sample)
{
	NNinputInfo* nii = static_cast<NNinputInfo*>(GTEvent::Instance()->get("input_info"));

	for( vector<NNsample>::iterator it = sample.begin(); it != sample.end(); ++it )
	{
		vector<double>& input = it->input;
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

void GTA_NNsample::get_sample(vector<QuoteInfo>& vq, vector<NNsample>& sample)
{
	string ticker = GTEvent::Instance()->ticker();
	double last_msecs = 0;
	for( vector<QuoteInfo>::iterator it = vq.begin(); it != vq.end(); ++it )
	{
		int msecs = it->msecs;
		if( msecs - last_msecs < sampleInterval_ ) // time between samples.
			continue;

		double midRaw = (it->ask + it->bid) / 2.0; // Valid spread.
		if( midRaw <= 0 )
			continue;
		double mid = midRaw;
		if( adjust_market_ )
			mid = midRaw / secRetCum_[(msecs-msecOpen_)/1000];

		vector<QuoteInfo>::iterator itqb = vq.begin();
		vector<QuoteInfo>::iterator itqe = vq.end();
		if( msecs - itqb->msecs <= 10*60*1000 ) // Not too near the beginning of the time series.
			continue;

		if( distance(itqb, itqe) < 20 ) // Minimum number of quotes = 20.
			continue;

		if( (itqe-1)->msecs - msecs < 10*60*1000 ) // Not too near the end of the time series.
			continue;

		// Find the prices.
		map<int, double> m_msecs_mid;
		int cumPastRtnTime = 0;
		for( set<int>::iterator itp = pastRtnTime_.begin(); itp != pastRtnTime_.end(); ++itp )
		{
			cumPastRtnTime += (*itp);
			m_msecs_mid[msecs - cumPastRtnTime*60*1000] = 0;
		}
		m_msecs_mid[msecs + targetTime_] = 0;

		for( vector<QuoteInfo>::iterator itq = itqb; itq != itqe; ++itq )
		{
			if( itq->msecs <= msecs + targetTime_ )
			{
				double itqmid = (itq->ask + itq->bid) / 2.0;
				if( adjust_market_ )
					itqmid = itqmid / secRetCum_[(itq->msecs - msecOpen_)/1000];
				map<int, double>::iterator it2 = m_msecs_mid.begin();
				for( ; it2 != m_msecs_mid.end(); ++it2 )
				{
					if( it2->first >= itq->msecs )
						it2->second = itqmid;
				}
			}
			else
				break;
		}

		vector<double> vPrc;
		for( map<int, double>::iterator it2 = m_msecs_mid.begin(); it2 != m_msecs_mid.end(); ++it2 )
			vPrc.push_back(it2->second);

		// Past returns
		vector<double> vPastRet;
		for( int i = 0; i < nPastRtn_; ++i )
		{
			double pastRet = 0;
			double pastPrc = vPrc[i];
			if(	pastPrc > 0 )
				pastRet = (mid - pastPrc) / pastPrc;
			vPastRet.push_back(pastRet);
		}

		// Input.
		vector<double> input;
		for( vector<double>::iterator itp = vPastRet.begin(); itp != vPastRet.end(); ++itp )
			input.push_back(*itp);
		if( input_bidSize_ )
		{
			double sze = it->bidSize * lotSize_;
			if( log_share_ && sze >= 1 )
				sze = log(sze);
			input.push_back(sze);
		}
		if( input_askSize_ )
		{
			double sze = it->askSize * lotSize_;
			if( log_share_ && sze >= 1 )
				sze = log(sze);
			input.push_back(sze);
		}
		if( input_spread_ )
		{
			double sprd = (it->ask - it->bid) / midRaw;
			input.push_back(sprd);
		}
		if( input_quoteImb_ )
		{
			double quoteImb = 2.0 * (it->askSize - it->bidSize) / (it->askSize + it->bidSize);
			input.push_back(quoteImb);
		}
		if( input_volume_ )
		{
			double volume = log(mVol_[ticker]);
			if( volume < 0 )
				volume = 0;
			input.push_back(volume);
		}

		// Target
		double targetPrc = vPrc[nPastRtn_];
		double target = (targetPrc - mid) / mid;

		sample.push_back(NNsample(input, target, *it));
		last_msecs = msecs;
	}
			
	return;
}