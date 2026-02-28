#include <gtlib_fitana/EvStatMaker.h>
#include <jl_lib/sort_util.h>
#include <vector>
using namespace std;

namespace gtlib {

EvStatMaker::EvStatMaker()
{
}

EvStatMaker::EvStatMaker(const vector<float>& vTarget, const vector<float>& vPred)
	:evQuantile_(.01)
{
	int N = vTarget.size();
	vector<int> vIndex(N);
	gsl_heapsort_index(vIndex, vPred);
	int binSize = evQuantile_ * N;

	if( binSize < N )
	{
		for( int i = 0; i < binSize; ++i )
		{
			int iBottom = vIndex[i];
			int iTop = vIndex[N - i - 1];
			addBottom(vTarget[iBottom], vPred[iBottom]);
			addTop(vTarget[iTop], vPred[iTop]);
		}
	}
}

void EvStatMaker::addBottom(double target, double pred)
{
	accTargetBottom_.add(target);
	accPredBottom_.add(pred);
	return;
}

void EvStatMaker::addTop(double target, double pred)
{
	accTargetTop_.add(target);
	accPredTop_.add(pred);
	return;
}

EvStat EvStatMaker::getEvStat()
{
	EvStat evs;
	evs.npts = accTargetBottom_.n + accTargetTop_.n;
	evs.ev = econVal();
	evs.of = bias();
	evs.malpred = malpred();
	return evs;
}

double EvStatMaker::econVal()
{
	double ret = (accTargetTop_.mean() - accTargetBottom_.mean()) / 2.0;
	return ret;
}

double EvStatMaker::bias()
{
	double biasTop = accPredTop_.mean() - accTargetTop_.mean();
	double biasBottom = accPredBottom_.mean() - accTargetBottom_.mean();
	double ret = (biasTop - biasBottom) / 2.0;
	return ret;
}

double EvStatMaker::malpred()
{
	double ret = (accPredTop_.mean() - accPredBottom_.mean()) / 2.0;
	return ret;
}

} // namespace gtlib
