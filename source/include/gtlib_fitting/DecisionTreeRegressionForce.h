#ifndef __DecisionTreeRegressionForce__
#define __DecisionTreeRegressionForce__
#include <gtlib_fitting/DecisionTreeRegression.h>
#include <gtlib_fitting/fittingObjs.h>
#include <vector>
#include <string>
#include <memory>

namespace gtlib {

class DecisionTreeRegressionForce: public DecisionTreeRegression {
public:
	DecisionTreeRegressionForce(){};
	DecisionTreeRegressionForce(DecisionTreeParam decisionTreeParam, FitData* pFitData,
			std::shared_ptr<std::vector<std::vector<int>>> pSortedIndex, int indexForceCut, int nForceCutLevel);
	virtual ~DecisionTreeRegressionForce(){};
	virtual void fit();
private:
	int indexForceCut_;
	int nForceCutLevel_;
	bool fit_node_create_force();
	void fit_node_cut_force();
	void fit_node_cut_input_force(int ir);
};

}

#endif
