#include "GTA_Bsample.h"
#include "optionlibs/TickData.h"
#include "GTEvent.h"
#include "Butil.h"
#include "GTEnv.h"
#include "TFile.h"
#include "mto.h"
#include "jlutil.h"
#include <algorithm>
#include <functional>
#include <vector>
#include <fstream>
using namespace std;

GTA_Bsample::GTA_Bsample(const string& moduleName, const multimap<string, string>& conf)
:GTModule(moduleName),
iMarket_(0),
iModel_(0),
timeFutureRet_(61000),
interval_(60000),
gamma_(0),
normalize_(false),
norm_file_(""),
now_normalizing_(false),
do_corr_(false)
{
	if( conf.count("iModel") )
		iModel_ = atoi( conf.find("iModel")->second.c_str() );
	nInput_ = Butil::get_nInput(iModel_);
	nOutput_ = Butil::get_nOutput(iModel_);
	if( conf.count("timeFutureRet") )
		timeFutureRet_ = atoi( conf.find("timeFutureRet")->second.c_str() );
	if( conf.count("interval") )
		interval_ = atoi( conf.find("interval")->second.c_str() );
	if( conf.count("gamma") )
		gamma_ = atof( conf.find("gamma")->second.c_str() );
	if( conf.count("normalize") )
		normalize_ = true;
	if( conf.count("normFile") )
		norm_file_ = conf.find("normFile")->second;
}

GTA_Bsample::~GTA_Bsample()
{}

void GTA_Bsample::beginJob()
{
	TFile* f = GTEnv::Instance()->outfile();
	f->cd();
	f->mkdir(moduleName_.c_str());
	f->cd(moduleName_.c_str());
	gDirectory->mkdir("corr");

	return;
}

void GTA_Bsample::beginMarket()
{
	string market = GTEvent::Instance()->market();
	sessions_ = mto::sessions(market, 30, 0, 10);

	if( normalize_ && 0 == iMarket_ )
	{
		if( norm_file_ == "" )
		{
			now_normalizing_ = true;
			input_n_ = 0;
			input_sum_ = vector<double>(nInput_);
			input_sum2_ = vector<double>(nInput_);
		}
		else
		{
			BnormInfo ni(norm_file_);
			// Add mean and stdev to the event.
			GTEvent::Instance()->add<BnormInfo>("norm_info", ni);
		}
	}

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
			string inputName = Butil::get_inputName(iModel_, i);
			char name[40];
			sprintf(name, "%s", inputName.c_str());
			char title[100];
			sprintf(title, "Target vs. Input %s", inputName.c_str());

			h_corr_[i] = new TH2F(name, title, 40, 0, 1e-5, 40, 0, 1e-5);
			h_corr_[i]->SetBit(TH1::kCanRebin);
		}
	}

	return;
}

void GTA_Bsample::beginDay()
{
	return;
}

void GTA_Bsample::doTicker()
{
	// Access tick data.
	TickSeries<QuoteInfo>* ptsQ = static_cast<TickSeries<QuoteInfo>*>(GTEvent::Instance()->get("quotes"));

	// make sample.
	vector<Bsample> sample;
	get_sample(ptsQ, sample);

	if( normalize_ && !now_normalizing_ )
		normalize_sample(sample);

	// Update the correlations.
	if( do_corr_ )
	{
		for( vector<Bsample>::iterator it = sample.begin(); it != sample.end(); ++it )
		{
			vector<double>& input = it->input;
			vector<double>& target = it->target;

			for( int i=0; i<nInput_; ++i )
				h_corr_[i]->Fill(input[i], target[0]);
		}
	}

	if( now_normalizing_ )
	{
		for( vector<Bsample>::iterator it = sample.begin(); it != sample.end(); ++it )
		{
			vector<double>& input = it->input;
			for( int i=0; i<nInput_; ++i )
			{
				++input_n_;
				input_sum_[i] += input[i];
				input_sum2_[i] += input[i] * input[i];
			}
		}
	}
	else
	{
		// Add input to the event.
		GTEvent::Instance()->add<vector<Bsample> >("sample", sample);
	}

	return;
}

void GTA_Bsample::endDay()
{
	return;
}

void GTA_Bsample::endMarket()
{
	if( now_normalizing_ )
	{
		BnormInfo ni(nInput_);

		//vector<double> input_mean(nInput_);
		//vector<double> input_stdev(nInput_);

		for( int i=0; i<nInput_; ++i )
		{
			double mean = 0;
			double mean2 = 0;
			if( input_n_ >= 1 )
			{
				mean = input_sum_[i] / input_n_;
				mean2 = input_sum2_[i] / input_n_;
			}

			//input_mean[i] = mean;
			//input_stdev[i] = sqrt(mean2 - mean * mean);
			ni.mean[i] = mean;
			ni.stdev[i] = sqrt(mean2 - mean * mean);
		}

		// Add mean and stdev to the event.
		//GTEvent::Instance()->add<vector<double> >("input_mean", input_mean);
		//GTEvent::Instance()->add<vector<double> >("input_stdev", input_stdev);
		GTEvent::Instance()->add<BnormInfo>("norm_info", ni);

		ofstream ofs("normalization.txt");
		ofs << ni;

		now_normalizing_ = false;
	}

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

void GTA_Bsample::endJob()
{
	return;
}


void GTA_Bsample::get_sample(TickSeries<QuoteInfo>* ptsQ, vector<Bsample>& sample)
{
	vector<QuoteInfo> vq;
	get_quotes(ptsQ, vq);

	if( 0 == iModel_ )
		get_sample_10t(vq, sample);
	else if( 1 == iModel_ || 2 == iModel_ || 3 == iModel_ )
		get_sample_1m(vq, sample);
	else if( 4 == iModel_ )
		get_sample_1m_v2(vq, sample);

	return;
}

void GTA_Bsample::get_quotes(TickSeries<QuoteInfo>* ptsQ, vector<QuoteInfo>& vq)
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

void GTA_Bsample::normalize_sample(vector<Bsample>& sample)
{
	//vector<double>* input_mean = static_cast<vector<double>*>(GTEvent::Instance()->get("input_mean"));
	//vector<double>* input_stdev = static_cast<vector<double>*>(GTEvent::Instance()->get("input_stdev"));
	BnormInfo* ni = static_cast<BnormInfo*>(GTEvent::Instance()->get("norm_info"));

	for( vector<Bsample>::iterator it = sample.begin(); it != sample.end(); ++it )
	{
		vector<double>& input = it->input;
		for( int i=0; i<nInput_; ++i )
		{
			double input_normalized = 0;
			if( ni->stdev[i] > 0 )
				input_normalized = (input[i] - ni->mean[i]) / ni->stdev[i];
				//input_normalized = (input[i] - (*input_mean)[i]) / (*input_stdev)[i];
			input[i] = input_normalized;
		}
	}

	return;
}

void GTA_Bsample::get_sample_10t(vector<QuoteInfo>& vq, vector<Bsample>& sample)
{
	vector<double> input(nInput_);
	vector<double> target(nOutput_);
	for( vector<QuoteInfo>::iterator it = vq.begin(); it != vq.end(); ++it )
	{
		vector<QuoteInfo>::iterator itqb = vq.begin();
		vector<QuoteInfo>::iterator itqe = vq.end();
		if( distance(itqb, it) < nInput_ )
			continue;
		if( distance(it, itqe) <= nInput_ )
			continue;

		vector<QuoteInfo>::iterator temp_it = it;
		for( int i=0; i<nInput_; ++i )
			--temp_it;
		double last_mid = (temp_it->ask + temp_it->bid) / 2.0;
		for( int i=0; i<=nInput_; ++i, ++temp_it )
		{
			if( i > 0 )
			{
				double mid = (temp_it->ask + temp_it->bid) / 2.0;
				input[i - 1] = (mid - last_mid) / last_mid;
				last_mid = mid;
			}
		}
		double mid = (temp_it->ask + temp_it->bid) / 2.0;
		target[0] = (mid - last_mid) / last_mid;

		sample.push_back(Bsample(input, target, *it));
	}

	return;
}

void GTA_Bsample::get_sample_1m(vector<QuoteInfo>& vq, vector<Bsample>& sample)
{
	vector<double> input(nInput_);
	vector<double> target(nOutput_);
	double last_msecs = 0;
	for( vector<QuoteInfo>::iterator it = vq.begin(); it != vq.end(); ++it )
	{
		int msecs = it->msecs;
		if( msecs - last_msecs < interval_ )
			continue;

		double mid = (it->ask + it->bid) / 2.0;
		if( mid <= 0 )
			continue;

		vector<QuoteInfo>::iterator itqb = vq.begin();
		vector<QuoteInfo>::iterator itqe = vq.end();
		if( msecs - itqb->msecs <= 960000 )
			continue;

		if( distance(itqb, itqe) < 10 )
			continue;

		if( (itqe-1)->msecs - msecs < timeFutureRet_ )
			continue;

		double close = ((itqe-1)->ask + (itqe-1)->bid) / 2.0;
		double rtnClose = (close - mid) / mid;

		double mid_1m = 0;
		double mid_5m = 0;
		double mid_10m = 0;
		double mid_fut_1s = mid;
		double mid_fut = 0;
		for( vector<QuoteInfo>::iterator itq = itqb; itq != itqe; ++itq )
		{
			if( itq->msecs <= msecs + timeFutureRet_ )
			{
				double itmid = (itq->ask + itq->bid) / 2.0;
				if( itq->msecs > msecs + 1000 )
					mid_fut = itmid;
				else if( itq->msecs > msecs )
					mid_fut_1s = itmid;
				else if( itq->msecs <= msecs - 60000 )
				{
					if( itq->msecs > msecs - 360000 )
						mid_1m = itmid;
					else if( itq->msecs <= msecs - 360000 )
					{
						if( itq->msecs > msecs - 960000 )
							mid_5m = itmid;
						else if( itq->msecs <= msecs - 960000 )
							if( itq->msecs > msecs - 1920000 )
								mid_10m = itmid;
					}
				}
			}
			else
				break;
		}

		if( mid_1m <= 0 || mid_5m <= 0 || mid_10m <= 0 || mid_fut <= 0 )
			continue;

		double quote_imb = 2.0 * (it->askSize - it->bidSize) / (it->askSize + it->bidSize);
		double sprd = (it->ask - it->bid) / mid;
		double ret_1m = 0;
		double ret_5m = 0;
		double ret_10m = 0;
		ret_1m = (mid - mid_1m) / mid_1m;
		ret_5m = (mid - mid_5m) / mid_5m;
		ret_10m = (mid - mid_10m) / mid_10m;

		input[0] = ret_1m;
		input[1] = ret_5m;
		input[2] = ret_10m;
		input[3] = quote_imb * 1e-3;
		input[4] = sprd;
		if( nInput_ >= 6 )
		{
			input[5] = 0;
		}
		if( nInput_ > 6 )
		{
			for( int i=6; i<nInput_; ++i )
				input[i] = 0;
		}
		if( 3 == iModel_ )
		{
			double sumret = 0;
			int msec0 = it->msecs;
			QuoteInfo last_quote = *it;
			for( vector<QuoteInfo>::iterator itf = it; itf != itqe; ++itf )
			{
				int msec = itf->msecs;
				if( msec - last_quote.msecs > timeFutureRet_ )
				{
					double dmin = ceil((msec - msec0)/1000.0/60.0 - 0.5);
					double factor = pow(gamma_, dmin - 1);
					if( factor < 0.005 )
						break;

					double last_mid = (last_quote.bid + last_quote.ask) / 2.0;
					double mid = (itf->bid + itf->ask) / 2.0;
					sumret += factor * (mid - last_mid) / last_mid;

					last_quote = *itf;
				}
			}
			target[0] = sumret;
		}
		else
			target[0] = (mid_fut - mid) / mid;

		sample.push_back(Bsample(input, target, *it));
		last_msecs = msecs;
	}
			
	return;
}


void GTA_Bsample::get_sample_1m_v2(vector<QuoteInfo>& vq, vector<Bsample>& sample)
{
	vector<double> input(nInput_);
	vector<double> target(nOutput_);
	double last_msecs = 0;
	for( vector<QuoteInfo>::iterator it = vq.begin(); it != vq.end(); ++it )
	{
		int msecs = it->msecs;
		if( msecs - last_msecs < interval_ ) // time from sample to sample
			continue;

		double mid = (it->ask + it->bid) / 2.0;
		if( mid <= 0 )
			continue;

		vector<QuoteInfo>::iterator itqb = vq.begin();
		vector<QuoteInfo>::iterator itqe = vq.end();
		if( msecs - itqb->msecs <= 960000 )
			continue;

		if( distance(itqb, itqe) < 10 )
			continue;

		if( (itqe-1)->msecs - msecs < timeFutureRet_ )
			continue;

		double close = ((itqe-1)->ask + (itqe-1)->bid) / 2.0;
		double rtnClose = (close - mid) / mid;

		double mid_1m = 0;
		double mid_5m = 0;
		double mid_10m = 0;
		double mid_fut_1s = mid;
		double mid_fut = 0;
		for( vector<QuoteInfo>::iterator itq = itqb; itq != itqe; ++itq )
		{
			if( itq->msecs <= msecs + timeFutureRet_ )
			{
				double itmid = (itq->ask + itq->bid) / 2.0;
				if( itq->msecs >= msecs + 1000 )
					mid_fut = itmid;
				else if( itq->msecs >= msecs )
					mid_fut_1s = itmid;
				else if( itq->msecs <= msecs - 60000 )
				{
					if( itq->msecs > msecs - 360000 )
						mid_1m = itmid;
					else if( itq->msecs <= msecs - 360000 )
					{
						if( itq->msecs > msecs - 960000 )
							mid_5m = itmid;
						else if( itq->msecs <= msecs - 960000 )
							if( itq->msecs > msecs - 1920000 )
								mid_10m = itmid;
					}
				}
			}
			else
				break;
		}

		if( mid_1m <= 0 || mid_5m <= 0 || mid_10m <= 0 || mid_fut <= 0 )
			continue;

		double sprd = (it->ask - it->bid) / mid;
		double ret_1m = 0;
		double ret_5m = 0;
		double ret_10m = 0;
		ret_1m = (mid - mid_1m) / mid_1m;
		ret_5m = (mid - mid_5m) / mid_5m;
		ret_10m = (mid - mid_10m) / mid_10m;

		input[0] = sprd;
		input[1] = it->bidSize;
		input[2] = it->askSize;
		input[3] = ret_1m;
		input[4] = ret_5m;
		input[5] = ret_10m;
		target[0] = (mid_fut - mid) / mid;

		sample.push_back(Bsample(input, target, *it));
		last_msecs = msecs;
	}
			
	return;
}
