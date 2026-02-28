#include <gtlib_fitting/BDT_wgtTradable.h>
#include <gtlib_fitting/DTFtns.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_fitting/DecisionTreeRegression.h>
#include <gtlib_fitting/DecisionTreeRegressionWgt.h>
#include <gtlib_fitting/PerfBoost.h>
#include <memory>
#include <algorithm>
using namespace std;

namespace gtlib {

BDT_wgtTradable::BDT_wgtTradable()
	:udate_(0)
{}

BDT_wgtTradable::BDT_wgtTradable(FitData* pFitData, const string& path)
	:DecisionTree(pFitData)
{
	DTreadModel(path, vpTrees_, wts_);
}

BDT_wgtTradable::BDT_wgtTradable(BoostedDecisionTreeParam param, FitData* pFitData, const string& fitDir,
		int udate, int minTreeWgt, float wgtFacLimit, float maxPredAdjApplyCut, float maxSprdApplyCut, float maxWgt,
		bool constWgt, bool naturalUnit, bool naturalToCost, bool naturalToNegCost, bool debug)
	:DecisionTree(param.dtParam, pFitData),
	param_(param),
	ntrees_(param.bParam.nTrees),
	shrinkage_(param.bParam.shrinkage),
	fitDir_(fitDir),
	constWgt_(constWgt),
	naturalUnit_(naturalUnit),
	naturalToNegCost_(naturalToNegCost_),
	debug_(debug),
	udate_(udate),
	minTreeWgt_(minTreeWgt),
	wgtFacLimit_(wgtFacLimit),
	maxPredAdjApplyCut_(maxPredAdjApplyCut),
	maxSprdApplyCut_(maxSprdApplyCut),
	maxWgt_(maxWgt)
{
	wts_.resize(ntrees_);
	std::fill(begin(wts_), end(wts_), shrinkage_);

	pOriginalFitData_ = pFitData;
	pFitData_ = makeBoostFitData(pFitData);
}

BDT_wgtTradable::~BDT_wgtTradable()
{
	for( auto pTree : vpTrees_ )
		delete pTree;
}

FitData* BDT_wgtTradable::makeBoostFitData(FitData* pOriginalFitData)
{
	auto p = new FitData(*pOriginalFitData); // Shallow copy of FitData.
	p->cloneTarget();
	return p;
}

void BDT_wgtTradable::fit()
{
	// create trees
	for( int i = 0; i < ntrees_; ++i )
		vpTrees_.push_back(new DecisionTreeRegressionWgt(param_.dtParam, pFitData_, pSortedIndex_));

	PerfBoost perf(getStatDir(fitDir_, udate_), nSamplePoints_);
	for( int iTree = 0; iTree < ntrees_; ++iTree )
	{
		cout << "iTree " << iTree << endl;
		vpTrees_[iTree]->fit();
		perf.run(vpTrees_, wts_, iTree, *pOriginalFitData_, *pFitData_);
		update_weight(perf, iTree);
		DTupdateTarget(pFitData_, iTree, vpTrees_, wts_);
	}
	DTstat(pFitData_, vpTrees_, fitDir_, udate_);

	if(debug_)
	{
		for(auto& kv : mDebug_)
		{
			printf("%4d", kv.first);
			for(auto& kv2 : kv.second)
				printf(" %c %6d", kv2.first, kv2.second);
			printf("\n");
		}
	}
}

void BDT_wgtTradable::update_weight(PerfBoost& perf, int iTree)
{
	float wgtFacLimVal = 1000000;
	if(!vWgtFac_.empty())
	{
		sort(vWgtFac_.begin(), vWgtFac_.end());
		wgtFacLimVal = vWgtFac_[vWgtFac_.size()*wgtFacLimit_];
		vWgtFac_.clear();
	}

	for( int i = 0; i < nSamplePoints_; ++i )
	{
		double wgt = 1.;
		double pred = perf.pred_[i];
		double target = pFitData_->target(i);
		double cost = (target > 0.) ? pFitData_->spectator("costAsk", i) : pFitData_->spectator("costBid", i);
		double sprd = pFitData_->spectator("sprd", i);
		double iTreeFac = (double)iTree / ntrees_;
		if(constWgt_ && sprd < maxSprdApplyCut_)
		{
			wgt = iTreeFac * (maxWgt_ - 1.) + 1.;
		}
		else if(cost > 0. && sprd < maxSprdApplyCut_)
		{
			double wgtFac = 0.;
			double tarAdj = (target > 0.) ? target / cost : -target / cost;
			double predAdj = (target > 0.) ? pred / cost : -pred / cost;
			if(tarAdj > 1. && predAdj < maxPredAdjApplyCut_)
			{
				wgtFac = iTreeFac * tarAdj * abs(predAdj - 1.);
				if(naturalUnit_)
					wgtFac = abs(target - pred);
				else if(naturalToCost_)
					wgtFac = (target > 0.) ? abs(cost - pred) : abs(-cost - pred);
				else if(naturalToNegCost_)
					wgtFac = (target > 0.) ? abs(cost + pred) : abs(cost - pred);
				vWgtFac_.push_back(wgtFac);
				wgt = min(maxWgt_, wgtFac*maxWgt_/wgtFacLimVal + 1);
			}
			//else if(abs(tarAdj) < 1. && abs(predAdj) > 1.)
			//{
			//	wgtFac = abs(predAdj);
			//	if(naturalUnit_)
			//		wgtFac = abs(target - pred);
			//	else if(naturalToCost_)
			//		wgtFac = (target > 0.) ? abs(cost - pred) : abs(-cost - pred);
			//	vWgtFac_.push_back(wgtFac);
			//	wgt = min(maxWgt_, wgtFac*maxWgt_/wgtFacLimVal + 1);
			//}

			if(debug_)
			{
				if(tarAdj > 1. && predAdj < 1.)
				{
					if(predAdj < -1.)
						++mDebug_[iTree]['A'];
					else
						++mDebug_[iTree]['B'];
				}
				else if(abs(target/cost) < 1. && abs(pred/cost) > 1.)
					++mDebug_[iTree]['C'];
				else if(tarAdj > 1. && predAdj > 1.)
					++mDebug_[iTree]['D'];
				else if(tarAdj < 1. && predAdj < 1.)
					++mDebug_[iTree]['E'];
			}
		}
		if(iTree > minTreeWgt_)
			pFitData_->weight(i) = min(wgt, maxWgt_);
	}
}

void BDT_wgtTradable::writeModel(const string& path)
{
	DTwriteModel(path, vpTrees_, wts_);
}

vector<float> BDT_wgtTradable::getPredSeries(int iSample)
{
	return DTgetPredSeries(iSample, pFitData_, vpTrees_, wts_);
}

vector<string> BDT_wgtTradable::getPredSeriesNames()
{
	int ntrees = vpTrees_.size();
	return DTgetPredSeriesNames(ntrees);
}

} // namespace gtlib
