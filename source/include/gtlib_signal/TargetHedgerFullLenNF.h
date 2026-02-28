#ifndef __gtlib_signal_TargetHedgerFullLenNF__
#define __gtlib_signal_TargetHedgerFullLenNF__
#include <gtlib_signal/TargetHedger.h>
#include <vector>

namespace gtlib {

class TargetHedgerFullLenNF: public TargetHedger {
public:
	TargetHedgerFullLenNF(){}
	TargetHedgerFullLenNF(int nTarget, int n1sec, double bp, double trd, double rst);
	TargetHedgerFullLenNF(int nTarget, int n1sec);
	void setHedgedTarget(int iT, int iSec, double hedgedTarget);
	void setHedgedTarget11Close(int iSec, double hedgedTarget);
	void setHedgedTarget71Close(int iSec, double hedgedTarget);
	void setHedgedTargetClose(int iSec, double hedgedTarget);
	void setHedgedTarON(double hedgedTarget);
	void setHedgedTarClcl(double hedgedTarget);
	virtual double getHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const;
	virtual double getHedgedTarget11Close(double unHedged, int sec) const;
	virtual double getHedgedTarget71Close(double unHedged, int sec) const;
	virtual double getHedgedTargetClose(double unHedged, int sec) const;
	virtual double getHedgedTarON(double unHedged) const;
	virtual double getHedgedTarClcl(double unHedged) const;
	virtual double getNorthBP() const;
	virtual double getNorthTRD() const;
	virtual double getNorthRST() const;
private:
	double northBP_;
	double northTRD_;
	double northRST_;
	std::vector<std::vector<double>> vvHedgedTarget_;
	std::vector<double> vHedgedTarget11Close_;
	std::vector<double> vHedgedTarget71Close_;
	std::vector<double> vHedgedTargetClose_;
	double hedgedTarON_;
	double hedgedTarClcl_;
};

}

#endif
