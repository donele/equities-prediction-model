#include <MSignal/MReadOrder.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <map>
#include <string>
using namespace std;
using namespace hff;

MReadOrder::MReadOrder(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

MReadOrder::~MReadOrder()
{}

void MReadOrder::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void MReadOrder::beginMarketDay()
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;

	trades_.clear();
	orders_.clear();

	char cmd[1000];
	sprintf( cmd, "select symbol, eventMsecs, side, qty "
		" from hforderevents "
		" where %s "
		" and eventType = 2 ",
		mto::selOrder(market, idate).c_str() );
	GODBC::Instance()->read(mto::hf(market), cmd, trades_);
}

void MReadOrder::beginTicker(const string& ticker)
{
	vector<OrderQty> trades;
	string base_ticker = baseTicker(ticker);
	for( vector<vector<string> >::iterator it1 = trades_.begin(); it1 != trades_.end(); ++it1 )
	{
		string it_ticker = trim((*it1)[0]);
		if( it_ticker == base_ticker )
		{
			int msecs = atoi((*it1)[1].c_str());
			int side = (trim((*it1)[2])[0] == 'B') ? 1 : -1;
			int qty = atoi((*it1)[3].c_str());
			trades.push_back(OrderQty(msecs, side * qty));
		}
	}
	MEvent::Instance()->add<std::vector<OrderQty> >(ticker, "tradeQty", trades);
}

void MReadOrder::endTicker(const string& ticker)
{
	MEvent::Instance()->remove<vector<OrderQty> >(ticker, "tradeQty");
}

void MReadOrder::endMarketDay()
{
}

void MReadOrder::endJob()
{
}
