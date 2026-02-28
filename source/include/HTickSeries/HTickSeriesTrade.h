#ifndef __HTickSeriesTrade__
#define __HTickSeriesTrade__
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

class CLASS_DECLSPEC HTickSeriesTrade: public HModule {
public:
	HTickSeriesTrade(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HTickSeriesTrade();

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
	std::string source_;
	std::string source_ext_;
	std::vector<std::pair<int, int> > sessions_;
	std::vector<std::vector<std::string> > orders_;

	void replace_zeros(std::vector<double>& series);
	std::string flag2source(char c);
};

#endif