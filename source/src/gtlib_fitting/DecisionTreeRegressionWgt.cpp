#include <gtlib_fitting/DecisionTreeRegressionWgt.h>
#include <list>
#include <thread>
using namespace std;

namespace gtlib {

DecisionTreeRegressionWgt::DecisionTreeRegressionWgt(DecisionTreeParam decisionTreeParam, FitData* pFitData)
	:DecisionTree(decisionTreeParam, pFitData),
	iNodeBegin_(0),
	iNodeEnd_(1)
{
}

DecisionTreeRegressionWgt::DecisionTreeRegressionWgt(DecisionTreeParam decisionTreeParam, FitData* pFitData,
		shared_ptr<vector<vector<int>>> pSortedIndex)
	:DecisionTree(decisionTreeParam, pFitData, pSortedIndex),
	iNodeBegin_(0),
	iNodeEnd_(1)
{
}

DecisionTreeRegressionWgt::DecisionTreeRegressionWgt(vector<string>::const_iterator& itFrom, vector<string>::const_iterator itTo)
{
	pRoot_ = make_node(itFrom, itTo);
}

shared_ptr<DTNode> DecisionTreeRegressionWgt::make_node(vector<string>::const_iterator& itFrom, vector<string>::const_iterator& itTo)
{
	if( itFrom == itTo )
	{
		cerr << "ERROR: Corrupt model.\n";
		exit(12);
	}
	vector<string> thisLine = split(*itFrom);
	++itFrom;

	int iInput = atof(thisLine[2].c_str());
	double cut = atof(thisLine[3].c_str());
	int indnum = atoi(thisLine[4].c_str());
	double mu = atof(thisLine[5].c_str());
	double deviance = atof(thisLine[6].c_str());

	shared_ptr<DTNode> pnode = make_shared<DTNode>(mu, deviance, indnum, iInput, cut);
	if( pnode->iInput > -1 ) // not terminal.
	{
		pnode->left = make_node(itFrom, itTo);
		pnode->right = make_node(itFrom, itTo);
	}

	return pnode;
}

void DecisionTreeRegressionWgt::fit()
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
}

void DecisionTreeRegressionWgt::fit_initialize()
{
	if( pRoot_ != nullptr )
	{
		cerr << "ERROR: Multiple attempts of fitting.\n";
		exit(11);
	}
	pRoot_ = make_shared<DTNode>();
	nodes_.push_back(pRoot_);
	iNodeBegin_ = 0;
	iNodeEnd_ = 1;

	viNode_.resize(nSamplePoints_);
	std::fill(begin(viNode_), end(viNode_), 0.);
	vAccTarget_.resize(maxNodes_);
	vvStat_.resize(nInputFields_);
	for( int i = 0; i < nInputFields_; ++i )
		vvStat_[i].resize(maxNodes_);
}

void DecisionTreeRegressionWgt::fit_node_stat()
{
	// Get mu, deviance, tstat for terminal nodes.

	// Initialize vAcc.
	for( int iNode = iNodeBegin_; iNode < iNodeEnd_; ++iNode )
		vAccTarget_[iNode].clear();

	// Update vAcc.
	for( int iSample = 0; iSample < nSamplePoints_; ++iSample )
	{
		int iNode = viNode_[iSample]; // The data belong to which node.
		if( iNodeBegin_ <= iNode && iNode < iNodeEnd_ )
		{
			double target = pFitData_->target(iSample);
			vAccTarget_[iNode].add(target, pFitData_->weight(iSample));
		}
	}

	// Update mu, deviance, and tstat of the nodes.
	for( int iNode = iNodeBegin_; iNode < iNodeEnd_; ++iNode )
	{
		shared_ptr<DTNode> node = nodes_[iNode];
		const WAcc& acc = vAccTarget_[iNode];

		node->indnum = acc.n;
		if( acc.n >= 2 )
		{
			double mu = acc.mean();
			clip(mu, maxMu_);
			node->mu = mu;
			double var = acc.var();
			node->deviance = var * acc.wsum; // deviance = nPts * variance is minimized.
			node->tstat = (var > 0.) ? acc.sum / sqrt(var) : 0.;
		}
		else
		{
			node->mu = 0.;
			node->deviance = 0.;
			node->tstat = 0.;
		}
	}
}

void DecisionTreeRegressionWgt::fit_node_cut()
{
	// single thread.
	bool singleThread = false;
	if( singleThread )
	{
		for( int iInput = 0; iInput < nInputFields_; ++iInput )
			fit_node_cut_input(iInput);
	}
	else // multi thread. (test result 35 -> 10)
	{
		vector<thread> vt;
		for( int iInput = 0; iInput < nInputFields_; ++iInput )
			vt.push_back(thread(&DecisionTreeRegressionWgt::fit_node_cut_input, this, iInput));
		for( auto& t : vt )
			t.join();
	}

	// Loop nodes and find the input with max(diffMax).
	if( singleThread )
	{
		for( int iNode = iNodeBegin_; iNode < iNodeEnd_; ++iNode )
			fit_node_cut_recombine(iNode);
	}
	else // multi thread. (test result 10 -> 10)
	{
		vector<thread> vt;
		for( int iNode = iNodeBegin_; iNode < iNodeEnd_; ++iNode )
			vt.push_back(thread(&DecisionTreeRegressionWgt::fit_node_cut_recombine, this, iNode));
		for( auto& t : vt )
			t.join();
	}
}

void DecisionTreeRegressionWgt::fit_node_cut_input(int iInput)
{
	// For a given input variable, calculate leftMax, diffMax, cut, iInput for each terminal node.

	// Initialize vvStat.
	for( int iNode = iNodeBegin_; iNode < iNodeEnd_; ++iNode )
		vvStat_[iInput][iNode].clear();

	// Loop sample points.
	vector<int>& vIndx = (*pSortedIndex_)[iInput];
	for( int iSample = 0; iSample < nSamplePoints_; ++iSample )
	{
		int cIndx = vIndx[iSample];
		int iNode = viNode_[cIndx];
		//assert( iNode >= iNodeBegin_ );
		if( iNode < iNodeBegin_ ) // No need to update if not split.
			continue;
		DTWStat& stat = vvStat_[iInput][iNode];

		//If there is a genuine change in the ordered input data, compute TnodeCutFn used to make cutting decision
		//computing diff (the sum of the deviances of the 2 nodes if variable "j" is cut at < tmpX 
		const WAcc& accTot = vAccTarget_[iNode];
		double tmpX = pFitData_->input(iInput, cIndx);
		//if( tmpX > stat.tmpXold )
		if( tmpX > stat.tmpXold && iSample > minPtsNode_ && iSample < nSamplePoints_ - minPtsNode_)
		{
			//TnodeCutFn has returned a new optimal value, so store tnode->cut and tnode->iInput
			double diff = TnodeCutFn(stat.acc, accTot, 1);
			if( diff > stat.diffMax )
			{
				stat.leftMax = stat.acc.n;
				stat.diffMax = diff;
				stat.cut = (tmpX + stat.tmpXold) / 2.;
				stat.iInput = iInput;
			}
		}

		//update count y0, sum y1 and sumsqures y2
		stat.tmpXold = tmpX;
		stat.acc.add(pFitData_->target(cIndx), pFitData_->weight(cIndx));
	}
}

void DecisionTreeRegressionWgt::fit_node_cut_recombine(int iNode)
{
	// For a given node, choose the variable to cut.

	int cutIndex = -1;
	double diffMax = -max_double_;
	for( int iInput = 0; iInput < nInputFields_; ++iInput )
	{
		const DTWStat& stat = vvStat_[iInput][iNode];
		if( stat.diffMax > diffMax )
		{
			cutIndex = iInput;
			diffMax = stat.diffMax;
		}
	}

	if( cutIndex >= 0 )
	{
		const DTWStat& stat = vvStat_[cutIndex][iNode];
		shared_ptr<DTNode> z = nodes_[iNode];
		z->leftMax = stat.leftMax;
		//z->diffMax = stat.diffMax;
		z->cut = stat.cut;
		z->iInput = stat.iInput;
	}
}

double DecisionTreeRegressionWgt::TnodeCutFn(const WAcc& acc, const WAcc& accTot, int weirdFactor)
{
	/*
	   See Hastie et. al. "Elements of Statitical Learning p 270 and
	   Chambers & Hastie. "Statistical models in S" p 414
	   */
	if(acc.n < minPtsNode_ / weirdFactor || acc.n > accTot.n - minPtsNode_ / weirdFactor )
		return(-max_double_);

	double left_var = acc.sum2 - (acc.sum * acc.sum / acc.wsum);
	double right_var = (accTot.sum2 - acc.sum2) - ((accTot.sum - acc.sum) * (accTot.sum - acc.sum) / (accTot.wsum - acc.wsum));

	return(-left_var - right_var);
}

bool DecisionTreeRegressionWgt::fit_node_create()
{
	int newEnd = iNodeEnd_;
	for( int iNode = iNodeBegin_; iNode < iNodeEnd_; ++iNode )
	{
		shared_ptr<DTNode> z = nodes_[iNode];
		if( z->leftMax >= minPtsNode_ && z->leftMax <= z->indnum - minPtsNode_ && newEnd < maxNodes_ )
		{
			int newLevel = z->level + 1;

			z->left = make_shared<DTNode>(newEnd++, newLevel, z);
			nodes_.push_back(z->left);

			z->right = make_shared<DTNode>(newEnd++, newLevel, z);
			nodes_.push_back(z->right);
		}
	}

	//Update data->rPartWhich used to identify which node a data point belongs to.
	for( int iSample = 0; iSample < nSamplePoints_; ++iSample )
	{
		int iNode = viNode_[iSample];
		shared_ptr<DTNode> z = nodes_[iNode];
		if( z->left == NULL ) // terminal node.
			continue;

		double tmpX = pFitData_->input(z->iInput, iSample);
		if( tmpX < z->cut )
			viNode_[iSample] = z->left->iNode;
		else
			viNode_[iSample] = z->right->iNode;
	}

	iNodeBegin_ = iNodeEnd_;
	iNodeEnd_ = newEnd;
	if( iNodeBegin_ == iNodeEnd_ || iNodeEnd_ >= maxNodes_ )
		return false;
	return true;
}

// pred

double DecisionTreeRegressionWgt::pred(int iSample)
{
	vector<float> input = pFitData_->getSample(iSample);
	return pred(input);
}

double DecisionTreeRegressionWgt::pred(const vector<float>& input)
{
	return traverse_mu(pRoot_, input);
}

double DecisionTreeRegressionWgt::traverse_mu(shared_ptr<DTNode> pnode, const vector<float>& input)
{
	if( pnode->left != nullptr )
	{
		if( input[pnode->iInput] < pnode->cut )
			return traverse_mu(pnode->left, input);
		else
			return traverse_mu(pnode->right, input);
	}
	return pnode->mu;
}

// get_deviance()

double DecisionTreeRegressionWgt::get_deviance()
{
	double sumDeviance = 0.;
	int npts = 0;
	traverse_deviance(pRoot_, sumDeviance, npts);

	double avgDeviance = 0.;
	if( npts > 0 )
		avgDeviance = sqrt(sumDeviance / npts);
	return avgDeviance;
}

void DecisionTreeRegressionWgt::traverse_deviance(shared_ptr<DTNode> pnode, double& sumDeviance, int& npts)
{
	if( pnode->left == nullptr || pnode->right == nullptr ) // terminal.
	{
		sumDeviance += pnode->deviance;
		npts += pnode->indnum;
	}
	else
	{
		traverse_deviance(pnode->left, sumDeviance, npts);
		traverse_deviance(pnode->right, sumDeviance, npts);
	}
}

// deviance_improvement()

void DecisionTreeRegressionWgt::deviance_improvement(vector<double>& stats)
{
	traverse_improvement(pRoot_, stats);
}

void DecisionTreeRegressionWgt::traverse_improvement(shared_ptr<DTNode> pnode, vector<double>& stats)
{
	if( pnode != nullptr && pnode->left != nullptr && pnode->right != nullptr )
	{
		double z = pnode->deviance - pnode->left->deviance - pnode->right->deviance;
		stats[pnode->iInput] += z;
		traverse_improvement(pnode->left, stats);
		traverse_improvement(pnode->right, stats);
	}
}

void DecisionTreeRegressionWgt::writeModel(const string& path)
{
}

} // namespace gtlib
