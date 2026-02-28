#ifndef __HSignalSubset__
#define __HSignalSubset__
#include <HLearnObj/Signal.h>
#include <HLearnObj/Sample.h>
#include "HLib.h"
#include "optionlibs/TickData.h"
#include <string>
#include <map>
#include <set>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HSignalSubset: public HModule {
public:
	HSignalSubset(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HSignalSubset();

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
	double scale_;
	std::string id_;
	std::string input_dir_;
	std::ifstream ifs_;
	std::ofstream ofs_;

	void read_sample(std::string ticker);
	void write_signal(std::vector<hnn::Signal>& vSignal, std::string lastTicker);
};

#endif

