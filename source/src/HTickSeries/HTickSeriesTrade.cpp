#include <HTickSeries.h>
#include "optionlibs/TickData.h"
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib.h>
//#include <jl_lib/mto.h>
#include <boost/thread.hpp>
#include <iostream>
#include <algorithm>
#include <map>
#include <numeric>
#include <list>
#include <string>
using namespace std;

HTickSeriesTrade::HTickSeriesTrade(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
debug_(false),
msecOpen_(0),
msecClose_(0),
source_(""),
minNTrades_(100),
minNQuotes_(100)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";

	if( conf.count("minNTrades") )
		minNTrades_ = atoi( conf.find("minNTrades")->second.c_str() );

	if( conf.count("source") )
		source_ = conf.find("source")->second;

	if( !source_.empty() )
		source_ext_ = (string)"_" + source_;
}

HTickSeriesTrade::~HTickSeriesTrade()
{}

void HTickSeriesTrade::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	return;
}

void HTickSeriesTrade::beginMarketDay()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	sessions_ = mto::sessions(market, 10, 0, 0, idate);
	msecOpen_ = mto::msecOpen(market, idate);
	msecClose_ = mto::msecClose(market, idate);

	return;
}

void HTickSeriesTrade::beginTicker(const string& ticker)
{
	int idate = HEnv::Instance()->idate();

	vector<double> msecT;
	vector<double> prcT;
	vector<double> volT;
	vector<double> signT;
	vector<double> midT;
	vector<double> msecT1s;
	vector<double> midT1s;

	// Read Trade
	const TickSeries<TradeInfo>* ptsT = static_cast<const TickSeries<TradeInfo>*>(HEvent::Instance()->get(ticker, "trades"));
	int ntt = ptsT->NTicks();
	if( ntt > minNTrades_ )
	{
		const_cast<TickSeries<TradeInfo>*>(ptsT)->StartRead();
		TradeInfo trade;
		int lastMsecs = 0;
		double sum_prc = 0;
		double sum_vol = 0;
		int n_prc = 0;
		for( int n=0; n<ntt; ++n )
		{
			const_cast<TickSeries<TradeInfo>*>(ptsT)->Read(&trade);

			int ms = trade.msecs;
			bool inrange = find_if(sessions_.begin(), sessions_.end(), bind2nd(inRange(), ms)) != sessions_.end();

			bool goodSource = source_.empty() || flag2source(trade.flags) == source_;

			if( inrange && goodSource )
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

		if( n_prc > 0.5 )
		{
			msecT.push_back(lastMsecs);
			prcT.push_back(sum_prc / n_prc);
			volT.push_back(sum_vol);
		}
	}

	// Read Quote
	const vector<double>& msecQ = *static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "msecQ"));
	const vector<double>& midQ = *static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "midQ"));
	const vector<double>& bidQ = *static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "bidQ"));
	const vector<double>& askQ = *static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "askQ"));
	const vector<double>& msecQ1s = *static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "msecQ1s"));
	const vector<double>& midQ1s = *static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "midQ1s"));
	const vector<double>& bidQ1s = *static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "bidQ1s"));
	const vector<double>& askQ1s = *static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "askQ1s"));

	// Fill signT and midT.
	int NT = msecT.size();
	int NQ = msecQ.size();
	if( NT > minNTrades_ && NQ > minNQuotes_ )
	{
		signT = vector<double>(NT, 0);
		midT = vector<double>(NT, 0);
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
			int sec1T = msecOpen_;
			int sec2T = msecClose_;
			int NT1s = sec2T - sec1T + 1;

			msecT1s = vector<double>(NT1s, 0);
			midT1s = vector<double>(NT1s, 0);

			for( int sec = sec1T; sec <= sec2T; ++sec )
				msecT1s[sec - sec1T] = sec * 1000;

			for( int i=0; i<NT; ++i )
			{
				int msecs = msecT[i];
				int index1s = (msecs - sec1T*1000 <= 0)?0:( msecs - sec1T*1000 -1 ) / 1000 + 1;
				midT1s[index1s] = midT[i];
			}

/*
			int sec1T = msecT[0]/1000 + 1;
			int sec2T = msecT[NT-1]/1000;
			int iT = NT - 1;
			if( sec1T <= sec2T )
			{
				msecT1s = vector<double>(sec2T - sec1T + 1, 0);
				midT1s = vector<double>(sec2T - sec1T + 1, 0);
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
			}*/
		}

		// Replace zeros in midT.
		replace_zeros(midT);
		replace_zeros(midT1s);
	}

	// Add to the event
	HEvent::Instance()->add<vector<double> >(ticker, (string)"msecT" + source_ext_, msecT);
	HEvent::Instance()->add<vector<double> >(ticker, (string)"prcT" + source_ext_, prcT);
	HEvent::Instance()->add<vector<double> >(ticker, (string)"volT" + source_ext_, volT);
	HEvent::Instance()->add<vector<double> >(ticker, (string)"signT" + source_ext_, signT);
	HEvent::Instance()->add<vector<double> >(ticker, (string)"midT" + source_ext_, midT);
	HEvent::Instance()->add<vector<double> >(ticker, (string)"msecT1s" + source_ext_, msecT1s);
	HEvent::Instance()->add<vector<double> >(ticker, (string)"midT1s" + source_ext_, midT1s);

	return;
}

void HTickSeriesTrade::endTicker(const string& ticker)
{
	// Remove from the event
	HEvent::Instance()->remove<vector<double> >(ticker, (string)"msecT" + source_ext_);
	HEvent::Instance()->remove<vector<double> >(ticker, (string)"prcT" + source_ext_);
	HEvent::Instance()->remove<vector<double> >(ticker, (string)"volT" + source_ext_);
	HEvent::Instance()->remove<vector<double> >(ticker, (string)"signT" + source_ext_);
	HEvent::Instance()->remove<vector<double> >(ticker, (string)"midT" + source_ext_);
	HEvent::Instance()->remove<vector<double> >(ticker, (string)"msecT1s" + source_ext_);
	HEvent::Instance()->remove<vector<double> >(ticker, (string)"midT1s" + source_ext_);
	return;
}

void HTickSeriesTrade::endMarketDay()
{
	return;
}

void HTickSeriesTrade::endJob()
{
	return;
}

void HTickSeriesTrade::replace_zeros(vector<double>& series)
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
	return;
}

string HTickSeriesTrade::flag2source(char c)
{
	if( c == 1 )
		return "N";
	else if( c == 2 )
		return "Q";
	else if( c == 3 )
		return "P";
	else if( c == 4 )
		return "C";
	else if( c == 6 )
		return "A";
	else if( c == 7 )
		return "M";
	else if( c == 11 )
		return "X";
	else if( c == 13 )
		return "B";
	else if( c == 138 )
		return "D";
	else if( c == 17 )
		return "W";
	else if( c == 23 )
		return "T";
	else if( c == 55 )
		return "U";
	else if( c == 175 )
		return "I";
	else if( c == 194 )
		return "Z";
	return "";
}
