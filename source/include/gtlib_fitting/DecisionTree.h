#ifndef __DecisionTree__
#define __DecisionTree__
#include <gtlib_fitting/fittingObjs.h>
#include <gtlib_fitting/Fitter.h>
#include <gtlib_fitting/FitData.h>
#include <memory>
#include <vector>

namespace gtlib {

class DecisionTree: public Fitter {
public:
	std::shared_ptr<DTNode> pRoot_;
	DecisionTree();
	DecisionTree(int nInputFields);
	DecisionTree(FitData* pFitData);
	DecisionTree(DecisionTreeParam decisionTreeParam, FitData* pFitData);
	DecisionTree(DecisionTreeParam decisionTreeParam, FitData* pFitData, std::shared_ptr<std::vector<std::vector<int>>> pSortedIndex);
	virtual ~DecisionTree();
	virtual double get_deviance() = 0;
	virtual void deviance_improvement(std::vector<double>& stats) = 0;
protected:
	int nInputFields_;
	int nSamplePoints_;
	FitData* pFitData_;
	std::vector<int> viNode_;
	std::vector<Acc> vAccTarget_;
	std::vector<std::vector<DTStat> > vvStat_;
	int maxNodes_;
	int minPtsNode_;
	int maxTreeLevels_;
	float maxMu_;
	std::vector<std::shared_ptr<DTNode>> nodes_;
	std::shared_ptr<std::vector<std::vector<int>>> pSortedIndex_;

	void writeNode(std::ofstream& ofs, std::shared_ptr<DTNode> z, int iTree, double weight);
	void sortInput(int iThread, int nThreadMax);
};

}

#endif
