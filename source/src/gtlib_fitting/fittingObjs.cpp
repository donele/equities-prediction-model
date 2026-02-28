#include <gtlib_fitting/fittingObjs.h>
#include <gtlib_fitting/DecisionTree.h>
#include <string>
using namespace std;

namespace gtlib {

DTStat::DTStat()
	:iInput(-1),
	cut(-max_double_),
	leftMax(0),
	diffMax(-max_double_),
	tmpXold(0.)
{
}

void DTStat::clear()
{
	acc.clear();
	iInput = -1;
	cut = -max_double_;
	leftMax = 0;
	diffMax = -max_double_;
	tmpXold = 0.;
}

DTWStat::DTWStat()
	:iInput(-1),
	cut(-max_double_),
	leftMax(0),
	diffMax(-max_double_),
	tmpXold(0.)
{
}

void DTWStat::clear()
{
	acc.clear();
	iInput = -1;
	cut = -max_double_;
	leftMax = 0;
	diffMax = -max_double_;
	tmpXold = 0.;
}

DTNode::DTNode()
	:indnum(-1),
	iInput(-1),
	leftMax(0),
	cut(0.),
	mu(0.),
	deviance(0.),
	tstat(0.),
	level(0),
	left(nullptr),
	right(nullptr),
	parent(nullptr),
	iNode(0)
{}

DTNode::DTNode(double mu_, double deviance_, int indnum_, int iInput_, double cut_)
	:indnum(indnum_),
	iInput(iInput_),
	cut(cut_),
	mu(mu_),
	deviance(deviance_),
	tstat(0.),
	level(0),
	left(nullptr),
	right(nullptr),
	parent(nullptr),
	iNode(0)
{}

DTNode::DTNode(int iNode_, int level_, std::shared_ptr<DTNode> parent_)
	:indnum(-1),
	iInput(-1),
	cut(0.),
	mu(0.),
	deviance(0.),
	tstat(0.),
	level(level_),
	left(nullptr),
	right(nullptr),
	parent(parent_),
	iNode(iNode_)
{}

bool DTNode::isTerminal()
{
	return left == nullptr && right == nullptr;
}

} // namespace gtlib
