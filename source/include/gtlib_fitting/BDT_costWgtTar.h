#ifndef __BDT_costWgtTar__
#define __BDT_costWgtTar__
#include <gtlib_fitting/DecisionTree.h>
#include <gtlib_fitting/fittingObjs.h>
#include <gtlib_fitting/PerfBoost.h>
#include <string>
#include <vector>
#include <memory>

namespace gtlib {

class BDT_costWgtTar: public DecisionTree {
public:
	BDT_costWgtTar();

	// Read model to use
	BDT_costWgtTar(FitData* fitData, const std::string& path);

	// Fitting
	BDT_costWgtTar(BoostedDecisionTreeParam param, FitData* pFitData, const std::string& fitDir,
			int udate, bool debug);
	virtual ~BDT_costWgtTar();

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
	int udate_;
	int ntrees_;
	double shrinkage_;
	std::string fitDir_;
	std::vector<DecisionTree*> vpTrees_;
	FitData* pOriginalFitData_;
	BoostedDecisionTreeParam param_;
	std::vector<double> wts_;

	FitData* makeBoostFitData(FitData* pOrignialFitData);
};

} // namespace gtlib

#endif

