#include <gtlib_fitana/MevStatMaker.h>
#include <vector>
using namespace std;

namespace gtlib {

MevStatMaker::MevStatMaker(const vector<float>& vTarget, const vector<float>& vPred, const vector<float>& vCost, double thres)
{
	int N = vTarget.size();
	for( int i = 0; i < N; ++i )
	{
		double t = vTarget[i];
		double p = vPred[i];
		double c = vCost[i];

		if( p > c + thres )
		{
			accTar_.add(t - c);
			accPred_.add(p - c);
		}
		else if( -p > c + thres )
		{
			accTar_.add(-t - c);
			accPred_.add(-p - c);
		}
	}
}

MevStat MevStatMaker::getMevStat()
{
	MevStat mevs;
	mevs.ntrds = accTar_.n;
	mevs.mev = accTar_.mean();
	mevs.bias = accPred_.mean() - mevs.mev;
	return mevs;
}

} // namespace gtlib
