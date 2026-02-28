#include <gtlib_fitting/PerfBoost.h>
#include <gtlib_fitting/DecisionTree.h>
#include <boost/filesystem.hpp>
using namespace std;

namespace gtlib {

PerfBoost::PerfBoost(const string& statDir, int nSample)
	:lastTree_(0),
	costWgt_(false)
{
	pred_.resize(nSample);
	filename_ = statDir + "/perf.txt";
	mkd(statDir);
}

void PerfBoost::setCostWgt()
{
	costWgt_ = true;
}

void PerfBoost::run(const vector<DecisionTree*>& dts, const vector<double>& wts, int iTree,
		FitData& originalFitData, FitData& boostFitData)
{
	// RMS of residual target.
	double sumResid2 = 0.;
	int nSample = pred_.size();
	for( int ic = 0; ic < nSample; ++ic )
	{
		double target = boostFitData.target(ic);
		sumResid2 += target * target;
	}
	double fitResidRMS = 0.;
	if( nSample > 0 )
		fitResidRMS = sqrt( sumResid2 / nSample );

	int fitPts = nSample;

	// Deviance of the most recent tree.
	double treeDeviance = 0.;

	if( iTree > 0 )
		treeDeviance = dts[iTree - 1]->get_deviance();

	double wgt = 0.;
	if( iTree > 0 )
		wgt = wts[iTree - 1];

	// pred.
	Acc accResid;
	Corr corr;
	for( int ic = 0; ic < nSample; ++ic )
	{
		if( iTree > 0 )
		{
			for( int it = lastTree_; it < iTree; ++it )
				pred_[ic] += dts[it]->pred(ic) * wgt;
		}
		double pred = 0.;
		if(costWgt_)
		{
			float avgCost = originalFitData.avgCost(ic);
			if(avgCost > 0.)
				pred = pred_[ic] * avgCost;
			else
				pred = 0.;
		}
		else
			pred = pred_[ic];
		double target = originalFitData.target(ic);
		corr.add(pred, target);
		accResid.add(target - pred);
	}
	double fitRMS = accResid.RMS();
	double fitCorr = corr.corr();

	lastTree_ = iTree;

	ofstream ofs;
	char buf[1000];
	if( iTree == 0 ) // header.
	{
		ofs.open(filename_);
		sprintf(buf, "%3s %9s %9s %10s %9s %9s\n", "i", "tarRMS", "treeDev", "fitPts", "fitRMS", "fitCorr");
		ofs << buf;
	}
	else
		ofs.open(filename_, ios::app);

	sprintf(buf, "%3d %9.5f %9.5f %10d %9.5f %9.5f\n", iTree, fitResidRMS, treeDeviance, fitPts, fitRMS, fitCorr);
	ofs << buf;
}

} // namespace gtlib
