#ifndef __gtlib_signal_TargeetHegerFullLen__
#define __gtlib_signal_TargeetHegerFullLen__
#include <gtlib_signal/TargetHedger.h>
#include <vector>

namespace gtlib {

class TargetHedgerFullLen: public TargetHedger {
public:
	TargetHedgerFullLen(){}
	TargetHedgerFullLen(int nTarget, int n1sec);
	void setMarketRet(int iT, int iSec, double marketRet);
	void setMarketRet11Close(int iSec, double marketRet);
	void setMarketRet71Close(int iSec, double marketRet);
	void setMarketRetClose(int iSec, double marketRet);
	void setMarketRetON(double marketRet);
	void setMarketRetClcl(double marketRet);
	virtual double getHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const;
	virtual double getHedgedTarget11Close(double unHedged, int sec) const;
	virtual double getHedgedTarget71Close(double unHedged, int sec) const;
	virtual double getHedgedTargetClose(double unHedged, int sec) const;
	virtual double getHedgedTarON(double unHedged) const;
	virtual double getHedgedTarClcl(double unHedged) const;
	virtual double getNorthBP() const {return 0.;}
	virtual double getNorthTRD() const {return 0.;}
	virtual double getNorthRST() const {return 0.;}
private:
	std::vector<std::vector<double>> vvMarketRet_;
	std::vector<double> vMarketRet11Close_;
	std::vector<double> vMarketRet71Close_;
	std::vector<double> vMarketRetClose_;
	double marketRetON_;
	double marketRetClcl_;
};

} // namespace gtlib

#endif

