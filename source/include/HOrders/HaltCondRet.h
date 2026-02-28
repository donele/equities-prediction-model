#ifndef __HaltCondRet__
#define __HaltCondRet__
#include <HOrders/HaltCond.h>
#include <HOrders/HaltSched.h>
#include <gtlib_signal/QuoteSample.h>
#include <jl_lib/MercatorTrade.h>
#include <vector>

class HaltCondRet: public HaltCond {
public:
	HaltCondRet(){}
	HaltCondRet(int haltLenSec, int pastMsec, double stopBpt, bool debug = false);
	virtual ~HaltCondRet(){}
	virtual void updateSched(HaltSched& hSched, const std::string& ticker,
			const std::vector<MercatorTrade>& mts, const gtlib::QuoteSample& qSample,
			int minMsecs);
	virtual std::string getDesc();

private:
	bool debug_;
	int haltLenSec_;
	int pastMsec_;
	double stopBpt_;
};

#endif
