#include <gtlib_signal/TargetHedgerMarket.h>
#include <jl_lib/jlutil.h>
#include <vector>
using namespace std;

namespace gtlib {

TargetHedgerMarket::TargetHedgerMarket(int n1sec)
	:vMarketRet_(vector<double>(n1sec, 0.))
{
}

void TargetHedgerMarket::setMarketRet(int iSec, double mean)
{
	vMarketRet_[iSec] = mean;
}

double TargetHedgerMarket::getHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const
{
	double sumMarketRet = 0.;
	int iSecMax = vMarketRet_.size();
	for( int iSec = sec + lag; iSec < sec + len + lag && iSec < iSecMax - 1; ++iSec )
		sumMarketRet += vMarketRet_[iSec];
	return unHedged - sumMarketRet;
}

double TargetHedgerMarket::getMultHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const
{
	double sumMarketRet = 1.;
	int iSecMax = std::min(static_cast<int>(vMarketRet_.size()) - 2, sec + len + lag - 1);
	for( int iSec = sec + lag; iSec <= iSecMax; ++iSec )
		sumMarketRet *= (vMarketRet_[iSec] / basis_pts_ + 1.);
	sumMarketRet -= 1.;
	sumMarketRet *= basis_pts_;
	return unHedged - sumMarketRet;
}

} // namespace gtlib
