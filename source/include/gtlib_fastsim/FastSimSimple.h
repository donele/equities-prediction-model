#ifndef __gtlib_sigread_FastSimSimple__
#define __gtlib_sigread_FastSimSimple__
#include <gtlib_fastsim/FastSim.h>

namespace gtlib {

class FastSimSimple: public FastSim {
// Run ticker by ticker. Obsolete.
public:
	~FastSimSimple(){}

private:
	FastSimSimple():FastSim(){}
	FastSimSimple(const std::string& name, const std::string& baseDir, const std::string& fitDir,
			const std::vector<std::string>& vPredModel, const std::vector<std::string>& vSigModel,
			const std::vector<std::string>& vTargetName, const std::vector<double>& vWgt,
			double restore, double thres, int maxShrTrade, double maxDollarPosNet, double maxDollarPosTicker);
	std::map<std::string, std::vector<gtlib::SampleData>> mTickerSample_;

	virtual void getData(std::vector<PredWholeDay*>& vPredDay,
			const std::map<std::string, std::string>* mTicker2Uid,
			const std::map<std::string, std::vector<double>>* mMid);
	virtual void getIntradayInfo(gtlib::DailySimStat& dss,
		const std::map<std::string, gtlib::CloseInfo>* mClose,
		const std::map<std::string, std::string>* mTicker2Uid);
};

} // namespace gtlib

#endif

