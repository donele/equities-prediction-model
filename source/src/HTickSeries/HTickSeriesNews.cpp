#include <HTickSeries.h>
#include "optionlibs/TickData.h"
#include "HLib/HEvent.h"
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

HTickSeriesNews::HTickSeriesNews(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
debug_(false),
include_overnight_(false),
msecOpen_(0),
msecClose_(0),
minNQuotes_(1),
min_rel_(75),
rel_weight_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";

	if( conf.count("minRel") )
		min_rel_ = atoi( conf.find("minRel")->second.c_str() );

	if( conf.count("relWeight") )
		rel_weight_ = conf.find("relWeight")->second == "true";

	if( conf.count("minNQutoes") )
		minNQuotes_ = atoi( conf.find("minNQutoes")->second.c_str() );
}

HTickSeriesNews::~HTickSeriesNews()
{}

void HTickSeriesNews::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void HTickSeriesNews::beginMarketDay()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	sessions_ = mto::sessions(market, 10, 0, 0, idate);
	msecOpen_ = mto::msecOpen(market, idate);
	msecClose_ = mto::msecClose(market, idate);
}

void HTickSeriesNews::beginTicker(const string& ticker)
{
	int idate = HEnv::Instance()->idate();

	vector<double> msecN;
	vector<double> valN;
	vector<double> msecN1s;
	vector<double> valN1s;

	// Read Quote
	const TickSeries<QuoteInfo>* ptsN = static_cast<const TickSeries<QuoteInfo>*>(HEvent::Instance()->get(ticker, "news"));

	get_quote_series( msecN, valN, ptsN, sessions_);

	get_quote_1s_series( msecN1s, valN1s, msecN, valN);

	// Add to the event
	HEvent::Instance()->add<vector<double> >(ticker, "msecN", msecN);
	HEvent::Instance()->add<vector<double> >(ticker, "valN", valN);
	HEvent::Instance()->add<vector<double> >(ticker, "msecN1s", msecN1s);
	HEvent::Instance()->add<vector<double> >(ticker, "valN1s", valN1s);
}

void HTickSeriesNews::endTicker(const string& ticker)
{
	// Remove from the event
	HEvent::Instance()->remove<vector<double> >(ticker, "msecN");
	HEvent::Instance()->remove<vector<double> >(ticker, "valN");
	HEvent::Instance()->remove<vector<double> >(ticker, "msecN1s");
	HEvent::Instance()->remove<vector<double> >(ticker, "valN1s");
}


void HTickSeriesNews::endMarketDay()
{
}

void HTickSeriesNews::endJob()
{
}

void HTickSeriesNews::get_quote_series( vector<double>& msecN, vector<double>& valN,
										const TickSeries<QuoteInfo>* ptsN, vector<pair<int, int> >& sessions)
{
	int ntq = ptsN->NTicks();
	if( ntq >= minNQuotes_ )
	{
		const_cast<TickSeries<QuoteInfo>*>(ptsN)->StartRead();
		QuoteInfo quote;
		for( int n=0; n<ntq; ++n )
		{
			const_cast<TickSeries<QuoteInfo>*>(ptsN)->Read(&quote);
			int ms = quote.msecs;
			bool inrange = find_if(sessions.begin(), sessions.end(), bind2nd(inRange(), ms)) != sessions.end();
			if( inrange || include_overnight_ )
			{
				double rel = quote.bidEx;
				double css = quote.bidSize;
				double mean = quote.bid;
				//double ecm = quote.ask;
				//double rcm = quote.askSize;
				//double vcm = quote.askEx;
				//double val = (rel / 100.0) * ( wle + pcm + ecm + rcm + vcm - 250 );
				if( rel >= min_rel_ )
				{
					msecN.push_back(quote.msecs);
					if( rel_weight_ )
						valN.push_back((rel/100.0) * (css - 50));
					else
						valN.push_back(css - 50);
				}
			}
		}
	}
}

void HTickSeriesNews::get_quote_1s_series( vector<double>& msecN1s, vector<double>& valN1s,
			vector<double>& msecN, vector<double>& valN)
{
	int NN = msecN.size();

	// Fill 1 sec news series
	if( NN > 0 )
	{
		int sec1N = msecOpen_/1000;
		int sec2N = msecClose_/1000;

		int N1s = sec2N - sec1N + 1;
		msecN1s = vector<double>(N1s, 0);
		valN1s = vector<double>(N1s, 0);

		vector<double> valN1s_sum(N1s, 0);
		vector<double> valN1s_n(N1s, 0);

		for( int sec = sec1N; sec <= sec2N; ++sec )
			msecN1s[sec - sec1N] = sec * 1000;

		for( int i = 0; i < NN; ++i )
		{
			int msecs = msecN[i];
			double val = valN[i];
			int index1s = (msecs - sec1N*1000 <= 0)?0:( msecs - sec1N*1000 -1 ) / 1000 + 1;
			valN1s_sum[index1s] += val;
			++valN1s_n[index1s];
		}

		for( int i=0; i<N1s; ++i )
			valN1s[i] = valN1s_n[i] > 0.5? valN1s_sum[i] / valN1s_n[i]: 0;
	}
}
