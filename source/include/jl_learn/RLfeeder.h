#ifndef __RLfeeder__
#define __RLfeeder__
#include "RLobj.h"
#include "optionlibs/TickData.h"
#include <string>
#include <vector>
#include <map>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC RLfeeder {
public:
	RLfeeder();
	void addTicker(std::string ticker, std::vector<QuoteInfo>& series);
	bool advance();
	RLinput next();
	void endDay();
	bool empty();

	typedef std::vector<QuoteInfo>::iterator vqi;

private:
	RLinput ri_;
	std::map<std::string, std::vector<QuoteInfo> > tickerPrcs_;
	std::map<std::string, std::pair<vqi, vqi> > tickerIters_;
	std::map<std::string, vqi> tickerEnds_;
};

#endif