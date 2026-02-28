#ifndef __HaltSimCumPredRat__
#define __HaltSimCumPredRat__
#include <HOrders/HaltSim.h>
#include <gtlib_signal/QuoteSample.h>
#include <jl_lib/MercatorTrade.h>
#include <vector>

class HaltSimCumPredRat: public HaltSim {
public:
	HaltSimCumPredRat(){}
	HaltSimCumPredRat(int id, double minPrice, int minMsecs,
			int pastSec, int haltLenSec, int minNtrades, double stopRat);
	virtual ~HaltSimCumPredRat(){}
	virtual HaltCondition* getHaltCondition(const std::string& ticker, const std::vector<MercatorTrade>& mts,
			const gtlib::QuoteSample& qSample);
	virtual std::string getDesc();

private:
	int pastSec_;
	int haltLenSec_;
	int minNtrades_;
	double stopRat_;
};

//
// HaltCondition subclass.
//

class HaltConditionCumPredRat: public HaltCondition {
public:
	HaltConditionCumPredRat();
	HaltConditionCumPredRat(const std::string& ticker, const std::vector<MercatorTrade>& mts,
			const gtlib::QuoteSample& qSample, double minPrice, int minMsecs,
			int pastSec, int haltLenSec, int minNtrades, double stopRat, bool debug);
};

#endif
