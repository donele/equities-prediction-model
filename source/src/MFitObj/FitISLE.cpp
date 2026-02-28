#include <MFitObj/FitISLE.h>
#include <MFitObj.h>
#include <string>
#include <vector>
#include <numeric>
//#include <omp.h>
#include <boost/filesystem.hpp>
using namespace std;

FitISLE::FitISLE(int ntrees, double shrinkage, int maxNodes, int minPtsNode, int maxLevels, int nthreads, double step)
:ntrees_(ntrees),
shrinkage_(shrinkage),
step_(step),
udate_(0),
sort_single_thread_(false)
{
	wts_.resize(ntrees);

	for( int i = 0; i < ntrees; ++i )
		dts_.push_back(new DTMT(2 * maxNodes - 1, minPtsNode, maxLevels, nthreads));
}

void FitISLE::setDir(string fitDir, int udate)
{
	udate_ = udate;
	coefDir_ = fitDir + xpf("\\coef");
	statDir_ = fitDir + xpf("\\stat_") + itos(udate);
	predDir_ = fitDir + xpf("\\preds");
	predInsDir_ = fitDir + xpf("\\predsIns");
	mkd(coefDir_);
	mkd(statDir_);
	mkd(predDir_);
	mkd(predInsDir_);
}

void FitISLE::fit(int iTree, vector<vector<float> >* pvvInputTarget, vector<vector<float> >* pvvInputTargetOOS)
{
	cout << "iTree " << iTree << endl;
	nrows_ = pvvInputTarget->size();
	nInput_ = nrows_ - 1;
	int ncols = (*pvvInputTarget)[0].size();
	int ncolsOOS = (*pvvInputTargetOOS)[0].size();

	pvvInputTarget_ = pvvInputTarget;
	pvvInputTargetOOS_ = pvvInputTargetOOS;
	vvInputTargetIndx_.resize(nInput_);
	for( int i = 0; i < nInput_; ++i )
		vvInputTargetIndx_[i].resize(ncols);

	// Make a copy the targets.
	vTarget_.resize(ncols);
	for( int ic = 0; ic < ncols; ++ic )
		vTarget_[ic] = (*pvvInputTarget)[nrows_ - 1][ic];

	// Index the input values.
	sort_input();

	// Set the pointers to the data.
	dts_[iTree]->setData(pvvInputTarget_, pvvInputTargetOOS_, &vvInputTargetIndx_, vTarget_);

	// Residual.
	if( iTree > 0 )
		calcResiduals(iTree);

	// oos performance.
	bool oosEmpty = pvvInputTargetOOS_ == 0 || pvvInputTargetOOS_->empty() || (*pvvInputTargetOOS_)[0].empty();
	if( !oosEmpty && (iTree % 5 == 0 || iTree == ntrees_ - 1) )
	{
		Perf perf(statDir_, ncols, ncolsOOS);
		perf.run(dts_, wts_, iTree, vTarget_, pvvInputTarget_, pvvInputTargetOOS_);
	}

	// Fit.
	dts_[iTree]->fit();
	wts_[iTree] = shrinkage_;

	// Dump.
	char filename[1000];
	sprintf(filename, "%s\\%d_%d.txt", coefDir_.c_str(), udate_, iTree);
	ofstream ofs(xpf(filename).c_str());

	char buf[1000];
	sprintf(buf, "%4s %7s %3s %12s %10s %10s %15s\n", "num", "weight", "var", "varCut<", "nPts", "mu", "deviance");
	ofs << buf;

	dts_[iTree]->dump(ofs, iTree, wts_[iTree]);
}

void FitISLE::postProcess(vector<vector<float> >& vvInputTarget, vector<vector<float> >& vvInputTargetOOS)
{
	// Prepare covariants.
	int ncols = vvInputTarget[0].size();
	int ncolsPostProcess = ncols * 0.8;
	covy_.resize(ntrees_);
	covx_.resize(ntrees_);
	for( int i = 0; i < ntrees_; ++i )
		covx_[i].resize(i + 1);

	// Predictions of each tree.
	vector<vector<double> > vvPred(ntrees_, vector<double>(ncols));

	int it, ic, nthreads, tid;
	int chunk = 4;
// #pragma omp parallel shared(vvPred, nthreads, chunk) private(it, ic, tid)
// 	{
// 		tid = omp_get_thread_num();
// 		if (tid == 0)
// 		{
// 			nthreads = omp_get_num_threads();
// 			printf("Number of threads = %d\n", nthreads);
// 		}
// 		printf("Thread %d starting...\n", tid);
// 
// #pragma omp for schedule(dynamic, chunk)
// 		for( it = 0; it < ntrees_; ++it )
// 		{
// 			for( ic = 0; ic < ncols; ++ic )
// 				vvPred[it][ic] = dts_[it]->get_pred(ic);
// 		}
// 	}
// 
// 	// Covariants 1.
// 	for( int it = 0; it < ntrees_; ++it )
// 	{
// 		Corr corr;
// 		for( int ic = 0; ic < ncolsPostProcess; ++ic )
// 			corr.add(vvInputTarget[nInput_][ic], vvPred[it][ic]);
// 		covy_[it] = corr.cov();
// 	}
// 
// 	// Covariants 2.
// 	for( int it1 = 0; it1 < ntrees_; ++it1 )
// 	{
// 		for( int it2 = 0; it2 <= it1; ++it2 )
// 		{
// 			Corr corr;
// 			for( int ic = 0; ic < ncolsPostProcess; ++ic )
// 				corr.add(vvPred[it1][ic], vvPred[it2][ic]);
// 			covx_[it1][it2] = corr.cov();
// 		}
// 	}

//
// Path construction.
//
	// Initialize the weights.
	for( int it = 0; it < ntrees_; ++it )
		wts_[it] = 0.;

	// Initialize the gradient vector.
	vector<double> vGrad(ntrees_);
	//grad_.resize(ntrees_);
	for( int ig = 0; ig < ntrees_; ++ig )
	{
		vGrad[ig] = covy_[ig];
		for( int it = 0; it < ntrees_; ++it )
		{
			int it1 = -1;
			int it2 = -1;
			(it > ig) ? (it1 = it, it2 = ig) : (it1 = ig, it2 = it);
			vGrad[ig] -= wts_[it] * covx_[it1][it2];
		}
	}

	// Update.
	//step_ = 0.005;
	gradThres_ = 0.6;
	stopThres_ = 1. + 1e-6;
	vector<int> vGradFilter(ntrees_);
	get_gradFilter(vGradFilter, vGrad);

	int nStepDecreased = 0;
	double minPredRisk = max_double_;
	for( int step = 0; step < 100000; ++step )
	{
		// Update the weights.
		for( int it = 0; it < ntrees_; ++it )
		{
			if( vGradFilter[it] == 1 )
				wts_[it] += step_ * vGrad[it];
		}

		// Test.
		if( step % 100 == 0 )
		{
			double predRisk = get_predRisk(ncols, ncolsPostProcess, vvInputTarget, vvPred);
			cout << predRisk << endl;
			if( predRisk > minPredRisk )
			{
				step_ *= 0.5;
				++nStepDecreased;
			}
			else
			{
				step_ *= 1.1;
				nStepDecreased = 0;
			}
			if( nStepDecreased > 10 )
				break;
			if( predRisk < minPredRisk )
				minPredRisk = predRisk;
		}

		// Update gradients.
		for( int ig = 0; ig < ntrees_; ++ig )
		{
			for( int it = 0; it < ntrees_; ++it )
			{
				if( vGradFilter[it] == 1 )
				{
					int it1 = -1;
					int it2 = -1;
					(it > ig) ? (it1 = it, it2 = ig) : (it1 = ig, it2 = it);
					vGrad[ig] -= step_ * vGrad[it] * covx_[it1][it2];
				}
			}
		}
		get_gradFilter(vGradFilter, vGrad);
		int sumFilter = accumulate(vGradFilter.begin(), vGradFilter.end(), 0);
		//cout << sumFilter << endl;
	}

	int ncolsOOS = (*pvvInputTargetOOS_)[0].size();
	Perf perf(statDir_, ncols, ncolsOOS);
	perf.run(dts_, wts_, ntrees_, pvvInputTarget_, pvvInputTargetOOS_, vvPred);
}

void FitISLE::get_gradFilter(vector<int>& vGradFilter, vector<double>& vGrad)
{
	double absMinGrad = fabs(*min_element(vGrad.begin(), vGrad.end()));
	double absMaxGrad = fabs(*max_element(vGrad.begin(), vGrad.end()));
	double gradMax = (absMinGrad > absMaxGrad) ? absMinGrad : absMaxGrad;
	for( int i = 0; i < ntrees_; ++i )
	{
		vGradFilter[i] = 0;
		if( fabs(vGrad[i]) >= gradThres_ * gradMax )
			vGradFilter[i] = 1;
	}
}

double FitISLE::get_predRisk(int ncols, int ncolsPostProcess, vector<vector<float> >& vvInputTarget, vector<vector<double> >& vvPred)
{
	Acc acc;
	for( int ic = ncolsPostProcess; ic < ncols; ++ic )
	{
		double target = vvInputTarget[nInput_][ic];
		double pred = 0.;
		for( int it = 0; it < ntrees_; ++it )
			pred += wts_[it] * vvPred[it][ic];
		double loss = (target - pred) * (target - pred);
		acc.add(loss);
	}
	return acc.mean();
}

void FitISLE::calcResiduals(int iTree)
{
	// Subtract prediction of dts[iTree - 1] from the target.
	int ncols = (*pvvInputTarget_)[0].size();
	for( int ic = 0; ic < ncols; ++ic )
	{
		double pred = 0.;
		for( int it = 0; it < iTree; ++it )
			pred += dts_[it]->get_pred(ic) * wts_[it];
		vTarget_[ic] -= pred;
	}
}

void FitISLE::sort_input()
{
	if( sort_single_thread_ )
	{
		for( int ir = 0; ir < nInput_; ++ir )
			gsl_heapsort_index<int, float>(vvInputTargetIndx_[ir], (*pvvInputTarget_)[ir]);
	}
	else
	{
		list<boost::shared_ptr<boost::thread> > listThread;
		for( int ir = 0; ir < nInput_; ++ir )
		{
			listThread.push_back(
				boost::shared_ptr<boost::thread>(
					new boost::thread(boost::bind(&FitISLE::sort_input, this, ir))
				)
			);
		}

		// join ticker threads.
		for( list<boost::shared_ptr<boost::thread> >::iterator it = listThread.begin(); it != listThread.end(); ++it )
			(*it)->join();
	}
}

void FitISLE::sort_input(int ir)
{
	gsl_heapsort_index<int, float>(vvInputTargetIndx_[ir], (*pvvInputTarget_)[ir]);
}

