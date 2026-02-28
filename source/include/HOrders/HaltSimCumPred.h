#ifndef __HaltSimCumPred__
#define __HaltSimCumPred__
#include <HOrders/HaltSim.h>
#include <gtlib_signal/QuoteSample.h>
#include <jl_lib/MercatorTrade.h>
#include <vector>

class HaltSimCumPred: public HaltSim {
public:
	HaltSimCumPred(){}
	HaltSimCumPred(int id, double minPrice, int minMsecs,
			int pastSec, int haltLenSec, int minNtrades, double stopBpt);
	virtual ~HaltSimCumPred(){}
	virtual HaltCondition* getHaltCondition(const std::string& ticker,
			const std::vector<MercatorTrade>& mts, const gtlib::QuoteSample& qSample);
	virtual std::string getDesc();

private:
	int pastSec_;
	int haltLenSec_;
	int minNtrades_;
	double stopBpt_;
};

//
// HaltCondition subclass.
//

class HaltConditionCumPred: public HaltCondition {
public:
	HaltConditionCumPred();
	HaltConditionCumPred(const std::string& ticker, const std::vector<MercatorTrade>& mts,
			const gtlib::QuoteSample& qSample, double minPrice, int minMsecs,
			int pastSec, int haltLenSec, int minNtrades, double stopBpt, bool debug);
};

#endif
