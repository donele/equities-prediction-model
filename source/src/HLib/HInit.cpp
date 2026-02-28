#include <HLib/HInit.h>
#include <HLib/HEnv.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/GEX.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <string>
#include <map>
using namespace std;

HInit::HInit(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
verbose_(1),
multiThreadModule_(false),
multiThreadTicker_(false),
nMaxThreadTicker_(4),
requireDataOK_(true),
includeHolidays_(false)
{
	swTotal_.Reset();
	swTotal_.Start(kFALSE);

	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("debugODBC") )
		GODBC::Instance()->set_debug(true);

	// Multithreading
	if( conf.count("multiThreadModule") )
		multiThreadModule_ = conf.find("multiThreadModule")->second == "true";
	if( conf.count("multiThreadTicker") )
		multiThreadTicker_ = conf.find("multiThreadTicker")->second == "true";
	if( conf.count("nMaxThreadTicker") )
		nMaxThreadTicker_ = atoi(conf.find("nMaxThreadTicker")->second.c_str());
	HEnv::Instance()->multiThreadModule(multiThreadModule_);
	HEnv::Instance()->multiThreadTicker(multiThreadTicker_);
	HEnv::Instance()->nMaxThreadTicker(nMaxThreadTicker_);

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
		HEnv::Instance()->markets(vmarkets);
	}

	// Exclude dates.
	if( conf.count("excludeDates") )
	{
		pair<mmit, mmit> dates = conf.equal_range("excludeDates");
		for( mmit mi = dates.first; mi != dates.second; ++mi )
		{
			int d = atoi(mi->second.c_str());
			excludeDates_.insert(d);
		}
	}

	// Market rep.
	if( conf.count("marketRep") )
	{
		int marketRep = atoi( conf.find("marketRep")->second.c_str() );
		HEnv::Instance()->marketRep(marketRep);
	}

	// Looping order.
	if( conf.count("loopingOrder") )
		HEnv::Instance()->loopingOrder( conf.find("loopingOrder")->second );

	// Dates.
	if( conf.count("dateFrom") )
		d1_ = atoi( conf.find("dateFrom")->second.c_str() );
	if( conf.count("dateTo") )
		d2_ = atoi( conf.find("dateTo")->second.c_str() );

	// requireDataOK.
	if( conf.count("requireDataOK") )
		requireDataOK_ = conf.find("requireDataOK")->second != "false";

	// includeHolidays.
	if( conf.count("includeHolidays") )
		includeHolidays_ = conf.find("includeHolidays")->second != "false";

	// Outfile.
	if( conf.count("outfile") )
		HEnv::Instance()->outfile( conf.find("outfile")->second );
}

HInit::~HInit()
{}

void HInit::beginJob()
{
	if( HEnv::Instance()->loopingOrder() == "dmt" )
	{
		set<int> dates;
		vector<string> markets = HEnv::Instance()->markets();
		for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it )
		{
			string market = *it;

			// dates list.
			if( includeHolidays_ )
			{
				int idate = d1_;
				for( ; idate <= d2_; idate = (int)(QuoteTime(idate, 200000, "GMT") + RealTime(1)).Date() ) // Time zone doesn't matter.
				{
					if( !excludeDates_.count(idate) )
						dates.insert(idate);
				}
			}
			else
			{
				int idate = (int)GEX::Instance()->get(market)->NextOpen( QuoteTime(d1_, 040000, mto::tz(market)) ).Date();
				if( isECN_EU(market) ) // Find all the weekdays.
				{
					for( ; idate <= d2_; idate = (int)(QuoteTime(idate, 200000, mto::tz(market)) + RealTime(1.0)).Date() )
					{
						QuoteTime qt = QuoteTime(idate, 200000, mto::tz(market));
						int weekday = qt.WeekDay();
						if( !excludeDates_.count(idate) && weekday > 0 && weekday < 6 )
							dates.insert(idate);
					}
				}
				else
				{
					for( ; idate <= d2_; idate = (int)GEX::Instance()->get(market)->NextOpen( QuoteTime(idate, 200000, mto::tz(market)) ).Date() )
					{
						if( !excludeDates_.count(idate) )
							dates.insert(idate);
					}
				}
			}

			// dates list for each market.
			vector<int> midates;
			if( includeHolidays_ || isECN_EU(market) )
				midates = vector<int>(dates.begin(), dates.end());
			else
			{
				int idate = (int)GEX::Instance()->get(market)->NextOpen( QuoteTime(d1_, 040000, mto::tz(market)) ).Date();
				for( ; idate <= d2_; idate = (int)GEX::Instance()->get(market)->NextOpen( QuoteTime(idate, 200000, mto::tz(market)) ).Date() )
					if( !requireDataOK_ || mto::dataOK(market, idate) )
						if( !excludeDates_.count(idate) )
							midates.push_back(idate);
			}
			HEnv::Instance()->idates(market, midates);
		}

		vector<int> idates(dates.begin(), dates.end());
		HEnv::Instance()->idates(idates);
		HEnv::Instance()->nDates(idates.size());
	}
	return;
}

void HInit::beginMarket()
{
	if( HEnv::Instance()->loopingOrder() == "mdt" )
	{
		string market = HEnv::Instance()->market();

		// dates list.
		vector<int> idates;
		if( includeHolidays_ )
		{
			int idate = d1_;
			for( ; idate <= d2_; idate = (int)(QuoteTime(idate, 200000, "GMT") + RealTime(1)).Date() ) // Time zone doesn't matter.
			{
				if( !excludeDates_.count(idate) )
					idates.push_back(idate);
			}
		}
		else
		{
			int idate = (int)GEX::Instance()->get(market)->NextOpen( QuoteTime(d1_, 040000, mto::tz(market)) ).Date();
			for( ; idate <= d2_; idate = (int)GEX::Instance()->get(market)->NextOpen( QuoteTime(idate, 200000, mto::tz(market)) ).Date() )
				if( !requireDataOK_ || mto::dataOK(market, idate) )
					if( !excludeDates_.count(idate) )
						idates.push_back(idate);
		}
		HEnv::Instance()->idates(idates);
		HEnv::Instance()->d1(d1_);
		HEnv::Instance()->d2(d2_);

		HEnv::Instance()->nDates(idates.size());

		if( verbose_ >= 1 )
		{
			cout << "\n\n";
			cout << "beginMarket: " << market << ". date(s): ";
			int ds = idates.size();
			if( ds > 5 )
			{
				copy(idates.begin(), idates.begin() + 5, ostream_iterator<int>(cout, " "));
				cout << "... (" << ds << " dates)";
			}
			else
				copy(idates.begin(), idates.end(), ostream_iterator<int>(cout, " "));
			cout << "\n";
		}
	}

	return;
}

void HInit::beginDay()
{
	if( HEnv::Instance()->loopingOrder() == "dmt" )
	{
		int idate = HEnv::Instance()->idate();
		if( verbose_ >= 1 )
		{
			cout << "\n\n";
			cout << "beginDay: " << idate << ".\n";
		}
	}
	cout << flush;

	return;
}

void HInit::beginTicker(const string& ticker)
{
	return;
}

void HInit::endTicker(const string& ticker)
{
	return;
}

void HInit::endDay()
{
	return;
}

void HInit::endMarket()
{
	return;
}

void HInit::endJob()
{
	swTotal_.Stop();
	double realtime = swTotal_.RealTime();

	int hreal = int(realtime / 3600);
	realtime -= hreal * 3600;
	int mreal = int(realtime / 60);
	realtime -= mreal * 60;

	char buf[200];
	sprintf(buf, "%26s %3d:%02d:%06.3f\n", "Total",
		hreal, mreal, realtime);
	cout << buf;
	cout << flush;
	return;
}
