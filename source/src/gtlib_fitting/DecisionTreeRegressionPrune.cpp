#include <gtlib_fitting/DecisionTreeRegressionPrune.h>
#include <gtlib_fitting/DTFtns.h>
#include <list>
#include <thread>
using namespace std;

namespace gtlib {

DecisionTreeRegressionPrune::DecisionTreeRegressionPrune(DecisionTreeParam decisionTreeParam, FitData* pFitData)
	:DecisionTreeRegression(decisionTreeParam, pFitData)
{
}

DecisionTreeRegressionPrune::DecisionTreeRegressionPrune(DecisionTreeParam decisionTreeParam, FitData* pFitData,
		shared_ptr<vector<vector<int>>> pSortedIndex, float reduction)
	:DecisionTreeRegression(decisionTreeParam, pFitData, pSortedIndex),
	reduction_(reduction)
{
}

DecisionTreeRegressionPrune::DecisionTreeRegressionPrune(vector<string>::const_iterator& itFrom, vector<string>::const_iterator itTo)
{
	pRoot_ = make_node(itFrom, itTo);
}

void DecisionTreeRegressionPrune::fit()
{
	fit_initialize();
	for( int il = 0; il < maxTreeLevels_; ++il )
	{
		fit_node_stat();
		fit_node_cut();
		if( !fit_node_create() )
			break;
	}

	// if end > start, compute stats on any nodes which would have been non-terminal
	fit_node_stat();

	if( true )
		fit_node_prune();
}

void DecisionTreeRegressionPrune::fit_node_prune()
{
	// Find the nodes that are one level up the terminal nodes and put them in a list ordered by diff.
	multimap<float, int> mDiffInode;
	int nNodes = nodes_.size();
	for( int iNode = 0; iNode < nNodes; ++iNode )
	{
		shared_ptr<DTNode> z = nodes_[iNode];
		if( z->left != nullptr && z->right != nullptr )
		{
			bool leftIsTerminal = z->left->left == nullptr && z->left->right == nullptr;
			bool rightIsTerminal = z->right->left == nullptr && z->right->right == nullptr;
			if( leftIsTerminal && rightIsTerminal )
			{
				float improvement = z->deviance - (z->left->deviance + z->right->deviance);
				mDiffInode.insert(make_pair(improvement, iNode));
			}
		}
	}

	// Prune the nodes according to the order. Update the list.
	// Set left = nullptr, right = nullptr, leftMax = 0, etc.
	int nPrune = nNodes * reduction_ / 2;
	for( int iPrune = 0; iPrune < nPrune; ++iPrune )
	{
		int iNode = mDiffInode.begin()->second;
		shared_ptr<DTNode> z = nodes_[iNode];
		z->iInput = -1;
		z->cut = 0;
		z->leftMax = 0;
		z->left = nullptr;
		z->right = nullptr;
		mDiffInode.erase(mDiffInode.begin());

		if( z->parent->left->isTerminal() && z->parent->right->isTerminal() )
		{
			float improvement = z->parent->deviance - (z->parent->left->deviance + z->parent->right->deviance);
			int iNode = z->parent->iNode;
			mDiffInode.insert(make_pair(improvement, iNode));
		}
	}
}

} // namespace gtlib
