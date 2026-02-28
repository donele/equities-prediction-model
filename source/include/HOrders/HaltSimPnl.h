#ifndef __HaltSimPnl__
#define __HaltSimPnl__
#include <HOrders/HaltSim.h>
#include <gtlib_signal/QuoteSample.h>
#include <jl_lib/MercatorTrade.h>
#include <vector>

class HaltSimPnl: public HaltSim {
public:
	HaltSimPnl(){}
	HaltSimPnl(int id, double minPrice, int minMsecs, int pastSec, int haltLenSec, double stopBpt);
	virtual ~HaltSimPnl(){}
	virtual HaltCondition* getHaltCondition(const std::string& ticker, const std::vector<MercatorTrade>& mts,
			const gtlib::QuoteSample& qSample);
	virtual std::string getDesc();

private:
	int pastSec_;
	int haltLenSec_;
	double stopBpt_;
};

//
// HaltCondition subclass.
//

class HaltConditionPnl: public HaltCondition {
public:
	HaltConditionPnl();
	HaltConditionPnl(const std::string& ticker, const std::vector<MercatorTrade>& mts,
			const gtlib::QuoteSample& qSample, double minPrice, int minMsecs,
			int pastSec, int haltLenSec, double stopBpt, bool debug = false);
};

#endif
