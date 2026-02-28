#ifndef __gtlib_sigread_FastSimNet__
#define __gtlib_sigread_FastSimNet__
#include <gtlib_fastsim/FastSim.h>

namespace gtlib {

class FastSimNet: public FastSim {
public:
	FastSimNet():FastSim(){}
	FastSimNet(const std::string& name, const std::string& baseDir, const std::string& fitDir,
			const std::vector<std::string>& vPredModel, const std::vector<std::string>& vSigModel,
			const std::vector<std::string>& vTargetName, const std::vector<double>& vWgt,
			double restore, double thres, int maxShrTrade, double maxDollarPosNet, double maxDollarPosTicker);
	~FastSimNet(){}

private:
	std::vector<gtlib::SampleData> vSample_;

	virtual void getData(std::vector<PredWholeDay*>& vPredDay,
			const std::map<std::string, std::string>* mTicker2Uid,
			const std::map<std::string, std::vector<double>>* mMid);
	virtual void getIntradayInfo(gtlib::DailySimStat& dss,
		const std::map<std::string, gtlib::CloseInfo>* mClose,
		const std::map<std::string, std::string>* mTicker2Uid);
};

} // namespace gtlib

#endif

