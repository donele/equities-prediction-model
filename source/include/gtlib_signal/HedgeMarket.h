#ifndef __gtlib_signal_HedgeMarket__
#define __gtlib_signal_HedgeMarket__
#include <gtlib_signal/Hedge.h>
#include <gtlib_signal/TargetHedgerMarket.h>

namespace gtlib {

class HedgeMarket: public Hedge {
public:
	HedgeMarket(){}
	virtual ~HedgeMarket(){}
	virtual const TargetHedger* getTargetHedger(const std::string& ticker);
	virtual void calculateHedge(int n1sec);
private:
	TargetHedgerMarket targetHedger_;
};

} // namespace gtlib

#endif
