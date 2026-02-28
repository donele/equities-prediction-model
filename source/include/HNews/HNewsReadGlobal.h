#ifndef __HNewsReadGlobal__
#define __HNewsReadGlobal__
#include <HLib/HModule.h>
#include <optionlibs/TickData.h>
#include <jl_lib/Sessions.h>
#include <TH1.h>
#include <TH2.h>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HNewsReadGlobal: public HModule {
public:
	HNewsReadGlobal(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HNewsReadGlobal();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	//virtual void beginTicker(const std::string& ticker);
	//virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	bool debug_;
	TickAccessMulti<QuoteInfo> taRP_;
	//TickAccessMulti<QuoteInfo> taRT_;
	boost::mutex taRP_mutex_;
	//boost::mutex taRT_mutex_;
};

#endif
