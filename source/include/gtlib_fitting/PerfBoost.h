#ifndef __gtlib_fitting_PerfBoost___
#define __gtlib_fitting_PerfBoost___
#include <gtlib_fitting/fittingObjs.h>
#include <gtlib_fitting/FitData.h>
#include <vector>
#include <memory>

namespace gtlib {

class DecisionTree;
class PerfBoost {
public:
	PerfBoost(){}
	PerfBoost(const std::string& statDir, int nSample);
	void run(const std::vector<DecisionTree*>& dts, const std::vector<double>& wgts,
			int iTree, FitData& originalFitData, FitData& boostFitData);
	std::vector<float> pred_;
	void setCostWgt();

private:
	int lastTree_;
	bool costWgt_;
	std::string filename_;
};

} // namespace gtlib
#endif
