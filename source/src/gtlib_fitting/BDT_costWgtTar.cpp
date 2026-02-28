#include <gtlib_fitting/BDT_costWgtTar.h>
#include <gtlib_fitting/DTFtns.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_fitting/DecisionTreeRegression.h>
#include <gtlib_fitting/DecisionTreeRegressionWgt.h>
#include <gtlib_fitting/PerfBoost.h>
#include <memory>
#include <algorithm>
using namespace std;

namespace gtlib {

BDT_costWgtTar::BDT_costWgtTar()
	:udate_(0)
{}

BDT_costWgtTar::BDT_costWgtTar(FitData* pFitData, const string& path)
	:DecisionTree(pFitData)
{
	DTreadModel(path, vpTrees_, wts_);
}

BDT_costWgtTar::BDT_costWgtTar(BoostedDecisionTreeParam param, FitData* pFitData, const string& fitDir,
		int udate, bool debug)
	:DecisionTree(param.dtParam, pFitData),
	param_(param),
	ntrees_(param.bParam.nTrees),
	shrinkage_(param.bParam.shrinkage),
	fitDir_(fitDir),
	debug_(debug),
	udate_(udate)
{
	wts_.resize(ntrees_);
	std::fill(begin(wts_), end(wts_), shrinkage_);

	pOriginalFitData_ = pFitData;
	pFitData_ = makeBoostFitData(pFitData);
}

BDT_costWgtTar::~BDT_costWgtTar()
{
	for( auto pTree : vpTrees_ )
		delete pTree;
}

FitData* BDT_costWgtTar::makeBoostFitData(FitData* pOriginalFitData)
{
	auto p = new FitData(*pOriginalFitData); // Shallow copy of FitData.
	p->cloneCostWgtTarget();
	return p;
}

void BDT_costWgtTar::fit()
{
	// create trees
	for( int i = 0; i < ntrees_; ++i )
		vpTrees_.push_back(new DecisionTreeRegression(param_.dtParam, pFitData_, pSortedIndex_));

	PerfBoost perf(getStatDir(fitDir_, udate_), nSamplePoints_);
	perf.setCostWgt();
	for( int iTree = 0; iTree < ntrees_; ++iTree )
	{
		cout << "iTree " << iTree << endl;
		vpTrees_[iTree]->fit();
		DTupdateTarget(pFitData_, iTree, vpTrees_, wts_);
		if( iTree % 5 == 0 || iTree == ntrees_ - 1 )
			perf.run(vpTrees_, wts_, iTree, *pOriginalFitData_, *pFitData_);
	}
	DTstat(pFitData_, vpTrees_, fitDir_, udate_);
}

void BDT_costWgtTar::writeModel(const string& path)
{
	DTwriteModel(path, vpTrees_, wts_);
}

vector<float> BDT_costWgtTar::getPredSeries(int iSample)
{
	return DTgetPredSeries(iSample, pFitData_, vpTrees_, wts_);
}

vector<string> BDT_costWgtTar::getPredSeriesNames()
{
	int ntrees = vpTrees_.size();
	return DTgetPredSeriesNames(ntrees);
}

} // namespace gtlib
