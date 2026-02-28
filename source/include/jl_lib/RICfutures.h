#ifndef __RICfutures__
#define __RICfutures__
#include <optionlibs/TickData.h>
#include <map>
#include <string>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC RICfutures {
public:
	static RICfutures* Instance();
	std::string most_traded_ticker(const std::string& market, int idate);

private:
	static RICfutures* instance_;
	struct Cleaner { ~Cleaner(); };
	friend struct Cleaner;
	RICfutures();

	std::map<std::string, std::string> mMarketBase_;
	std::map<int, std::string> mMonthCode_;

	int next_trading_day(const std::string& market, int idate, int n = 1);
	int next_expiration(const std::string& market, int idate);
	int next_expiration(const std::string& market, int idate, int expWeek, int expWeekDay, int freq);
	int next_expiration(const std::string& market, int idate, int nBeforeEndOfMonth);
};

#endif
