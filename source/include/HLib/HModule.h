#ifndef __HModule__
#define __HModule__
#include <string>
#include <TStopwatch.h>
#include <boost/thread.hpp>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HModule {
public:
	HModule();
	HModule(const std::string& moduleName, bool threadSafe = false);
	virtual ~HModule();

	bool validIdate(int idate);

	void beginJobBase();
	void beginMarketBase();
	void beginDayBase();
	void beginMarketDayBase();
	void beginTickerBase(std::string ticker);
	void endJobBase();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginMarketDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endMarketDay();
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

protected:
	int minIdate_;
	int maxIdate_;
	std::string moduleName_;
	bool threadSafe_;
	boost::mutex private_mutex_;
	boost::mutex timer_mutex_;
	double realtimeBeginTicker_;
	double cputimeBeginTicker_;

	TStopwatch swBeginJob_;
	TStopwatch swBeginMarket_;
	TStopwatch swBeginDay_;
	TStopwatch swBeginMarketDay_;
	TStopwatch swBeginTicker_;

	void timeSummary();
	void printTime(std::string fname, double realtime, double cputime);
	void assert_loopingOrder_dmt();
	void assert_loopingOrder_mdt();
};

#endif
