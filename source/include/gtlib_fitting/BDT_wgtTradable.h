#ifndef __BDT_wgtTradable__
#define __BDT_wgtTradable__
#include <gtlib_fitting/DecisionTree.h>
#include <gtlib_fitting/fittingObjs.h>
#include <gtlib_fitting/PerfBoost.h>
#include <string>
#include <vector>
#include <memory>

namespace gtlib {

class BDT_wgtTradable: public DecisionTree {
public:
	BDT_wgtTradable();

	// Read model to use
	BDT_wgtTradable(FitData* fitData, const std::string& path);

	// Fitting
	BDT_wgtTradable(BoostedDecisionTreeParam param, FitData* pFitData, const std::string& fitDir,
			int udate, int minTreeWgt, float wgtFacLimit, float maxPredAdjApplyCut, float maxSprdApplyCut, float maxWgt,
			bool constWgt, bool naturalUnit, bool naturalToCost, bool naturalToNegCost, bool debug);
	virtual ~BDT_wgtTradable();

	// Fitting
	virtual void fit();
	virtual void writeModel(const std::string& path);
	virtual double pred(const std::vector<float>& input){return 0.;}
	virtual double pred(int iSample){return 0.;}
	virtual double get_deviance() {return 0.;}
	virtual void deviance_improvement(std::vector<double>& stats) {return;}
	std::vector<float> getPredSeries(int iSample);
	std::vector<std::string> getPredSeriesNames();

private:
	bool debug_;
	bool constWgt_;
	bool naturalUnit_;
	bool naturalToCost_;
	bool naturalToNegCost_;
	int minTreeWgt_;
	int udate_;
	int ntrees_;
	float maxPredAdjApplyCut_;
	float maxSprdApplyCut_;
	float wgtFacLimit_;
	double shrinkage_;
	double maxWgt_;
	std::string fitDir_;
	std::vector<DecisionTree*> vpTrees_;
	FitData* pOriginalFitData_;
	BoostedDecisionTreeParam param_;
	std::vector<double> wts_;
	std::map<int, std::map<char, int>> mDebug_;
	std::vector<float> vWgtFac_;

	void update_weight(PerfBoost& perf, int iTree);
	FitData* makeBoostFitData(FitData* pOrignialFitData);
};

} // namespace gtlib

#endif

