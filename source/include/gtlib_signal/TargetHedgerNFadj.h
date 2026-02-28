#ifndef __gtlib_signal_TargetHedgerNFadj__
#define __gtlib_signal_TargetHedgerNFadj__
#include <gtlib_signal/TargetHedger.h>
#include <vector>

namespace gtlib {

class TargetHedgerNFadj: public TargetHedger {
public:
	TargetHedgerNFadj(){}
	TargetHedgerNFadj(int n1sec, double bp, double trd, double rst);
	TargetHedgerNFadj(int n1sec);
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
