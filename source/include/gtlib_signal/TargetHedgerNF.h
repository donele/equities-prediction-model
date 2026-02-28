#ifndef __gtlib_signal_TargetHedgerNF__
#define __gtlib_signal_TargetHedgerNF__
#include <gtlib_signal/TargetHedger.h>
#include <vector>

namespace gtlib {

class TargetHedgerNF: public TargetHedger {
public:
	TargetHedgerNF(){}
	TargetHedgerNF(int n1sec, double bp, double trd, double rst);
	TargetHedgerNF(int n1sec);
	void setHedgedTarget(int iSec, double hedgedTarget);
	virtual double getHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const;
	virtual double getMultHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const;
	virtual double getNorthBP() const;
	virtual double getNorthTRD() const;
	virtual double getNorthRST() const;
private:
	double northBP_;
	double northTRD_;
	double northRST_;
	std::vector<double> vHedgedTarget_;
};

}

#endif
