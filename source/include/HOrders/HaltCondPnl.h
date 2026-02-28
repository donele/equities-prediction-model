#ifndef __HaltCondPnl__
#define __HaltCondPnl__
#include <HOrders/HaltCond.h>
#include <HOrders/HaltSched.h>
#include <gtlib_signal/QuoteSample.h>
#include <jl_lib/MercatorTrade.h>
#include <vector>

class HaltCondPnl: public HaltCond {
public:
	HaltCondPnl(){}
	HaltCondPnl(int haltLenSec, int pastSec, double stopBpt, bool debug = false);
	virtual ~HaltCondPnl(){}
	virtual void updateSched(HaltSched& hSched, const std::string& ticker, const std::vector<MercatorTrade>& mts,
			const gtlib::QuoteSample& qSample, int minMsecs);
	virtual std::string getDesc();

private:
	bool debug_;
	int haltLenSec_;
	int pastSec_;
	double stopBpt_;
};

#endif
