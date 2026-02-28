#ifndef __BoostedDecisionTree__
#define __BoostedDecisionTree__
#include <gtlib_fitting/DecisionTree.h>
#include <gtlib_fitting/fittingObjs.h>
#include <string>
#include <vector>
#include <memory>

namespace gtlib {

class BoostedDecisionTree: public DecisionTree {
public:
	BoostedDecisionTree();
	BoostedDecisionTree(int nInputFields, const std::string& path);
	BoostedDecisionTree(FitData* fitData, const std::string& path);
	BoostedDecisionTree(FitData* pFitData, const std::string& dbname, const std::string& dbtable, const std::string& mkt, int udate);
	BoostedDecisionTree(BoostedDecisionTreeParam param, FitData* pFitData, const std::string& fitDir, int udate);
	virtual ~BoostedDecisionTree();
	virtual void fit();
	virtual void writeModel(const std::string& path);
	virtual double pred(const std::vector<float>& input){return 0.;}
	virtual double pred(int iSample){return 0.;}
	virtual double get_deviance() {return 0.;}
	virtual void deviance_improvement(std::vector<double>& stats) {return;}
	std::vector<float> getPred(const std::vector<std::vector<float>>& vvInput);
	std::vector<float> getPredSeries(int iSample);
	std::vector<std::string> getPredSeriesNames();
	void setForceCut(std::vector<int>& forceCutIndex, int nForceCutLevel, const std::set<int>& forceCutTrees);
	void setPrune(int reduction);
	void setDecay(float decay);

	enum TreeType {
		plain, wgt, forceCut, prune
	};
private:
	enum TreeType treeType_;
	std::vector<int> indexPreCut_;
	int nPreCut_;
	int udate_;
	int ntrees_;
	double shrinkage_;
	float reduction_;
	float decayFactor_;
	std::set<int> forceCutTrees_;
	std::string fitDir_;
	std::vector<DecisionTree*> vpTrees_;
	FitData* pOriginalFitData_;
	BoostedDecisionTreeParam param_;
	std::vector<double> wts_;

	void createTrees();
	FitData* makeBoostFitData(FitData* pOrignialFitData);
};

} // namespace gtlib

#endif

