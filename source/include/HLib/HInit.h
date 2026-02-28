#ifndef __HInit__
#define __HInit__
#include "HModule.h"
#include "optionlibs/TickData.h"
#include <TStopwatch.h>
#include <boost/thread.hpp>
#include <vector>
#include <string>
#include <map>
#include <set>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HInit: public HModule {
public:
	HInit(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HInit();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	int verbose_;
	bool multiThreadModule_;
	bool multiThreadTicker_;
	int nMaxThreadTicker_;
	bool requireDataOK_;
	bool includeHolidays_;
	int d1_;
	int d2_;
	std::set<int> excludeDates_;

	TStopwatch swTotal_;
};

#endif