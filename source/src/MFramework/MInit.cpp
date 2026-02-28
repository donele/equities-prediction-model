#include <MFramework/MInit.h>
#include <MFramework/MEnv.h>
#include <MSignal/flt.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib_model/mFtns.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <string>
#include <map>
using namespace std;
using namespace gtlib;

MInit::MInit(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	verbose_(0),
	cntDay_(0),
	maxNticker_(0),
	d1_(0),
	d2_(0),
	multiThreadModule_(false),
	multiThreadTicker_(false),
	nMaxThreadTicker_(4),
	requireDataOK_(true),
	ticker_choice_("univ")
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("debugODBC") )
		GODBC::Instance()->set_debug(true);
	if( conf.count("maxNticker") )
		maxNticker_ = atoi(conf.find("maxNticker")->second.c_str());

	// Multithreading
	if( conf.count("multiThreadModule") )
		multiThreadModule_ = conf.find("multiThreadModule")->second == "true";
	if( conf.count("multiThreadTicker") )
		multiThreadTicker_ = conf.find("multiThreadTicker")->second == "true";
	if( conf.count("nMaxThreadTicker") )
		nMaxThreadTicker_ = atoi(conf.find("nMaxThreadTicker")->second.c_str());
	MEnv::Instance()->multiThreadModule = multiThreadModule_;
	MEnv::Instance()->multiThreadTicker = multiThreadTicker_;
	MEnv::Instance()->nMaxThreadTicker = nMaxThreadTicker_;

	if( conf.count("debugFlag") )
		MEnv::Instance()->debugFlag = atoi(conf.find("debugFlag")->second.c_str());

	// Markets.
	typedef multimap<string, string>::const_iterator mmit;
	if( conf.count("market") )
	{
		pair<mmit, mmit> markets = conf.equal_range("market");
		vector<string> vmarkets;
		for( mmit mi = markets.first; mi != markets.second; ++mi )
		{
			string m = mi->second;
			vector<string> vm = split(m);
			for( vector<string>::iterator it = vm.begin(); it != vm.end(); ++it )
				vmarkets.push_back(*it);
		}
		MEnv::Instance()->markets = vmarkets;
	}

	if( conf.count("tickerChoice") )
		ticker_choice_ = conf.find("tickerChoice")->second;

	// Looping order.
	MEnv::Instance()->loopingOrder = "mdt";

	if( conf.count("requireDataOK") )
		requireDataOK_ = conf.find("requireDataOK")->second != "false";

	// Tickers debug.
	vector<string> vticker;
	if( conf.count("ticker") )
	{
		pair<mmit, mmit> tickers = conf.equal_range("ticker");
		for( mmit mi = tickers.first; mi != tickers.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 1 )
				vticker.push_back(vs[0]);
		}
		MEnv::Instance()->tickersDebug = vticker;
	}

	if( conf.count("nTickerMax") )
		MEnv::Instance()->nTickerMax = atoi(conf.find("nTickerMax")->second.c_str());

	// Dates.
	if( conf.count("dateFrom") && conf.count("dateTo") )
	{
		d1_ = arg_idate( conf.find("dateFrom")->second );
		d2_ = arg_idate( conf.find("dateTo")->second );
		if( d1_ < 20000000 || d2_ > 21000000 || d2_ < d1_ )
		{
			cerr << "Invalid date range " << d1_ << ' ' << d2_ << endl;
			exit(21);
		}
	}
}

MInit::~MInit()
{
}

void MInit::beginJob()
{
	cout << getTimerInfoSimple() << endl;

	set_idate_list();

	return;
}

void MInit::beginDay()
{
	++cntDay_;
	int idate = MEnv::Instance()->idate;
	if( verbose_ > 0 )
	{
		char buf[200];
		if( MEnv::Instance()->loopingOrder == "mdt" )
			sprintf(buf, "\nMInit::beginDay() %s %d %d ", MEnv::Instance()->market.c_str(), cntDay_, idate);
		else
			sprintf(buf, "\nMInit::beginDay() %d %d ", cntDay_, idate);
		cout << buf;
		cout << getMemoryInfoSimple() << ' ' << getTimerInfoSimple() << endl;
	}
}

void MInit::beginMarket()
{
	if( MEnv::Instance()->loopingOrder == "mdt" )
		set_idate_list();
}

void MInit::beginMarketDay()
{
	//set_ticker_list(MEnv::Instance()->idate, MEnv::Instance()->market);
}

void MInit::beginTicker(const string& ticker)
{
}

void MInit::endTicker(const string& ticker)
{
}

void MInit::endMarket()
{
}

void MInit::endDay()
{
	int idate = MEnv::Instance()->idate;
}

void MInit::endJob()
{
	cout << "MInit::endJob() " << getMemoryInfoSimple() << ' ' << getTimerInfoSimple() << endl;
}

int MInit::arg_idate(const string& sdate)
{
	int ret = 0;
	if( sdate == "today" )
		ret = itoday();
	else if( !sdate.empty() )
	{
		char s = sdate[0];
		if( s == '-' || (s >= 48 && s <= 57) ) // minus sign and numbers.
		{
			int n = atoi( sdate.c_str() );
			if( n == 0 || n > 20000000 )
				ret = atoi( sdate.c_str() );
			else // relative date.
			{
				ret = itoday();
				string market = MEnv::Instance()->markets[0];
				if( n > 0 )
				{
					for( int i = 0; i < n; ++i )
						ret = nextClose(market, ret);
				}
				else if( n < 0 )
				{
					for( int i = 0; i < abs(n); ++i )
						ret = prevClose(market, ret);
				}
			}
		}
	}

	return ret;
}

void MInit::set_idate_list()
{
	set<int> dates;
	vector<string> markets = MEnv::Instance()->markets;
	for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it )
	{
		string market = *it;
		int idate = (int)GEX::Instance()->get(market)->NextOpen( QuoteTime(d1_, 040000, mto::tz(market)) ).Date();
		if( isECN_EU(market) ) // Find all the weekdays.
		{
			for( ; idate <= d2_; idate = (int)(QuoteTime(idate, 200000, mto::tz(market)) + RealTime(1.0)).Date() )
			{
				QuoteTime qt = QuoteTime(idate, 200000, mto::tz(market));
				int weekday = qt.WeekDay();
			}
		}
		else
		{
			for( ; idate <= d2_; idate = (int)GEX::Instance()->get(market)->NextOpen( QuoteTime(idate, 200000, mto::tz(market)) ).Date() )
				dates.insert(idate);
		}
	}

	vector<int> idates(dates.begin(), dates.end());
	MEnv::Instance()->idates = idates;
}
