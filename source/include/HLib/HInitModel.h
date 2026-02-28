#ifndef __HInitModel__
#define __HInitModel__
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

class CLASS_DECLSPEC HInitModel: public HModule {
public:
	HInitModel(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HInitModel();

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
	int udate_;
	int ndates_;
	std::string model_;
	std::string baseDir_;
	std::map<std::string, std::vector<std::string> > marketTickers_;
};

#endif