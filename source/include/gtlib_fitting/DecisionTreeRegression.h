#ifndef __DecisionTreeRegression__
#define __DecisionTreeRegression__
#include <gtlib_fitting/DecisionTree.h>
#include <gtlib_fitting/fittingObjs.h>
#include <vector>
#include <string>
#include <memory>

namespace gtlib {

class DecisionTreeRegression: public DecisionTree {
public:
	DecisionTreeRegression(){};
	DecisionTreeRegression(DecisionTreeParam decisionTreeParam, FitData* pFitData);
	DecisionTreeRegression(DecisionTreeParam decisionTreeParam, FitData* pFitData,
			std::shared_ptr<std::vector<std::vector<int>>> pSortedIndex);
	DecisionTreeRegression(std::vector<std::string>::const_iterator& itFrom, std::vector<std::string>::const_iterator itTo);
	virtual ~DecisionTreeRegression(){};
	virtual void fit();
	virtual void writeModel(const std::string& path);
	virtual double pred(int iSample);
	virtual double pred(const std::vector<float>& input);
	virtual double get_deviance();
	virtual void deviance_improvement(std::vector<double>& stats);
protected:
	int iNodeBegin_;
	int iNodeEnd_;
	//std::shared_ptr<DTNode> make_node(std::vector<std::string>::const_iterator& itFrom, std::vector<std::string>::const_iterator& itTo);
	void fit_initialize();
	bool fit_node_create();
	double traverse_mu(std::shared_ptr<DTNode> pnode, const std::vector<float>& input);
	void traverse_deviance(std::shared_ptr<DTNode> pnode, double& sumDeviance, int& npts);
	void traverse_improvement(std::shared_ptr<DTNode> pnode, std::vector<double>& stats);
	void fit_node_stat();
	void fit_node_cut();
	void fit_node_cut_input(int ir);
	void fit_node_cut_recombine(int in);
	double TnodeCutFn(const Acc& acc, const Acc& accTot, int weirdFactor);
};

}

#endif
