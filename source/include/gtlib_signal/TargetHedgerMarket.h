#ifndef __gtlib_signal_TargeetHegerMarket__
#define __gtlib_signal_TargeetHegerMarket__
#include <gtlib_signal/TargetHedger.h>
#include <vector>

namespace gtlib {

class TargetHedgerMarket: public TargetHedger {
public:
	TargetHedgerMarket(){}
	TargetHedgerMarket(int n1sec);
	void setMarketRet(int iSec, double marketRet);
	virtual double getHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const;
	virtual double getMultHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const;
	virtual double getNorthBP() const {return 0.;}
	virtual double getNorthTRD() const {return 0.;}
	virtual double getNorthRST() const {return 0.;}
private:
	std::vector<double> vMarketRet_;
};

} // namespace gtlib

#endif
