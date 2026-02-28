#include <HTickSeries.h>
#include "optionlibs/TickData.h"
#include "HLib/HEvent.h"
#include <HLib/HEnv.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <boost/thread.hpp>
#include <iostream>
#include <algorithm>
#include <map>
#include <numeric>
#include <list>
#include <string>
using namespace std;

HTickSeries::HTickSeries(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
debug_(false),
msecOpen_(0),
msecClose_(0),
minNQuotes_(100),
minNTrades_(100)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";

	if( conf.count("desc") )
		desc_ = conf.find("desc")->second;

	if( conf.count("minNQutoes") )
		minNQuotes_ = atoi( conf.find("minNQutoes")->second.c_str() );

	if( conf.count("minNTrades") )
		minNTrades_ = atoi( conf.find("minNTrades")->second.c_str() );
}

HTickSeries::~HTickSeries()
{}

void HTickSeries::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	if( !desc_.empty() )
		desc_ = (string)"_" + desc_;
	quote_name_ = (string)"quotes" + desc_ + "TS";
	trade_name_ = (string)"trades" + desc_ + "TS";
}

void HTickSeries::beginMarketDay()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	sessions_ = mto::sessions(market, 10, 0, 0, idate);
	msecOpen_ = mto::msecOpen(market, idate);
	msecClose_ = mto::msecClose(market, idate);
}

void HTickSeries::beginTicker(const string& ticker)
{
	boost::mutex::scoped_lock lock(event_mutex_);
	int idate = HEnv::Instance()->idate();

	vector<int> msecT;
	vector<double> prcT;
	vector<int> volT;
	vector<int> msecQ;
	vector<double> midQ;
	vector<double> bidQ;
	vector<double> askQ;
	vector<int> msecQ1s;
	vector<double> midQ1s;
	vector<double> bidQ1s;
	vector<double> askQ1s;
	vector<int> signT;
	vector<double> midT;
	vector<int> msecT1s;
	vector<double> midT1s;
	msecT.reserve(20000);
	prcT.reserve(20000);
	volT.reserve(20000);
	msecQ.reserve(20000);
	midQ.reserve(20000);
	bidQ.reserve(20000);
	askQ.reserve(20000);
	msecQ1s.reserve(20000);
	midQ1s.reserve(20000);
	bidQ1s.reserve(20000);
	askQ1s.reserve(20000);
	signT.reserve(20000);
	midT.reserve(20000);
	msecT1s.reserve(20000);
	midT1s.reserve(20000);

	// Read Trade
	const TickSeries<TradeInfo>* ptsT = static_cast<const TickSeries<TradeInfo>*>(HEvent::Instance()->get(ticker, trade_name_));
	int ntt = ptsT->NTicks();
	if( ntt > minNTrades_ )
	{
		const_cast<TickSeries<TradeInfo>*>(ptsT)->StartRead();
		TradeInfo trade;
		int lastMsecs = 0;
		double sum_prc = 0;
		int sum_vol = 0;
		int n_prc = 0;
		for( int n=0; n<ntt; ++n )
		{
			const_cast<TickSeries<TradeInfo>*>(ptsT)->Read(&trade);
			int ms = trade.msecs;
			bool inrange = find_if(sessions_.begin(), sessions_.end(), bind2nd(inRange(), ms)) != sessions_.end();
			if( inrange )
			{
				if( trade.msecs > lastMsecs )
				{
					bool lastMsecs_inrange = find_if(sessions_.begin(), sessions_.end(),
						bind2nd(inRange(), lastMsecs)) != sessions_.end();
					if( lastMsecs_inrange )
					{
						msecT.push_back(lastMsecs);
						prcT.push_back(sum_prc / n_prc);
						volT.push_back(sum_vol);
					}

					lastMsecs = trade.msecs;
					sum_prc = trade.price;
					sum_vol = trade.qty;
					n_prc = 1;
				}
				else // Add the trades with the same timestamps. Rare.
				{
					lastMsecs = trade.msecs;
					sum_prc += trade.price;
					sum_vol += trade.qty;
					++n_prc;
				}
			}
		}

		msecT.push_back(lastMsecs);
		prcT.push_back(sum_prc / n_prc);
		volT.push_back(sum_vol);
	}

	// Read Quote
	const TickSeries<QuoteInfo>* ptsQ = static_cast<const TickSeries<QuoteInfo>*>(HEvent::Instance()->get(ticker, quote_name_));
	int ntq = ptsQ->NTicks();
	if( ntq > minNQuotes_ )
	{
		int lastMsecs = 0;
		const_cast<TickSeries<QuoteInfo>*>(ptsQ)->StartRead();
		QuoteInfo quote;
		for( int n=0; n<ntq; ++n )
		{
			const_cast<TickSeries<QuoteInfo>*>(ptsQ)->Read(&quote);
			int ms = quote.msecs;
			bool inrange = find_if(sessions_.begin(), sessions_.end(), bind2nd(inRange(), ms)) != sessions_.end();
			if( inrange )
			{
				double bid = quote.bid;
				double ask = quote.ask;
				if( bid < ask )
				{
					double mid = (ask + bid)/2.0;
					double sprd = (ask - bid)/mid;
					if( fabs(sprd) < 0.05 )
					{
						msecQ.push_back(quote.msecs);
						midQ.push_back(mid);
						bidQ.push_back(bid);
						askQ.push_back(ask);
					}
				}
			}
		}
	}
	int NQ = msecQ.size();

	// Fill 1 sec quote price series
	if( NQ > 0 )
	{
		int sec1Q = msecQ[0]/1000 + 1;
		int sec2Q = msecQ[NQ-1]/1000;
		if( sec1Q <= sec2Q )
		{
			msecQ1s = vector<int>(sec2Q - sec1Q + 1, 0);
			midQ1s = vector<double>(sec2Q - sec1Q + 1, 0);
			bidQ1s = vector<double>(sec2Q - sec1Q + 1, 0);
			askQ1s = vector<double>(sec2Q - sec1Q + 1, 0);
			{
				int iQ = NQ - 1;
				for( int sec = sec2Q; sec >= sec1Q; --sec )
				{
					msecQ1s[sec - sec1Q] = sec*1000;
					while( iQ > 0 && msecQ[iQ]/1000 >= sec )
						--iQ;

					if( msecQ[iQ]/1000 < sec )
					{
						double bid = bidQ[iQ];
						double ask = askQ[iQ];
						double mid = (ask + bid)/2.0;
						int index = sec - sec1Q;
						midQ1s[index] = mid;
						bidQ1s[index] = bid;
						askQ1s[index] = ask;

						if( iQ + 1 < NQ )
							++iQ;
					}
				}
			}
		}

		// Replace zeros.
		//replace_zeros(midQ);
		replace_zeros(midQ1s);
	}

	// Fill signT and midT.
	int NT = msecT.size();
	//int NQ = msecQ.size();
	if( NT > minNTrades_ && NQ > minNQuotes_ )
	{
		signT = vector<int>(NT, 0);
		midT = vector<double>(NT, 0.);
		{
			int iQ = NQ - 1;
			for( int iT=NT-1; iT >= 0; --iT )
			{
				while( iQ > 0 && msecQ[iQ] >= msecT[iT] )
					--iQ;

				if( msecQ[iQ] < msecT[iT] )
				{
					double tPrc = prcT[iT];
					double bid = bidQ[iQ];
					double ask = askQ[iQ];
					double mid = (ask + bid)/2.0;

					if( fabs(tPrc-ask) < 1e-6 )
						signT[iT] = 1;
					else if( fabs(tPrc-bid) < 1e-6 )
						signT[iT] = -1;

					midT[iT] = mid;

					// Rewind one step.
					if( iQ + 1 < NQ )
						++iQ;
				}
			}
		}

		// Fill 1 sec trade price series
		if( NT > 0 )
		{
			int sec1T = msecT[0]/1000 + 1;
			int sec2T = msecT[NT-1]/1000;
			int iT = NT - 1;
			if( sec1T <= sec2T )
			{
				msecT1s = vector<int>(sec2T - sec1T + 1, 0);
				midT1s = vector<double>(sec2T - sec1T + 1, 0.);
				{
					for( int sec = sec2T; sec >= sec1T; --sec )
					{
						msecT1s[sec - sec1T] = sec*1000;
						while( iT > 0 && msecT[iT]/1000 >= sec )
							--iT;

						if( msecT[iT]/1000 < sec )
						{
							int index = sec - sec1T;
							midT1s[index] = midT[iT];

							if( iT + 1 < NT )
								++iT;
						}
					}
				}
			}
		}

		// Replace zeros in midT.
		replace_zeros(midT);
		replace_zeros(midT1s);
	}

	// Add to the event
	HEvent::Instance()->add<vector<int> >(ticker, "msecT" + desc_, msecT);
	HEvent::Instance()->add<vector<double> >(ticker, "prcT" + desc_, prcT);
	HEvent::Instance()->add<vector<int> >(ticker, "volT" + desc_, volT);
	HEvent::Instance()->add<vector<int> >(ticker, "msecQ" + desc_, msecQ);
	HEvent::Instance()->add<vector<double> >(ticker, "midQ" + desc_, midQ);
	HEvent::Instance()->add<vector<double> >(ticker, "bidQ" + desc_, bidQ);
	HEvent::Instance()->add<vector<double> >(ticker, "askQ" + desc_, askQ);
	HEvent::Instance()->add<vector<int> >(ticker, "msecQ1s" + desc_, msecQ1s);
	HEvent::Instance()->add<vector<double> >(ticker, "midQ1s" + desc_, midQ1s);
	HEvent::Instance()->add<vector<double> >(ticker, "bidQ1s" + desc_, bidQ1s);
	HEvent::Instance()->add<vector<double> >(ticker, "askQ1s" + desc_, askQ1s);
	HEvent::Instance()->add<vector<int> >(ticker, "signT" + desc_, signT);
	HEvent::Instance()->add<vector<double> >(ticker, "midT" + desc_, midT);
	HEvent::Instance()->add<vector<int> >(ticker, "msecT1s" + desc_, msecT1s);
	HEvent::Instance()->add<vector<double> >(ticker, "midT1s" + desc_, midT1s);
}

void HTickSeries::endTicker(const string& ticker)
{
	// Remove from the event
	boost::mutex::scoped_lock lock(event_mutex_);
	HEvent::Instance()->remove<vector<int> >(ticker, "msecT" + desc_);
	HEvent::Instance()->remove<vector<double> >(ticker, "prcT" + desc_);
	HEvent::Instance()->remove<vector<int> >(ticker, "volT" + desc_);
	HEvent::Instance()->remove<vector<int> >(ticker, "msecQ" + desc_);
	HEvent::Instance()->remove<vector<double> >(ticker, "midQ" + desc_);
	HEvent::Instance()->remove<vector<double> >(ticker, "bidQ" + desc_);
	HEvent::Instance()->remove<vector<double> >(ticker, "askQ" + desc_);
	HEvent::Instance()->remove<vector<int> >(ticker, "msecQ1s" + desc_);
	HEvent::Instance()->remove<vector<double> >(ticker, "midQ1s" + desc_);
	HEvent::Instance()->remove<vector<double> >(ticker, "bidQ1s" + desc_);
	HEvent::Instance()->remove<vector<double> >(ticker, "askQ1s" + desc_);
	HEvent::Instance()->remove<vector<int> >(ticker, "signT" + desc_);
	HEvent::Instance()->remove<vector<double> >(ticker, "midT" + desc_);
	HEvent::Instance()->remove<vector<int> >(ticker, "msecT1s" + desc_);
	HEvent::Instance()->remove<vector<double> >(ticker, "midT1s" + desc_);
}

void HTickSeries::endMarketDay()
{
}

void HTickSeries::endJob()
{
}

void HTickSeries::replace_zeros(vector<double>& series)
{
	double last = 0;
	int N = series.size();
	for( int i=0; i<N; ++i )
	{
		if( series[i] < 1e-6 )
			series[i] = last;

		if( series[i] > 1e-6 )
			last = series[i];
	}

	int izero = 0;
	for( int i=0; i<N; ++i )
	{
		double prc = series[i];
		if( prc > 1e-6 )
		{
			for( int j=0; j<izero; ++j )
				series[j] = prc;
			break;
		}
		else
			++izero;
	}

	for( int i=0; i<N; ++i )
	{
		double prc = series[i];
		if( prc < 1e-6 )
		{
			cout << i << "th price " << prc << " out of range" << endl;
			exit(4);
		}
	}
}
