#ifndef __OneDayStat__
#define __OneDayStat__

#include "optionlibs/TickData.h"
#include <string>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC OneDayStat {
public:
	OneDayStat();
	virtual ~OneDayStat();

	virtual void fill_quote(std::string& symbol, QuoteInfo& quote);
	virtual void fill_trade(std::string& symbol, TradeInfo& trade);
	virtual void fill_order(std::string& symbol, OrderData& order);
	virtual void start_name( std::string& symbol );
	virtual void finish_name( std::string& symbol );
	virtual void finish_day() = 0;
};

#endif