#ifndef __HaltCondRetWgt__
#define __HaltCondRetWgt__
#include <HOrders/HaltCond.h>
#include <HOrders/HaltSched.h>
#include <gtlib_signal/QuoteSample.h>
#include <jl_lib/MercatorTrade.h>
#include <jl_lib/jlutil.h>
#include <vector>

class HaltCondRetWgt: public HaltCond {
public:
	HaltCondRetWgt(){}
	HaltCondRetWgt(int haltLenSec, int pastMsec, double stopRat, bool debug = false);
	virtual ~HaltCondRetWgt(){}
	virtual void beginDay(int idate);
	virtual void updateSched(HaltSched& hSched, const std::string& ticker,
			const std::vector<MercatorTrade>& mts, const gtlib::QuoteSample& qSample,
			int minMsecs);
	virtual std::string getDesc();

private:
	bool debug_;
	int haltLenSec_;
	int pastMsec_;
	double stopRat_;
	TickersVolat tickersVolat_;
};

#endif
