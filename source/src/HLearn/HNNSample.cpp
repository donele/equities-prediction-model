#include <HLearn/HNNSample.h>
#include <HLearnObj/Sample.h>
#include <HLearnObj/InputInfo.h>
#include <optionlibs/TickData.h>
#include <HLib.h>
#include <jl_lib.h>
#include "TFile.h"
#include <algorithm>
#include <functional>
#include <vector>
#include <fstream>
using namespace std;

HNNSample::HNNSample(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
nInput_(0),
targetTime_(60),
sampleInterval_(60),
//adjust_market_(true),
hedge_inputs_(false),
hedge_target_(true),
log_share_(true),
lotSize_(1),
input_bidSize_(false),
input_askSize_(false),
input_spread_(false),
input_quoteImb_(false),
input_volume_(false),
input_pred1m_(false),
input_pred1m_log_(false),
input_pred40m_(false),
input_pred40m_log_(false),
input_pred41m_(false)
{
	if( conf.count("targetTime") )
		targetTime_ = atoi( conf.find("targetTime")->second.c_str() );
	if( conf.count("sampleInterval") )
		sampleInterval_ = atoi( conf.find("sampleInterval")->second.c_str() );
	//if( conf.count("adjustMarket") )
	//	adjust_market_ = conf.find("adjustMarket")->second == "true";
	if( conf.count("hedgeInputs") )
		hedge_inputs_ = conf.find("hedgeInputs")->second == "true";
	if( conf.count("hedgeTarget") )
		hedge_target_ = conf.find("hedgeTarget")->second == "true";
	if( conf.count("logShare") )
		log_share_ = conf.find("logShare")->second == "true";
	if( conf.count("sampleDir") )
		sample_dir_ = conf.find("sampleDir")->second;

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
		sprintf( iname, "rtn_%ds", *it );
		inputNames_.push_back(iname);
	}
	nPastRtn_ = pastRtnTime_.size();
	if( conf.count("input") )
	{
		pair<mmit, mmit> values = conf.equal_range("input");
		for( mmit mi = values.first; mi != values.second; ++mi )
		{
			//++nInput_;
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
			else if( "pred1m" == name )
			{
				input_pred1m_ = true;
				inputNames_.push_back("pred1m");
			}
			else if( "pred1mLog" == name )
			{
				input_pred1m_log_ = true;
				inputNames_.push_back("pred1mLogPos");
				inputNames_.push_back("pred1mLogNeg");
			}
			else if( "pred40m" == name )
			{
				input_pred40m_ = true;
				inputNames_.push_back("pred40m");
			}
			else if( "pred40mLog" == name )
			{
				input_pred40m_log_ = true;
				inputNames_.push_back("pred40mLogPos");
				inputNames_.push_back("pred40mLogNeg");
			}
			else if( "pred41m" == name )
			{
				input_pred41m_ = true;
				inputNames_.push_back("pred41m");
			}
		}
	}
	nInput_ = inputNames_.size();

	if( HEnv::Instance()->multiThreadTicker() )
	{
		cerr << "HNNSample:Error multiThreadTicker is not supported." << endl;
		exit(5);
	}
}

HNNSample::~HNNSample()
{}

void HNNSample::beginJob()
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

void HNNSample::beginMarket()
{
	string market = HEnv::Instance()->market();
	if( market == "US" )
		lotSize_ = 100;

	GTH::Instance()->init(market);

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

	if( "CJ" == market )
		pred_dir_ = "\\\\smrc-ltc-mrct46\\f\\dump\\dumpPredsJedong\\TSX";
	else if( "EL" == market )
		pred_dir_ = "\\\\smrc-ltc-mrct46\\f\\dump\\dumpPredsJedong\\LSE";

	return;
}

void HNNSample::beginDay()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	sessions_ = mto::sessions(market, 30, 0, 10, idate);
	msecOpen_ = mto::msecOpen(market, idate);
	msecClose_ = mto::msecClose(market, idate);
	vector<string> tickers = HEnv::Instance()->tickers();

	// Read volume from stockCharacteristics table.
	if( input_volume_ )
	{
		mVol_.clear();
		string market = HEnv::Instance()->market();
		int idate = HEnv::Instance()->idate();
		int idate_prev = (int)GEX::Instance()->get(market)->PrevClose(QuoteTime(idate, 040000, mto::tz(market))).Date();

		string cmd = (string)"select " + mto::ts(market) + ", medVolume "
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
	//if( adjust_market_ )
	{
		const TickSeries<ReturnData>* ptsR = static_cast<const TickSeries<ReturnData>*>(HEvent::Instance()->get(mto::retName(market), "returns"));
		//int nSecInterval = (msecClose_ - msecOpen_)/1000 + 1;
		nSecInterval_ = (msecClose_ - msecOpen_)/1000 + 1;
		secRetCum_ = vector<double>(nSecInterval_, 0);
		vector<double> secRet = vector<double>(nSecInterval_, 0);

		int ntr = ptsR->NTicks();
		const_cast<TickSeries<ReturnData>*>(ptsR)->StartRead();
		ReturnData r;
		for( int n=0; n<ntr; ++n )
		{
			const_cast<TickSeries<ReturnData>*>(ptsR)->Read(&r);
			int rIndx = (r.msecs - msecOpen_)/1000;
			secRet[rIndx] = r.ret;
		}

		secRetCum_[0] = 1.0;
		for( int n=1; n<nSecInterval_; ++n )
			secRetCum_[n] = secRetCum_[n-1] * secRet[n] + secRetCum_[n-1];
	}

	if( !pred_dir_.empty() &&
		(
		find(inputNames_.begin(), inputNames_.end(), "pred1m") != inputNames_.end()
		|| find(inputNames_.begin(), inputNames_.end(), "pred1mLogPos") != inputNames_.end()
		|| find(inputNames_.begin(), inputNames_.end(), "pred1mLogNeg") != inputNames_.end()
		|| find(inputNames_.begin(), inputNames_.end(), "pred40m") != inputNames_.end()
		|| find(inputNames_.begin(), inputNames_.end(), "pred40mLogPos") != inputNames_.end()
		|| find(inputNames_.begin(), inputNames_.end(), "pred40mLogNeg") != inputNames_.end()
		|| find(inputNames_.begin(), inputNames_.end(), "pred41m") != inputNames_.end()
		) )
		preds_ = Preds(pred_dir_, market, idate);

	// Open file to write the sample.
	char filename[400];
	sprintf(filename, "%s\\%d.bin", sample_dir_.c_str(), idate);
	ofs_.open( filename, ios::out | ios::binary );

	// Write SampleSummary.
	hnn::SampleSummary ss(inputNames_, tickers);
	ofs_ << ss;

	return;
}

void HNNSample::beginTicker(const string& ticker)
{
	// Access tick data.
	const TickSeries<QuoteInfo>* ptsQ = static_cast<const TickSeries<QuoteInfo>*>(HEvent::Instance()->get(ticker, "quotes"));

	// Make sample.
	vector<hnn::Sample> sample;
	get_sample(ticker, ptsQ, sample);

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

void HNNSample::endTicker(const string& ticker)
{
	return;
}

void HNNSample::endDay()
{
	ofs_.close();
	ofs_.clear();
	return;
}

void HNNSample::endMarket()
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

void HNNSample::endJob()
{
	return;
}


void HNNSample::get_sample(string ticker, const TickSeries<QuoteInfo>* ptsQ, vector<hnn::Sample>& sample)
{
	vector<QuoteInfo> vq;
	get_quotes(ptsQ, vq); // Read tick data.
	get_sample(ticker, vq, sample); // Make sample.

	return;
}

void HNNSample::get_quotes(const TickSeries<QuoteInfo>* ptsQ, vector<QuoteInfo>& vq)
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

void HNNSample::get_sample(string ticker, vector<QuoteInfo>& vq, vector<hnn::Sample>& sample)
{
	double last_msecs = 0;
	for( vector<QuoteInfo>::iterator it = vq.begin(); it != vq.end(); ++it )
	{
		int msecs = it->msecs;
		if( last_msecs > 0 && msecs - last_msecs < sampleInterval_ * 1000 ) // time between samples.
			continue;

		double mid = (it->ask + it->bid) / 2.0; // Valid spread.
		if( mid <= 0 )
			continue;
		//double mid = midRaw;
		double midHedged = mid / secRetCum_[ (msecs - msecOpen_) / 1000 ];
		//if( adjust_market_ )
		//	mid = midRaw / secRetCum_[(msecs-msecOpen_)/1000];

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
			m_msecs_mid[msecs - cumPastRtnTime*1000] = 0;
		}
		m_msecs_mid[min(msecs + targetTime_ * 1000, msecClose_)] = 0;

		for( vector<QuoteInfo>::iterator itq = itqb; itq != itqe; ++itq )
		{
			if( itq->msecs <= msecs + targetTime_ * 1000 )
			{
				double itqmid = (itq->ask + itq->bid) / 2.0;
				//if( (hedge_inputs_ && itq->msecs <= msecs) // past returns.
				//	|| (hedge_target_ && itq->msecs > msecs) )
				//	itqmid = itqmid / secRetCum_[(itq->msecs - msecOpen_)/1000];
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

		vector<double> vCumRet;
		for( map<int, double>::iterator it2 = m_msecs_mid.begin(); it2 != m_msecs_mid.end(); ++it2 )
		{
			int sec = it2->first / 1000;
			int indx = sec - msecOpen_ / 1000;
			vCumRet.push_back(secRetCum_[indx]);
		}

		// Past returns
		vector<double> vPastRet;
		for( int i = 0; i < nPastRtn_; ++i )
		{
			double pastRet = 0;
			double pastPrc = vPrc[i];
			double cumRet = vCumRet[i];
			if(	pastPrc > 0 )
			{
				if( hedge_inputs_ )
					pastRet = (midHedged - (pastPrc / cumRet)) / (pastPrc / cumRet);
				else
					pastRet = (mid - pastPrc) / pastPrc;
			}
			vPastRet.push_back(pastRet);
		}

		// Input.
		bool predFail = false;
		vector<float> input;
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
			double sprd = (it->ask - it->bid) / mid * basis_pts_;
			input.push_back(sprd);
		}
		if( input_quoteImb_ )
		{
			double quoteImb = 2.0 * (it->askSize - it->bidSize) / (it->askSize + it->bidSize);
			input.push_back(quoteImb);
		}
		if( input_volume_ )
		{
			double volume = 0;
			if( mVol_.count(ticker) )
				volume = log(mVol_[ticker]);
			if( volume < 0 )
				volume = 0;
			input.push_back(volume);
		}
		if( input_pred1m_ )
		{
			double pred1m = preds_.get_pred1m(ticker, msecs);
			if( fabs(pred1m) < 1e3 )
				input.push_back(pred1m);
			else
				predFail = true;
		}
		if( input_pred1m_log_ )
		{
			double pred1m = preds_.get_pred1m(ticker, msecs);
			if( fabs(pred1m) < 1e3 )
			{
				double pred1mLogPos = 0;
				double pred1mLogNeg = 0;
				if( pred1m > 0 )
					pred1mLogPos = log(pred1m);
				else if( pred1m < 0 )
					pred1mLogNeg = log(-pred1m);
				input.push_back(pred1mLogPos);
				input.push_back(pred1mLogNeg);
			}
			else
				predFail = true;
		}
		if( input_pred40m_ )
		{
			double pred40m = preds_.get_pred40m(ticker, msecs);
			if( fabs(pred40m) < 1e3 )
				input.push_back(pred40m);
			else
				predFail = true;
		}
		if( input_pred40m_log_ )
		{
			double pred40m = preds_.get_pred40m(ticker, msecs);
			if( fabs(pred40m) < 1e3 )
			{
				double pred40mLogPos = 0;
				double pred40mLogNeg = 0;
				if( pred40m > 0 )
					pred40mLogPos = log(pred40m);
				else if( pred40m < 0 )
					pred40mLogNeg = log(-pred40m);
				input.push_back(pred40mLogPos);
				input.push_back(pred40mLogNeg);
			}
			else
				predFail = true;
		}
		if( input_pred41m_ )
		{
			double pred41m = preds_.get_pred1m(ticker, msecs) + preds_.get_pred40m(ticker, msecs);
			if( fabs(pred41m) < 1e3 )
				input.push_back(pred41m);
			else
				predFail = true;
		}

		if( !predFail )
		{
			// Target
			float target = 0;
			double targetPrc = vPrc[nPastRtn_];
			double cumRet = vCumRet[nPastRtn_];

			if( hedge_target_ )
				target = (targetPrc / cumRet - midHedged) / midHedged;
			else
				target = (targetPrc - mid) / mid;

			//if( hedge_target_ )
			//	targetPrc /= vCumRet[nPastRtn_];
			//float target = (targetPrc - mid) / mid;

			sample.push_back(hnn::Sample(input, target, *it));
			last_msecs = msecs;
		}
	}
			
	return;
}

HNNSample::Preds::Preds()
{}

HNNSample::Preds::Preds(string pred_dir, string market, int idate)
:market_(market),
idate_(idate),
N_((mto::msecClose(market, idate) - mto::msecOpen(market, idate))/1000)
{
	char path[200];
	sprintf(path, "%s\\%d.txt", pred_dir.c_str(), idate);
	ifstream ifs(path);

	vector<double> vPred1m(N_, 0);
	vector<double> vPred40m(N_, 0);

	string uid = "";
	string line = "";
	while( !ifs.eof() )
	{
		getline(ifs, line);
		vector<string> v = split(line);
		if( v.size() == 6 )
		{
			if( uid != v[0] )
			{
				if( !uid.empty() )
					flush_pred(uid, vPred1m, vPred40m);
				uid = v[0];
			}
			add_line(v, vPred1m, vPred40m);
		}
	}
}

void HNNSample::Preds::flush_pred(string uid, vector<double>& vPred1m, vector<double>& vPred40m)
{
	string ticker = mto::compTicker(GTH::Instance()->get(market_)->UniqueToTicker(uid, idate_), market_);
	mTickerPred1m_[ticker] = vPred1m;
	mTickerPred40m_[ticker] = vPred40m;
	vPred1m = vector<double>(N_, 0);
	vPred40m = vector<double>(N_, 0);
	return;
}

void HNNSample::Preds::add_line(vector<string>& v, vector<double>& vPred1m, vector<double>& vPred40m)
{
	int n = atoi(v[1].c_str()) / 1000 / 60 - 1;
	if( n >= 0 && n < N_ )
	{
		double pred1m = atof(v[2].c_str());
		double pred40m = atof(v[4].c_str());
		vPred1m[n] = pred1m;
		vPred40m[n] = pred40m;
	}
	return;
}

double HNNSample::Preds::get_pred1m(std::string ticker, int msecs)
{
	return get_pred(mTickerPred1m_, ticker, msecs);
}

double HNNSample::Preds::get_pred40m(std::string ticker, int msecs)
{
	return get_pred(mTickerPred40m_, ticker, msecs);
}

double HNNSample::Preds::get_pred41m(std::string ticker, int msecs)
{
	return get_pred(mTickerPred1m_, ticker, msecs) + get_pred(mTickerPred40m_, ticker, msecs);
}

double HNNSample::Preds::get_pred(std::map<std::string, std::vector<double> >& m, std::string ticker, int msecs)
{
	double ret = 1e6;
	int n = ( msecs - mto::msecOpen(market_, idate_) ) / 1000 /60 - 1;
	map<string, vector<double> >::iterator it = m.find(ticker);
	if( it != m.end() )
		ret = it->second[n];

	return ret;
}

