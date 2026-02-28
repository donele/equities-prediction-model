#ifndef __QuoteStat__
#define __QuoteStat__

#include "optionlibs/TickData.h"
#include <vector>
#include <string>
#include <utility>
#include <iostream>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC QuoteStat
{
public:
	QuoteStat();
	QuoteStat(std::string market, int idate);
	void reset(std::string market, int idate);
	
	void beginDay();
	void beginTicker();
	void endTicker();
	void endDay();

	void add_quote(const QuoteInfo& quote);

private:
	
};

#endif