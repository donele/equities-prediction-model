#ifndef __HaltSimRet__
#define __HaltSimRet__
#include <HOrders/HaltSim.h>
#include <gtlib_signal/QuoteSample.h>
#include <jl_lib/MercatorTrade.h>
#include <vector>

class HaltSimRet: public HaltSim {
public:
	HaltSimRet(){}
	HaltSimRet(int id, double minPrice, int minMsecs,
			int pastMsec, int haltLenSec, double stopBpt);
	virtual ~HaltSimRet(){}
	virtual HaltCondition* getHaltCondition(const std::string& ticker,
			const std::vector<MercatorTrade>& mts, const gtlib::QuoteSample& qSample);
	virtual std::string getDesc();

private:
	int pastMsec_;
	int haltLenSec_;
	double stopBpt_;
};

//
// HaltCondition subclass.
//

class HaltConditionRet: public HaltCondition {
public:
	HaltConditionRet();
	HaltConditionRet(const std::vector<MercatorTrade>& mts,
			const gtlib::QuoteSample& qSample,
			double minPrice, int minMsecs, int pastMsec, int haltLenSec, double stopBpt);
};

#endif
