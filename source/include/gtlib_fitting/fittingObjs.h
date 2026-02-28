#ifndef __fittingObjs__
#define __fittingObjs__
#include <jl_lib/jlutil.h>
#include <vector>
#include <memory>

namespace gtlib {

struct BoostParam {
	int nTrees;
	double shrinkage;
};

struct DecisionTreeParam {
	int minPtsNode;
	int maxLeafNodes;
	int maxTreeLevels;
	float maxMu;
};

struct BoostedDecisionTreeParam {
	BoostParam bParam;
	DecisionTreeParam dtParam;
};

struct DTStat {
	DTStat();
	void clear();
	Acc acc;
	int iInput;
	double cut;
	int leftMax;
	double diffMax;
	double tmpXold;
};

struct DTWStat {
	DTWStat();
	void clear();
	WAcc acc;
	int iInput;
	double cut;
	int leftMax;
	double diffMax;
	double tmpXold;
};

struct DTNode {
	DTNode();
	DTNode(double mu_, double deviance_, int indnum_, int iInput_, double cut_);
	DTNode(int iNode_, int level_, std::shared_ptr<DTNode> parent_);
	bool isTerminal();
	int indnum; // number of data points.
	int level;
	int iInput; // index of input variable. -1 if terminal.
	int leftMax; // number of data points in the left child.
	int iNode; // 
	double cut; // cut value.
	double mu; // average target.
	double deviance; // variance * indnum.
	double tstat; // sum of target / stdev
	std::shared_ptr<DTNode> left;
	std::shared_ptr<DTNode> right;
	std::shared_ptr<DTNode> parent;
};

} // namespace gtlib
#endif
