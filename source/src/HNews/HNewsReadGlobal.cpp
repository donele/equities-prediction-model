#include <HNews.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib/mto.h>
#include <map>
#include <string>
#include "TFile.h"
using namespace std;

HNewsReadGlobal::HNewsReadGlobal(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

HNewsReadGlobal::~HNewsReadGlobal()
{}

void HNewsReadGlobal::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	assert_loopingOrder_mdt();

	return;
}

void HNewsReadGlobal::beginMarket()
{
	string market = HEnv::Instance()->market();

	string temp_dir = "";
	if( "US" == market )
		temp_dir = "\\\\smrc-nas09\\gf1\\tickC\\us\\news_global";
	else if( "C" == mto::region(market) )
		temp_dir = "\\\\smrc-ltc-mrct16\\data00\\tickC\\ca\\news_global";
	else if( mto::isInternational(market) )
		temp_dir = (string)"\\\\smrc-nas09\\gf1\\tickC\\" + mto::region_long(market) + "\\news_global\\" + mto::code(market);

	string dirRP = temp_dir;

	//int loc = temp_dir.find("news");
	//string dirRT = temp_dir.insert(loc + 4, "RT");

	taRP_.AddRoot(dirRP, mto::longTicker(market));
	//taRT_.AddRoot(dirRT, mto::longTicker(market));

	return;
}

void HNewsReadGlobal::beginDay()
{
	int idate = HEnv::Instance()->idate();
	//string market = HEnv::Instance()->market();

	TickSeries<QuoteInfo> tsRP;
	{
		boost::mutex::scoped_lock lock(taRP_mutex_);
		taRP_.GetTickSeries( "GLOBAL", idate, &tsRP );
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
		HEvent::Instance()->add<vector<QuoteInfo> >("GLOBAL", "newsRP", v);
	}

	return;
}

//void HNewsReadGlobal::beginTicker(const string& ticker)
//{
//	return;
//}
//
//void HNewsReadGlobal::endTicker(const string& ticker)
//{
//	return;
//}

void HNewsReadGlobal::endDay()
{
	HEvent::Instance()->remove<vector<QuoteInfo> >("GLOBAL", "newsRP");
	return;
}

void HNewsReadGlobal::endMarket()
{
	return;
}

void HNewsReadGlobal::endJob()
{
	return;
}
