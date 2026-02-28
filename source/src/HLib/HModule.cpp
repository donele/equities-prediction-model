#include <HLib/HModule.h>
#include <HLib/HEnv.h>
#include <iostream>
using namespace std;

HModule::HModule()
{}

HModule::HModule(const string& moduleName, bool threadSafe)
:moduleName_(moduleName),
minIdate_(0),
maxIdate_(99999999),
threadSafe_(threadSafe),
realtimeBeginTicker_(0),
cputimeBeginTicker_(0)
{
	swBeginJob_.Reset();
	swBeginMarket_.Reset();
	swBeginDay_.Reset();
	swBeginTicker_.Reset();
}

HModule::~HModule(){}

bool HModule::validIdate(int idate)
{
	return minIdate_ <= idate && idate <= maxIdate_;
}

void HModule::beginJobBase()
{
	swBeginJob_.Start(kFALSE);

	beginJob();

	swBeginJob_.Stop();

	return;
}

void HModule::beginMarketBase()
{
	swBeginMarket_.Start(kFALSE);

	beginMarket();

	swBeginMarket_.Stop();

	return;
}

void HModule::beginDayBase()
{
	int idate = HEnv::Instance()->idate();
	if( minIdate_ <= idate && idate <= maxIdate_ )
	{
		swBeginDay_.Start(kFALSE);

		beginDay();

		swBeginDay_.Stop();
	}

	return;
}

void HModule::beginMarketDayBase()
{
	int idate = HEnv::Instance()->idate();
	if( minIdate_ <= idate && idate <= maxIdate_ )
	{
		swBeginMarketDay_.Start(kFALSE);

		beginMarketDay();

		swBeginMarketDay_.Stop();
	}

	return;
}

void HModule::beginTickerBase(string ticker)
{
	TStopwatch sw;
	if( !HEnv::Instance()->multiThreadTicker() )
		swBeginTicker_.Start(kFALSE);
	else
		sw.Start();

	beginTicker(ticker);

	if( !HEnv::Instance()->multiThreadTicker() )
		swBeginTicker_.Stop();
	else
	{
		sw.Stop();
		double realtime = sw.RealTime();
		double cputime = sw.CpuTime();
		{
			boost::mutex::scoped_lock lock(timer_mutex_);
			realtimeBeginTicker_ += realtime;
			cputimeBeginTicker_ += cputime;
		}
	}

	return;
}

void HModule::endJobBase()
{
	endJob();

	timeSummary();

	return;
}

void HModule::beginJob()
{
	return;
}

void HModule::beginMarket()
{
	return;
}

void HModule::beginDay()
{
	return;
}

void HModule::beginMarketDay()
{
	return;
}

void HModule::beginTicker(const string& ticker)
{
	return;
}

void HModule::endTicker(const string& ticker)
{
	return;
}

void HModule::endMarketDay()
{
	return;
}

void HModule::endDay()
{
	return;
}

void HModule::endMarket()
{
	return;
}

void HModule::endJob()
{
	return;
}

void HModule::timeSummary()
{
	char buf[200];
	sprintf(buf, "\n%20s %19s %13s\n", moduleName_.c_str(), "Real Time", "CPU Time");
	cout << buf;

	printTime("beginJob", swBeginJob_.RealTime(), swBeginJob_.CpuTime());
	printTime("beginMarket", swBeginMarket_.RealTime(), swBeginMarket_.CpuTime());
	printTime("beginDay", swBeginDay_.RealTime(), swBeginDay_.CpuTime());
	printTime("beginMarketDay", swBeginMarketDay_.RealTime(), swBeginMarketDay_.CpuTime());
	printTime("beginTicker", swBeginTicker_.RealTime(), swBeginTicker_.CpuTime());
	if( HEnv::Instance()->multiThreadTicker() )
		printTime("(sum threads) beginTicker", realtimeBeginTicker_, cputimeBeginTicker_);
	return;
}

void HModule::printTime(string fname, double realtime, double cputime)
{
	int hreal = int(realtime / 3600);
	realtime -= hreal * 3600;
	int mreal = int(realtime / 60);
	realtime -= mreal * 60;

	int hcpu = int(cputime / 3600);
	cputime -= hcpu * 3600;
	int mcpu = int(cputime / 60);
	cputime -= mcpu * 60;

	char buf[200];
	sprintf(buf, "%26s %3d:%02d:%06.3f %3d:%02d:%06.3f\n", fname.c_str(),
		hreal, mreal, realtime, hcpu, mcpu, cputime);
	cout << buf;
	cout << flush;
	return;
}

void HModule::assert_loopingOrder_dmt()
{
	if( HEnv::Instance()->loopingOrder() != "dmt" )
	{
		cerr << "module " << moduleName_ << " requires the loopingOrder 'dmt'." << endl;
		exit(7);
	}
	return;
}

void HModule::assert_loopingOrder_mdt()
{
	if( HEnv::Instance()->loopingOrder() != "mdt" )
	{
		cerr << "module " << moduleName_ << " requires the loopingOrder 'mdt'." << endl;
		exit(7);
	}
	return;
}
