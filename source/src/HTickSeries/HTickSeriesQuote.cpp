#include <HTickSeries.h>
#include <optionlibs/TickData.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib.h>
#include <boost/thread.hpp>
#include <iostream>
#include <algorithm>
#include <map>
#include <numeric>
#include <list>
#include <string>
using namespace std;

HTickSeriesQuote::HTickSeriesQuote(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
debug_(false),
msecOpen_(0),
msecClose_(0),
minNQuotes_(100)
{
	if( conf.count("debug") )
		if( conf.find("debug")->second == "true" )
			debug_ = true;

	if( conf.count("minNQutoes") )
		minNQuotes_ = atoi( conf.find("minNQutoes")->second.c_str() );
}

HTickSeriesQuote::~HTickSeriesQuote()
{}

void HTickSeriesQuote::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	return;
}

void HTickSeriesQuote::beginMarketDay()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	sessions_ = mto::sessions(market, 0, 0, 0, idate);
	msecOpen_ = mto::msecOpen(market, idate);
	msecClose_ = mto::msecClose(market, idate);

	return;
}

void HTickSeriesQuote::beginTicker(const string& ticker)
{
	int idate = HEnv::Instance()->idate();

	vector<double> msecQ;
	vector<double> midQ;
	vector<double> bidQ;
	vector<double> askQ;
	vector<double> msecQ1s;
	vector<double> midQ1s;
	vector<double> bidQ1s;
	vector<double> askQ1s;

	// Read Quote
	const TickSeries<QuoteInfo>* ptsQ = static_cast<const TickSeries<QuoteInfo>*>(HEvent::Instance()->get(ticker, "quotes"));

	if( ptsQ->NTicks() > minNQuotes_ )
		get_quote_series( msecQ, midQ, bidQ, askQ, ptsQ, sessions_);

	get_quote_1s_series( msecQ1s, midQ1s, bidQ1s, askQ1s,
						msecQ, midQ, bidQ, askQ);

	// Add to the event
	HEvent::Instance()->add<vector<double> >(ticker, "msecQ", msecQ);
	HEvent::Instance()->add<vector<double> >(ticker, "midQ", midQ);
	HEvent::Instance()->add<vector<double> >(ticker, "bidQ", bidQ);
	HEvent::Instance()->add<vector<double> >(ticker, "askQ", askQ);
	HEvent::Instance()->add<vector<double> >(ticker, "msecQ1s", msecQ1s);
	HEvent::Instance()->add<vector<double> >(ticker, "midQ1s", midQ1s);
	HEvent::Instance()->add<vector<double> >(ticker, "bidQ1s", bidQ1s);
	HEvent::Instance()->add<vector<double> >(ticker, "askQ1s", askQ1s);

	return;
}

void HTickSeriesQuote::endTicker(const string& ticker)
{
	// Remove from the event
	HEvent::Instance()->remove<vector<double> >(ticker, "msecQ");
	HEvent::Instance()->remove<vector<double> >(ticker, "midQ");
	HEvent::Instance()->remove<vector<double> >(ticker, "bidQ");
	HEvent::Instance()->remove<vector<double> >(ticker, "askQ");
	HEvent::Instance()->remove<vector<double> >(ticker, "msecQ1s");
	HEvent::Instance()->remove<vector<double> >(ticker, "midQ1s");
	HEvent::Instance()->remove<vector<double> >(ticker, "bidQ1s");
	HEvent::Instance()->remove<vector<double> >(ticker, "askQ1s");
	return;
}

void HTickSeriesQuote::endMarketDay()
{
	return;
}

void HTickSeriesQuote::endJob()
{
	return;
}

void HTickSeriesQuote::replace_zeros(vector<double>& series)
{
	double last = 0;
	int N = series.size();
	for( int i = 0; i < N; ++i )
	{
		if( series[i] < 1e-6 )
			series[i] = last;

		if( series[i] > 1e-6 )
			last = series[i];
	}

	int izero = 0;
	for( int i = 0; i < N; ++i )
	{
		double prc = series[i];
		if( prc > 1e-6 )
		{
			for( int j = 0; j < izero; ++j )
				series[j] = prc;
			break;
		}
		else
			++izero;
	}

	for( int i = 0; i < N; ++i )
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

void HTickSeriesQuote::get_quote_series( vector<double>& msecQ, vector<double>& midQ,
										vector<double>& bidQ, vector<double>& askQ,
										const TickSeries<QuoteInfo>* ptsQ, vector<pair<int, int> >& sessions)
{
	int ntq = ptsQ->NTicks();
	if( ntq > minNQuotes_ )
	{
		int lastMsecs = 0;
		const_cast<TickSeries<QuoteInfo>*>(ptsQ)->StartRead();
		QuoteInfo quote;
		for( int n = 0; n < ntq; ++n )
		{
			const_cast<TickSeries<QuoteInfo>*>(ptsQ)->Read(&quote);
			int ms = quote.msecs;
			bool inrange = find_if(sessions.begin(), sessions.end(), bind2nd(inRange(), ms)) != sessions.end();
			if( inrange )
			{
				double bid = quote.bid;
				double ask = quote.ask;
				if( bid < ask )
				{
					double mid = (ask + bid) / 2.0;
					double sprd = (ask - bid) / mid;
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
	return;
}

void HTickSeriesQuote::get_quote_1s_series( vector<double>& msecQ1s, vector<double>& midQ1s,
			vector<double>& bidQ1s, vector<double>& askQ1s,
			vector<double>& msecQ, vector<double>& midQ, vector<double>& bidQ, vector<double>& askQ)
{
	int NQ = msecQ.size();

	// Fill the 1 sec quote price series.
	if( NQ > 0 )
	{
		int sec1Q = msecOpen_ / 1000;
		int sec2Q = msecClose_ / 1000;
		int NQ1s = sec2Q - sec1Q + 1;

		msecQ1s = vector<double>(NQ1s, 0);
		midQ1s = vector<double>(NQ1s, 0);
		bidQ1s = vector<double>(NQ1s, 0);
		askQ1s = vector<double>(NQ1s, 0);

		for( int sec = sec1Q; sec <= sec2Q; ++sec )
			msecQ1s[sec - sec1Q] = sec * 1000;

		for( int i = 0; i < NQ; ++i )
		{
			int msecs = msecQ[i];
			//int index1s = (msecs - sec1Q * 1000 <= 0) ? 0: ( msecs - sec1Q * 1000 - 1 ) / 1000 + 1;
			int index1s = (msecs - sec1Q * 1000) / 1000 + 1;
			if( index1s >= 0 )
			{
				midQ1s[index1s] = (askQ[i] + bidQ[i]) / 2.0;
				bidQ1s[index1s] = bidQ[i];
				askQ1s[index1s] = askQ[i];
			}
		}

		replace_zeros(midQ1s);
		replace_zeros(bidQ1s);
		replace_zeros(askQ1s);
	}
	return;
}
