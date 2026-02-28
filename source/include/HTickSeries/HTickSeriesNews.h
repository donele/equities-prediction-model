#ifndef __HTickSeriesNews__
#define __HTickSeriesNews__
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

class CLASS_DECLSPEC HTickSeriesNews: public HModule {
public:
	HTickSeriesNews(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HTickSeriesNews();

	virtual void beginJob();
	virtual void beginMarketDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endMarketDay();
	virtual void endJob();

private:
	bool debug_;
	bool include_overnight_;
	int minNQuotes_;
	int msecOpen_;
	int msecClose_;
	double min_rel_;
	bool rel_weight_;
	std::vector<std::pair<int, int> > sessions_;
	std::vector<std::vector<std::string> > orders_;

	void get_quote_series( std::vector<double>& msecN, std::vector<double>& valN,
										const TickSeries<QuoteInfo>* ptsN, std::vector<std::pair<int, int> >& sessions);
	void get_quote_1s_series(  std::vector<double>& msecN1s, std::vector<double>& valN1s,
			std::vector<double>& msecN, std::vector<double>& valN);
	void replace_zeros(std::vector<double>& series);
};

#endif