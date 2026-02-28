#include <gtlib_fitting/BoostedDecisionTree.h>
#include <gtlib_fitting/DTFtns.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_fitting/DecisionTreeRegression.h>
#include <gtlib_fitting/DecisionTreeRegressionForce.h>
#include <gtlib_fitting/DecisionTreeRegressionPrune.h>
#include <gtlib_fitting/DecisionTreeRegressionTimeWgt.h>
#include <gtlib_fitting/PerfBoost.h>
#include <memory>
#include <algorithm>
using namespace std;

namespace gtlib {

BoostedDecisionTree::BoostedDecisionTree()
	:udate_(0),
	treeType_(plain)
{}

BoostedDecisionTree::BoostedDecisionTree(int nInputFields, const string& path)
	:DecisionTree(nInputFields),
	treeType_(plain)
{
	DTreadModel(path, vpTrees_, wts_);
}

BoostedDecisionTree::BoostedDecisionTree(FitData* pFitData, const string& path)
	:DecisionTree(pFitData),
	treeType_(plain)
{
	DTreadModel(path, vpTrees_, wts_);
}

BoostedDecisionTree::BoostedDecisionTree(FitData* pFitData, const string& dbname, const string& dbtable, const string& mkt, int udate)
	:DecisionTree(pFitData),
	treeType_(plain)
{
	DTreadModel(dbname, dbtable, mkt, udate, vpTrees_, wts_);
}

BoostedDecisionTree::BoostedDecisionTree(BoostedDecisionTreeParam param, FitData* pFitData,
		const string& fitDir, int udate)
	:DecisionTree(param.dtParam, pFitData),
	param_(param),
	ntrees_(param.bParam.nTrees),
	shrinkage_(param.bParam.shrinkage),
	fitDir_(fitDir),
	udate_(udate),
	treeType_(plain)
{
	wts_.resize(ntrees_);
	std::fill(begin(wts_), end(wts_), shrinkage_);

	pOriginalFitData_ = pFitData;
	pFitData_ = makeBoostFitData(pFitData);
}

void BoostedDecisionTree::setForceCut(vector<int>& forceCutIndex, int nForceCutLevel, const set<int>& forceCutTrees)
{
	treeType_ = forceCut;
	indexPreCut_ = forceCutIndex;
	nPreCut_ = nForceCutLevel;
	forceCutTrees_ = forceCutTrees;
	reduction_ = 0.;
}

void BoostedDecisionTree::setPrune(int reduction)
{
	treeType_ = prune;
	reduction_ = reduction;
}

void BoostedDecisionTree::setDecay(float decay)
{
	treeType_ = wgt;
	decayFactor_ = decay; // 1 or more: slow decay. less than 1: fast decay
	printf("decayFactor set to %.4f\n", decay);
}

BoostedDecisionTree::~BoostedDecisionTree()
{
	for( auto pTree : vpTrees_ )
		delete pTree;
}

FitData* BoostedDecisionTree::makeBoostFitData(FitData* pOriginalFitData)
{
	auto p = new FitData(*pOriginalFitData); // Shallow copy of FitData.
	p->cloneTarget();
	return p;
}

void BoostedDecisionTree::fit()
{
	createTrees();
	PerfBoost perf(getStatDir(fitDir_, udate_), nSamplePoints_);
	for( int iTree = 0; iTree < ntrees_; ++iTree )
	{
		cout << "iTree " << iTree << endl;
		vpTrees_[iTree]->fit();
		if( iTree % 5 == 0 || iTree == ntrees_ - 1 )
			perf.run(vpTrees_, wts_, iTree, *pOriginalFitData_, *pFitData_);
		DTupdateTarget(pFitData_, iTree, vpTrees_, wts_);
	}
	DTstat(pFitData_, vpTrees_, fitDir_, udate_);
}

void BoostedDecisionTree::createTrees()
{
	if( treeType_ == plain )
	{
		for( int i = 0; i < ntrees_; ++i )
			vpTrees_.push_back(new DecisionTreeRegression(param_.dtParam, pFitData_, pSortedIndex_));
	}
	else if( treeType_ == wgt )
	{
		for( int i = 0; i < ntrees_; ++i )
			vpTrees_.push_back(new DecisionTreeRegressionTimeWgt(param_.dtParam, pFitData_, pSortedIndex_, decayFactor_));
	}
	else if( treeType_ == prune )
	{
		for( int i = 0; i < ntrees_; ++i )
			vpTrees_.push_back(new DecisionTreeRegressionPrune(param_.dtParam, pFitData_, pSortedIndex_, reduction_));
	}
	else if( treeType_ == forceCut )
	{
		for( int i = 0; i < ntrees_; ++i )
		{
			const int n = indexPreCut_.size();
			if( n > 0 && forceCutTrees_.count(i) )
			{
				vpTrees_.push_back(new DecisionTreeRegressionForce(param_.dtParam, pFitData_, pSortedIndex_,
						indexPreCut_[i%n], nPreCut_));
			}
			else
				vpTrees_.push_back(new DecisionTreeRegression(param_.dtParam, pFitData_, pSortedIndex_));
		}
	}
}

void BoostedDecisionTree::writeModel(const string& path)
{
	DTwriteModel(path, vpTrees_, wts_);
}

vector<float> BoostedDecisionTree::getPred(const vector<vector<float>>& vvInput)
{
	vector<float> vPred;
	for(auto& input : vvInput)
		vPred.push_back(DTgetPred(input, vpTrees_, wts_));
	return vPred;
}

vector<float> BoostedDecisionTree::getPredSeries(int iSample)
{
	return DTgetPredSeries(iSample, pFitData_, vpTrees_, wts_);
}

vector<string> BoostedDecisionTree::getPredSeriesNames()
{
	int ntrees = vpTrees_.size();
	return DTgetPredSeriesNames(ntrees);
}

} // namespace gtlib
