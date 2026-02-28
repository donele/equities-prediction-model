#ifndef __gtlib_signal_HedgeNFadj__
#define __gtlib_signal_HedgeNFadj__
#include <gtlib_signal/Hedge.h>
#include <gtlib_signal/TargetHedgerNFadj.h>
#include <string>
#include <unordered_map>

namespace gtlib {

class HedgeNFadj: public Hedge {
public:
	HedgeNFadj(){}
	HedgeNFadj(int idate);
	virtual ~HedgeNFadj(){}
	virtual const TargetHedger* getTargetHedger(const std::string& ticker);
	virtual void calculateHedge(int n1sec);
private:
	int idate_;
	void getNFSignals(NorthfieldHedge& nfh, int n1sec);
	std::unordered_map<std::string, TargetHedgerNFadj> mTargetHedger_;
};

} // namespace gtlib

#endif
