#include <HFitting/HReadOrder.h>
#include <HLib.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include "TFile.h"
using namespace std;
using namespace hff;

HReadOrder::HReadOrder(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
debug_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

HReadOrder::~HReadOrder()
{}

void HReadOrder::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void HReadOrder::beginMarketDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();

	trades_.clear();
	orders_.clear();

	char cmd[1000];
	sprintf( cmd, "select symbol, eventMsecs, side, qty "
		" from hforderevents "
		" where %s "
		" and eventType = 2 ",
		mto::selOrder(market, idate).c_str() );
	GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &trades_);
}

void HReadOrder::beginTicker(const string& ticker)
{
	vector<OrderQty> trades;
	for( vector<vector<string> >::iterator it1 = trades_.begin(); it1 != trades_.end(); ++it1 )
	{
		string it_ticker = trim((*it1)[0]);
		if( it_ticker == ticker )
		{
			int msecs = atoi((*it1)[1].c_str());
			int side = (trim((*it1)[2])[0] == 'B') ? 1 : -1;
			int qty = atoi((*it1)[3].c_str());
			trades.push_back(OrderQty(msecs, side * qty));
		}
	}
	HEvent::Instance()->add<std::vector<OrderQty> >(ticker, "tradeQty", trades);
}

void HReadOrder::endTicker(const string& ticker)
{
	HEvent::Instance()->remove<vector<OrderQty> >(ticker, "tradeQty");
}

void HReadOrder::endMarketDay()
{
}

void HReadOrder::endJob()
{
}
