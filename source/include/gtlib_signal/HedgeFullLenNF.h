#ifndef __gtlib_signal_HedgeFullLenNF__
#define __gtlib_signal_HedgeFullLenNF__
#include <gtlib_signal/Hedge.h>
#include <gtlib_signal/TargetHedgerFullLenNF.h>
#include <string>
#include <unordered_map>

namespace gtlib {

class HedgeFullLenNF: public Hedge {
public:
	HedgeFullLenNF(){}
	HedgeFullLenNF(int idate);
	virtual ~HedgeFullLenNF(){}
	virtual const TargetHedger* getTargetHedger(const std::string& ticker);
	virtual void calculateHedge(int n1sec);
private:
	std::unordered_map<std::string, TargetHedgerFullLenNF> mTargetHedger_;
	int idate_;
	void getNFSignals(NorthfieldHedge& nfh, int n1sec);
};

} // namespace gtlib

#endif
