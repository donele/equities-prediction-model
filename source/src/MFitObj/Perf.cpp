#include <MFitObj/Perf.h>
#include <MFitObj.h>
using namespace std;

Perf::Perf(const string& statDir, int ncols, int ncolsOOS)
:lastTree_(0)
{
	pred_.resize(ncols);
	predOOS_.resize(ncolsOOS);

	sprintf(filename_, xpf("%s\\perf.txt").c_str(), statDir.c_str());
}

void Perf::run(vector<DTBase*>& dts, vector<double>& wts, int iTree, vector<float>& vTarget, vector<vector<float> >* pvvInputTarget, vector<vector<float> >* pvvInputTargetOOS)
{
	// RMS of residual target.
	double sumResid2 = 0.;
	int ncols = pred_.size();
	int nrows = pvvInputTargetOOS->size();
	for( int ic = 0; ic < ncols; ++ic )
	{
		double target = vTarget[ic];
		sumResid2 += target * target;
	}
	double fitResidRMS = 0.;
	if( ncols > 0 )
		fitResidRMS = sqrt( sumResid2 / ncols );

	int fitPts = ncols;

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
	for( int ic = 0; ic < ncols; ++ic )
	{
		if( iTree > 0 )
		{
			for( int it = lastTree_; it < iTree; ++it )
				pred_[ic] += dts[it]->get_pred(ic) * wgt;
		}
		double pred = pred_[ic];
		double target = (*pvvInputTarget)[nrows - 1][ic];
		corr.add(pred, target);
		accResid.add(target - pred);
	}
	double fitRMS = accResid.RMS();
	double fitCorr = corr.corr();

	// OOS pred.
	int ncolsOOS = predOOS_.size();
	Acc accResidOOS;
	Corr corrOOS;
	for( int ic = 0; ic < ncolsOOS; ++ic )
	{
		if( iTree > 0 )
		{
			for( int it = lastTree_; it < iTree; ++it )
				predOOS_[ic] += dts[it]->get_pred_oos(ic) * wgt;
		}
		double pred = predOOS_[ic];
		double target = (*pvvInputTargetOOS)[nrows - 1][ic];
		corrOOS.add(pred, target);
		accResidOOS.add(target - pred);
	}
	double tstRMS = accResidOOS.RMS();
	double tstCorr = corrOOS.corr();
	int tstPts = ncolsOOS;

	lastTree_ = iTree;

	char buf[1000];
	ofstream ofs;
	if( iTree == 0 )
	{
		ofs.open(filename_);
		sprintf(buf, "%3s %9s %9s %10s %9s %9s %10s %9s %9s\n", "i", "tarRMS", "treeDev", "fitPts", "tstRMS", "tstCorr", "tstPts", "fitRMS", "fitCorr");
		ofs << buf;
	}
	else
		ofs.open(filename_, ios::app);

	sprintf(buf, "%3d %9.5f %9.5f %10d %9.5f %9.5f %10d %9.5f %9.5f\n", iTree, fitResidRMS, treeDeviance, fitPts, tstRMS, tstCorr, tstPts, fitRMS, fitCorr);
	ofs << buf;
}

void Perf::run(vector<DTBase*>& dts, vector<double>& wts, int iTree, vector<vector<float> >* pvvInputTarget, vector<vector<float> >* pvvInputTargetOOS, vector<vector<double> >& vvPred)
{
	// RMS of residual target.
	double sumResid2 = 0.;
	int ncols = pred_.size();
	int nrows = pvvInputTarget->size();
	for( int ic = 0; ic < ncols; ++ic )
	{
		double target = (*pvvInputTarget)[nrows - 1][ic];
		sumResid2 += target * target;
	}
	double fitResidRMS = 0.;
	if( ncols > 0 )
		fitResidRMS = sqrt( sumResid2 / ncols );

	int fitPts = ncols;

	// Deviance of the most recent tree.
	double treeDeviance = 0.;
	
	// pred.
	Acc accResid;
	Corr corr;
	for( int ic = 0; ic < ncols; ++ic )
	{
		double pred = 0.;
		for( int it = lastTree_; it < iTree; ++it )
			pred += vvPred[it][ic] * wts[it];
		double target = (*pvvInputTarget)[nrows - 1][ic];
		corr.add(pred, target);
		accResid.add(target - pred);
	}
	double fitRMS = accResid.RMS();
	double fitCorr = corr.corr();

	// OOS pred.
	int ncolsOOS = predOOS_.size();
	Acc accResidOOS;
	Corr corrOOS;
	for( int ic = 0; ic < ncolsOOS; ++ic )
	{
		double pred = 0.;
		for( int it = lastTree_; it < iTree; ++it )
			pred += dts[it]->get_pred_oos(ic) * wts[it];
		double target = (*pvvInputTargetOOS)[nrows - 1][ic];
		corrOOS.add(pred, target);
		accResidOOS.add(target - pred);
	}
	double tstRMS = accResidOOS.RMS();
	double tstCorr = corrOOS.corr();
	int tstPts = ncolsOOS;

	lastTree_ = iTree;

	char buf[1000];
	ofstream ofs;
	if( iTree == 0 )
	{
		ofs.open(filename_);
		sprintf(buf, "%3s %9s %9s %10s %9s %9s %10s %9s %9s\n", "i", "tarRMS", "treeDev", "fitPts", "tstRMS", "tstCorr", "tstPts", "fitRMS", "fitCorr");
		ofs << buf;
	}
	else
		ofs.open(filename_, ios::app);

	sprintf(buf, "%3d %9.5f %9.5f %10d %9.5f %9.5f %10d %9.5f %9.5f\n", iTree, fitResidRMS, treeDeviance, fitPts, tstRMS, tstCorr, tstPts, fitRMS, fitCorr);
	ofs << buf;
}
