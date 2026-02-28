#include <gtlib_fitting/DecisionTreeRegressionForce.h>
#include <list>
#include <thread>
using namespace std;

namespace gtlib {

DecisionTreeRegressionForce::DecisionTreeRegressionForce(DecisionTreeParam decisionTreeParam, FitData* pFitData,
		shared_ptr<vector<vector<int>>> pSortedIndex, int indexForceCut, int nForceCutLevel)
	:DecisionTreeRegression(decisionTreeParam, pFitData, pSortedIndex),
	indexForceCut_(indexForceCut),
	nForceCutLevel_(nForceCutLevel)
{
}

void DecisionTreeRegressionForce::fit()
{
	fit_initialize();
	int il = 0;
	for( ; il < nForceCutLevel_ && il < maxTreeLevels_; ++il )
	{
		fit_node_stat();
		fit_node_cut_force();
		fit_node_create_force();
	}
	for( ; il < maxTreeLevels_; ++il )
	{
		fit_node_stat();
		fit_node_cut();
		if( !fit_node_create() )
			break;
	}

	// if end > start, compute stats on any nodes which would have been non-terminal
	fit_node_stat();
}

bool DecisionTreeRegressionForce::fit_node_create_force()
{
	// In the beginnig, iNodeBegin_ should be 0, iNodeEnd_ 1.

	int newEnd = iNodeEnd_;
	for( int iNode = iNodeBegin_; iNode < iNodeEnd_; ++iNode )
	{
		shared_ptr<DTNode> z = nodes_[iNode];
		if( newEnd < maxNodes_ )
		{
			int newLevel = z->level + 1;

			z->left = make_shared<DTNode>(newEnd++, newLevel, z);
			nodes_.push_back(z->left);

			z->right = make_shared<DTNode>(newEnd++, newLevel, z);
			nodes_.push_back(z->right);
		}
	}

	//Update data->rPartWhich used to identify which node a data point belongs to.
	for( int ic = 0; ic < nSamplePoints_; ++ic )
	{
		int which = viNode_[ic];
		shared_ptr<DTNode> z = nodes_[which];
		if( z->left == NULL ) // terminal node.
			continue;

		double tmpX = pFitData_->input(z->iInput, ic);
		if( tmpX < z->cut )
			viNode_[ic] = z->left->iNode;
		else
			viNode_[ic] = z->right->iNode;
	}

	iNodeBegin_ = iNodeEnd_;
	iNodeEnd_ = newEnd;
	if( iNodeBegin_ == iNodeEnd_ || iNodeEnd_ >= maxNodes_ )
		return false;
	return true;
}

void DecisionTreeRegressionForce::fit_node_cut_force()
{
	// single thread.
	int iInput = indexForceCut_;
	fit_node_cut_input_force(iInput);

	// Loop nodes and find the input with max(diffMax).
	for( int iNode = iNodeBegin_; iNode < iNodeEnd_; ++iNode )
		fit_node_cut_recombine(iNode);
}

void DecisionTreeRegressionForce::fit_node_cut_input_force(int iInput)
{
	// Initialize vvStat.
	for( int iNode = iNodeBegin_; iNode < iNodeEnd_; ++iNode )
		vvStat_[iInput][iNode].clear();

	// Loop sample points.
	vector<int>& vIndx = (*pSortedIndex_)[iInput];
	for( int ic = 0; ic < nSamplePoints_; ++ic )
	{
		int cIndx = vIndx[ic];
		int iNode = viNode_[cIndx];
		assert( iNode >= iNodeBegin_ );
		DTStat& stat = vvStat_[iInput][iNode];

		const Acc& accTot = vAccTarget_[iNode];
		double tmpX = pFitData_->input(iInput, cIndx);
		if( stat.iInput < 0 && tmpX > stat.tmpXold && stat.acc.n > accTot.n / 2 )
		{
			double diff = TnodeCutFn(stat.acc, accTot, 1);
			stat.leftMax = stat.acc.n;
			stat.diffMax = diff;
			stat.cut = (tmpX + stat.tmpXold) / 2.;
			stat.iInput = iInput;
		}

		//update count y0, sum y1 and sumsqures y2
		stat.tmpXold = tmpX;
		stat.acc.add(pFitData_->target(cIndx));
	}
}

} // namespace gtlib
