#ifndef __gtlib_signal_HedgeFullLen__
#define __gtlib_signal_HedgeFullLen__
#include <gtlib_model/hff.h>
#include <gtlib_signal/Hedge.h>
#include <gtlib_signal/TargetHedgerFullLen.h>
#include <vector>
#include <string>

namespace gtlib {

class HedgeFullLen : public Hedge {
public:
	HedgeFullLen(){}
	virtual ~HedgeFullLen(){}
	virtual const TargetHedger* getTargetHedger(const std::string& ticker);
	virtual void calculateHedge(int n1sec);
protected:
	TargetHedgerFullLen targetHedger_;
};

} // namespace gtlib

#endif
