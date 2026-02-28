#ifndef __gtlib_signal_HedgeNF__
#define __gtlib_signal_HedgeNF__
#include <gtlib_signal/Hedge.h>
#include <gtlib_signal/TargetHedgerNF.h>
#include <string>
#include <unordered_map>

namespace gtlib {

class HedgeNF: public Hedge {
public:
	HedgeNF(){}
	HedgeNF(int idate);
	virtual ~HedgeNF(){}
	virtual const TargetHedger* getTargetHedger(const std::string& ticker);
	virtual void calculateHedge(int n1sec);
private:
	int idate_;
	void getNFSignals(NorthfieldHedge& nfh, int n1sec);
	std::unordered_map<std::string, TargetHedgerNF> mTargetHedger_;
};

} // namespace gtlib

#endif
