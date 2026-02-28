#ifndef __MModuleBase__
#define __MModuleBase__
#include <string>
#include <boost/thread.hpp>
#include <jl_lib/crossCompile.h>

class CLASS_DECLSPEC MModuleBase {
public:
	MModuleBase();
	MModuleBase(const std::string& moduleName, bool threadSafe = false);
	virtual ~MModuleBase();

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
	std::string moduleName_;
	bool threadSafe_;
	boost::mutex private_mutex_;
	boost::mutex timer_mutex_;
	double realtimeBeginTicker_;
	double cputimeBeginTicker_;

	void assert_loopingOrder_dmt();
	void assert_loopingOrder_mdt();
};

#endif
