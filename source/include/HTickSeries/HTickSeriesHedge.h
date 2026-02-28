#ifndef __HTickSeriesHedge__
#define __HTickSeriesHedge__
#include <HLib/HModule.h>
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

class CLASS_DECLSPEC HTickSeriesHedge: public HModule {
public:
	HTickSeriesHedge(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HTickSeriesHedge();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();
private:
	bool debug_;
	int msecOpen_;
	int msecClose_;
	std::vector<double> secRetCum_;
};

#endif