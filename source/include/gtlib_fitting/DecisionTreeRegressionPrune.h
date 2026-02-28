#ifndef __DecisionTreeRegressionPrune__
#define __DecisionTreeRegressionPrune__
#include <gtlib_fitting/DecisionTreeRegression.h>
#include <gtlib_fitting/fittingObjs.h>
#include <vector>
#include <string>
#include <memory>

namespace gtlib {

class DecisionTreeRegressionPrune: public DecisionTreeRegression {
public:
	DecisionTreeRegressionPrune(){};
	DecisionTreeRegressionPrune(DecisionTreeParam decisionTreeParam, FitData* pFitData);
	DecisionTreeRegressionPrune(DecisionTreeParam decisionTreeParam, FitData* pFitData,
			std::shared_ptr<std::vector<std::vector<int>>> pSortedIndex, float reduction);
	DecisionTreeRegressionPrune(std::vector<std::string>::const_iterator& itFrom, std::vector<std::string>::const_iterator itTo);
	virtual ~DecisionTreeRegressionPrune(){};
	virtual void fit();
protected:
	float reduction_;
	void fit_node_prune();
};

}

#endif
