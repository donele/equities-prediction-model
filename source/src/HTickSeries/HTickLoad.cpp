#include <HTickSeries.h>
//#include <HMod/TickSeriesFtns.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib/mto.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <string>
#include <jl_lib/GODBC.h>
#include <TH1.h>
#include <TFile.h>
#include <TProfile.h>
using namespace std;

HTickLoad::HTickLoad(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
leadingTickerMarket_(""),
minNQuotes_(10),
openDelay_(10),
nDay_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("leadingTickerMarket") )
		leadingTickerMarket_ = conf.find("leadingTickerMarket")->second.c_str();
	if( conf.count("leadingTicker") )
	{
		pair<mmit, mmit> tickers = conf.equal_range("leadingTicker");
		for( mmit mi = tickers.first; mi != tickers.second; ++mi )
		{
			string leadingTicker = mi->second;
			leadingTickers_.insert(leadingTicker);
		}
	}
}

HTickLoad::~HTickLoad()
{}

void HTickLoad::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	return;
}

void HTickLoad::beginMarket()
{
	sessions_ = mto::sessions(leadingTickerMarket_, openDelay_, 0, 0);
	int ndates = HEnv::Instance()->nDates();
	vector<int> idates = HEnv::Instance()->idates();

	//leadingTickerMsecOpen_ = mto::msecOpen(leadingTickerMarket_);
	//leadingTickerMsecClose_ = mto::msecClose(leadingTickerMarket_);

	return;
}

void HTickLoad::beginDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();
	++nDay_;

	load_leading_tickers(idate);

	return;
}

void HTickLoad::beginTicker(const string& ticker)
{
	return;
}

void HTickLoad::endTicker(const string& ticker)
{
	return;
}

void HTickLoad::endDay()
{
	// Remove the price serieses from the Event.
	for( set<string>::iterator it = leadingTickers_.begin(); it != leadingTickers_.end(); ++it )
	{
		string ticker = *it;
		HEvent::Instance()->remove<vector<double> >("", ticker + "_msecQ");
		HEvent::Instance()->remove<vector<double> >("", ticker + "_midQ");
		HEvent::Instance()->remove<vector<double> >("", ticker + "_bidQ");
		HEvent::Instance()->remove<vector<double> >("", ticker + "_askQ");
		HEvent::Instance()->remove<vector<double> >("", ticker + "_msecQ1s");
		HEvent::Instance()->remove<vector<double> >("", ticker + "_midQ1s");
		HEvent::Instance()->remove<vector<double> >("", ticker + "_bidQ1s");
		HEvent::Instance()->remove<vector<double> >("", ticker + "_askQ1s");
	}

	return;
}

void HTickLoad::endMarket()
{
	return;
}

void HTickLoad::endJob()
{
	return;
}

void HTickLoad::load_leading_tickers(int idate)
{
	TickAccessMulti<QuoteInfo> taQ;
	TickSeries<QuoteInfo> ts;

	vector<string> quote_dir = mto::bindirs(leadingTickerMarket_, idate);
	for( vector<string>::iterator it = quote_dir.begin(); it != quote_dir.end(); ++it )
		taQ.AddRoot(*it, mto::longTicker(leadingTickerMarket_));

	for( set<string>::iterator it = leadingTickers_.begin(); it != leadingTickers_.end(); ++it )
	{
		string ticker = *it;
		taQ.GetTickSeries( ticker, idate, &ts );

		vector<double> msecQ;
		vector<double> midQ;
		vector<double> bidQ;
		vector<double> askQ;
		vector<double> msecQ1s;
		vector<double> midQ1s;
		vector<double> bidQ1s;
		vector<double> askQ1s;

		TickSeriesFtns::get_quote_series( msecQ, midQ, bidQ, askQ, &ts, sessions_);

		TickSeriesFtns::get_quote_1s_series( msecQ1s, midQ1s, bidQ1s, askQ1s,
							msecQ, midQ, bidQ, askQ);

		// Add to the event
		HEvent::Instance()->add<vector<double> >("", ticker + "_msecQ", msecQ);
		HEvent::Instance()->add<vector<double> >("", ticker + "_midQ", midQ);
		HEvent::Instance()->add<vector<double> >("", ticker + "_bidQ", bidQ);
		HEvent::Instance()->add<vector<double> >("", ticker + "_askQ", askQ);
		HEvent::Instance()->add<vector<double> >("", ticker + "_msecQ1s", msecQ1s);
		HEvent::Instance()->add<vector<double> >("", ticker + "_midQ1s", midQ1s);
		HEvent::Instance()->add<vector<double> >("", ticker + "_bidQ1s", bidQ1s);
		HEvent::Instance()->add<vector<double> >("", ticker + "_askQ1s", askQ1s);
	}
	return;
}