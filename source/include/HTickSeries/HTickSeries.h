#ifndef __HTickSeries__
#define __HTickSeries__
#include <HLib/HModule.h>
#include "optionlibs/TickData.h"
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

class CLASS_DECLSPEC HTickSeries: public HModule {
public:
	HTickSeries(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HTickSeries();

	virtual void beginJob();
	virtual void beginMarketDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endMarketDay();
	virtual void endJob();

private:
	bool debug_;
	int minNQuotes_;
	int minNTrades_;
	int msecOpen_;
	int msecClose_;
	std::string desc_;
	std::string quote_name_;
	std::string trade_name_;
	std::vector<std::pair<int, int> > sessions_;
	std::vector<std::vector<std::string> > orders_;
	boost::mutex event_mutex_;

	void replace_zeros(std::vector<double>& series);
};

#endif
