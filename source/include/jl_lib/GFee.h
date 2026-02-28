#ifndef __GFee__
#define __GFee__
#include <optionlibs/TickData.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>

class CLASS_DECLSPEC GFee {
public:
	static GFee& Instance();
	float feeBpt(const std::string& market, char primex, double price);
	float feeBpt(const std::string& market, char primex, double bid, double ask);
	std::vector<float> feeBptBidAsk(const std::string& market, char primex, double bid, double ask);
	std::vector<float> feeBptBidAsk(const std::string& market, char primex, QuoteInfo nbbo);

private:
	GFee();

	std::string market_;
	std::map<std::string, ExecFees**> mfees_; // us, ca, asia, eumarket
	ExecFees** getFees(const std::string& loc);
	std::string getLoc(const std::string& market);
	std::mutex mutex_;
};

#endif
