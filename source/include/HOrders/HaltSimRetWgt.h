#ifndef __HaltSimRetWgt__
#define __HaltSimRetWgt__
#include <HOrders/HaltSim.h>
#include <gtlib_signal/QuoteSample.h>
#include <jl_lib/MercatorTrade.h>
#include <jl_lib/jlutil.h>
#include <vector>

class HaltSimRetWgt: public HaltSim {
public:
	HaltSimRetWgt(){}
	HaltSimRetWgt(int id, double minPrice, int minMsecs,
			int pastMsec, int haltLenSec, double stopRat);
	virtual ~HaltSimRetWgt(){}
	virtual void beginDay(int idate);
	virtual HaltCondition* getHaltCondition(const std::string& ticker,
			const std::vector<MercatorTrade>& mts, const gtlib::QuoteSample& qSample);
	virtual std::string getDesc();

private:
	int pastMsec_;
	int haltLenSec_;
	double stopRat_;
	TickersVolat tickersVolat_;
};

class HaltConditionRetWgt: public HaltCondition {
public:
	HaltConditionRetWgt();
	HaltConditionRetWgt(const std::string& ticker, const std::vector<MercatorTrade>& mts,
			const gtlib::QuoteSample& qSample,
			double minPrice, int minMsecs, int pastMsec, int haltLenSec, double stopRat,
			double volat, bool debug);
};

#endif
