#ifndef __HTickSeriesQuote__
#define __HTickSeriesQuote__
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

class CLASS_DECLSPEC HTickSeriesQuote: public HModule {
public:
	HTickSeriesQuote(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HTickSeriesQuote();

	virtual void beginJob();
	virtual void beginMarketDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endMarketDay();
	virtual void endJob();

private:
	bool debug_;
	int minNQuotes_;
	int msecOpen_;
	int msecClose_;
	std::vector<std::pair<int, int> > sessions_;
	std::vector<std::vector<std::string> > orders_;

	void get_quote_series( std::vector<double>& msecQ, std::vector<double>& midQ,
										std::vector<double>& bidQ, std::vector<double>& askQ,
										const TickSeries<QuoteInfo>* ptsQ, std::vector<std::pair<int, int> >& sessions);
	void get_quote_1s_series(  std::vector<double>& msecQ1s,  std::vector<double>& midQ1s,
			std::vector<double>& bidQ1s, std::vector<double>& askQ1s,
			std::vector<double>& msecQ, std::vector<double>& midQ,
			std::vector<double>& bidQ_series, std::vector<double>& askQ_serie);
	void replace_zeros(std::vector<double>& series);
};

#endif