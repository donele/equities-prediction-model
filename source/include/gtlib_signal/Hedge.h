#ifndef __gtlib_signal_Hedge__
#define __gtlib_signal_Hedge__
#include <vector>
#include <string>
#include <gtlib_signal/TargetHedger.h>
#include <gtlib_model/hff.h>
#include <optionlibs/TickData.h>
#include <boost/thread.hpp>

namespace gtlib {

class Hedge {
public:
	Hedge();
	virtual ~Hedge();

	void updateTicker(const std::string ticker, int inUnivFit,
			double tarON, double tarClcl, int openMsecs, int closeMsecs,
			const TradeInfo& firstTrade, const std::vector<QuoteInfo>& qutoes);
	void updateTicker(const std::string ticker, int inUnivFit,
			double tarON, double tarClcl, int openMsecs, int closeMsecs,
			const TradeInfo& firstTrade, TCM_classic& tcmc);
	void updateTicker(const hff::LinearModel& linMod, const hff::NonLinearModel& nonLinMod,
			const std::string ticker, int inUnivFit, double closePrc, double tarON, double tarClcl,
			const TradeInfo& firstTrade, const std::vector<QuoteInfo>& quotes, bool smoothTarget);

	const std::vector<hff::SigH>* getPvSigH();
	virtual const TargetHedger* getTargetHedger(const std::string& ticker) = 0;
	virtual void calculateHedge(int n1sec) = 0;
protected:
	int nTarget_;
	std::vector<hff::SigH> vSigH_;
	boost::mutex hedge_mutex_;
};

} // namespace gtlib

#endif
