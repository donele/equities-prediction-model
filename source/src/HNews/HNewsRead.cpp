#include <HNews.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib/mto.h>
#include <map>
#include <string>
#include "TFile.h"
using namespace std;

HNewsRead::HNewsRead(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

HNewsRead::~HNewsRead()
{}

void HNewsRead::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	assert_loopingOrder_mdt();

	return;
}

void HNewsRead::beginMarket()
{
	string market = HEnv::Instance()->market();

	string temp_dir = "";
	if( "US" == market )
		temp_dir = "\\\\smrc-nas09\\gf1\\tickC\\us\\news";
	else if( "C" == mto::region(market) )
		temp_dir = "\\\\smrc-ltc-mrct16\\data00\\tickC\\ca\\news";
	else if( mto::isInternational(market) )
		temp_dir = (string)"\\\\smrc-nas09\\gf1\\tickC\\" + mto::region_long(market) + "\\news\\" + mto::code(market);

	string dirRP = temp_dir;

	int loc = temp_dir.find("news");
	string dirRT = temp_dir.insert(loc + 4, "RT");

	taRP_.AddRoot(dirRP, mto::longTicker(market));
	taRT_.AddRoot(dirRT, mto::longTicker(market));

	return;
}

void HNewsRead::beginDay()
{
	//int idate = HEnv::Instance()->idate();

	//vector<string> namesRP;
	//{
	//	boost::mutex::scoped_lock lock(taRP_mutex_);
	//	taRP_.GetNames(idate, &namesRP);
	//}

	//vector<string> namesRT;
	//TickSeries<QuoteInfo> tsRT;
	//{
	//	boost::mutex::scoped_lock lock(taRT_mutex_);
	//	taRT_.GetNames(idate, &namesRT);
	//}
	//sort(namesRT.begin(), namesRT.end());

	//vector<string> v;
	//for( vector<string>::iterator it = namesRP.begin(); it != namesRP.end(); ++it )
	//{
	//	if( binary_search(namesRT.begin(), namesRT.end(), *it) )
	//		v.push_back(*it);
	//}
	//HEnv::Instance()->tickers(v);

	return;
}

void HNewsRead::beginTicker(const string& ticker)
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();

	TickSeries<QuoteInfo> tsRP;
	{
		boost::mutex::scoped_lock lock(taRP_mutex_);
		taRP_.GetTickSeries( ticker, idate, &tsRP );
	}

	int n = tsRP.NTicks();
	if( n > 0 )
	{
		vector<QuoteInfo> v;
		tsRP.StartRead();
		QuoteInfo quote;
		for( int i = 0; i < n; ++i )
		{
			tsRP.Read(&quote);
			v.push_back(quote);
		}
		HEvent::Instance()->add<vector<QuoteInfo> >(ticker, "newsRP", v);
	}
	//HEvent::Instance()->add<vector<QuoteInfo> >(ticker, "newsRP", tsRP);

	//TickSeries<QuoteInfo> tsRT;
	//{
	//	boost::mutex::scoped_lock lock(taRT_mutex_);
	//	taRT_.GetTickSeries( ticker, idate, &tsRT );
	//}
	//HEvent::Instance()->add<TickSeries<QuoteInfo> >(ticker, "newsRT", tsRT);

	return;
}

void HNewsRead::endTicker(const string& ticker)
{
	HEvent::Instance()->remove<vector<QuoteInfo> >(ticker, "newsRP");
	return;
}

void HNewsRead::endDay()
{
	return;
}

void HNewsRead::endMarket()
{
	return;
}

void HNewsRead::endJob()
{
	return;
}
