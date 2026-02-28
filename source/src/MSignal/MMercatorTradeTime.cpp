#include <MSignal/MMercatorTradeTime.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <map>
#include <string>
using namespace std;
using namespace hff;

MMercatorTradeTime::MMercatorTradeTime(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

MMercatorTradeTime::~MMercatorTradeTime()
{}

void MMercatorTradeTime::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void MMercatorTradeTime::beginMarketDay()
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;

	trades_.clear();
	orders_.clear();

	char cmd[1000];
	sprintf( cmd, "select o.symbol, o.orderMsecs"
		" from hforders o inner join hforderevents e "
		" on o.orderid = e.orderid "
		" where %s  and o.orderschedtype <= 2 "
		" and eventType = 2 ",
		mto::selTradeTime(market, idate).c_str() );
	GODBC::Instance()->read(mto::hf(market), cmd, trades_);
}

void MMercatorTradeTime::beginTicker(const string& ticker)
{
	vector<int> trades;
	string base_ticker = baseTicker(ticker);
	int prev_msecs = 0;
	for( vector<vector<string> >::iterator it1 = trades_.begin(); it1 != trades_.end(); ++it1 )
	{
		string it_ticker = trim((*it1)[0]);
		if( it_ticker == base_ticker )
		{
			int msecs = atoi((*it1)[1].c_str());
			if(msecs > prev_msecs + 10000)
				trades.push_back(msecs);
			prev_msecs = msecs;
		}
	}
	MEvent::Instance()->add<std::vector<int> >(ticker, "mercatorTradeTime", trades);
}

void MMercatorTradeTime::endTicker(const string& ticker)
{
	MEvent::Instance()->remove<vector<int> >(ticker, "mercatorTradeTime");
}

void MMercatorTradeTime::endMarketDay()
{
}

void MMercatorTradeTime::endJob()
{
}
