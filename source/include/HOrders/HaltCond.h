#ifndef __HaltCond__
#define __HaltCond__
#include <HOrders/HaltSched.h>
#include <gtlib_signal/QuoteSample.h>
#include <jl_lib/MercatorTrade.h>

class HaltCond {
public:
	HaltCond(){}
	virtual ~HaltCond(){}
	virtual void beginDay(int idate){}
	virtual void updateSched(HaltSched& hSched, const std::string& ticker, const std::vector<MercatorTrade>& mts,
			const gtlib::QuoteSample& qSample, int minMsecs) = 0;
	virtual std::string getDesc() = 0;
};

#endif
