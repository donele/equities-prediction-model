#ifndef __HTickLoad__
#define __HTickLoad__
#include <HLib/HModule.h>
#include <optionlibs/TickData.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <string>
#include <set>
#include <vector>
#include <TH1.h>
#include <TProfile.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HTickLoad: public HModule {
public:
	HTickLoad(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HTickLoad();

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
	int minNQuotes_;
	//int leadingTickerMsecOpen_;
	//int leadingTickerMsecClose_;
	int openDelay_;
	std::string leadingTickerMarket_;

	int nDay_;
	std::vector<std::string> tickers_;
	std::set<std::string> leadingTickers_;
	std::vector<std::pair<int, int> > sessions_;


	void load_leading_tickers(int iadte);
	void get_quote_1s_series( std::vector<double>& midQ1s, const TickSeries<QuoteInfo>* ptsQ,
		std::vector<std::pair<int, int> >& sessions_ );
	void replace_zeros(std::vector<double>& series);
	//void get_quote_series( std::vector<double>& msecQ, std::vector<double>& midQ,
	//					const TickSeries<QuoteInfo>* ptsQ, std::vector<std::pair<int, int> >& sessions );
	//void get_quote_1s_series( std::vector<double>& midQ1s,
	//					std::vector<double>& msecQ, std::vector<double>& midQ );
};

#endif