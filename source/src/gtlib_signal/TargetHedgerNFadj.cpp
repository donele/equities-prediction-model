#include <gtlib_signal/TargetHedgerNFadj.h>
#include <jl_lib/jlutil.h>
#include <vector>
using namespace std;

namespace gtlib {

TargetHedgerNFadj::TargetHedgerNFadj(int n1sec, double bp, double trd, double rst)
	:northBP_(bp),
	northTRD_(trd),
	northRST_(rst),
	vHedgedTarget_(vector<double>(n1sec, 0.))
{
}

TargetHedgerNFadj::TargetHedgerNFadj(int n1sec)
	:northBP_(0.),
	northTRD_(0.),
	northRST_(0.),
	vHedgedTarget_(vector<double>(n1sec, 0.))
{
}

void TargetHedgerNFadj::setHedgedTarget(int iSec, double hedgedTarget)
{
	vHedgedTarget_[iSec] = hedgedTarget;
}

double TargetHedgerNFadj::getHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const
{
	double sumMarketRet = 0.;
	int iSecMax = vHedgedTarget_.size();
	for( int iSec = sec + lag; iSec < sec + len + lag && iSec < iSecMax - 1; ++iSec )
		sumMarketRet += vHedgedTarget_[iSec];
	return unHedged - sumMarketRet;
}

double TargetHedgerNFadj::getMultHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const
{
	double sumMarketRet = 1.;
	int iSecMax = std::min(static_cast<int>(vHedgedTarget_.size()) - 2, sec + len + lag - 1);
	for( int iSec = sec + lag; iSec <= iSecMax; ++iSec )
		sumMarketRet *= (vHedgedTarget_[iSec] / basis_pts_ + 1.);
	sumMarketRet -= 1.;
	sumMarketRet *= basis_pts_;
	return unHedged - sumMarketRet;
}

double TargetHedgerNFadj::getNorthBP() const
{
	return northBP_;
}

double TargetHedgerNFadj::getNorthTRD() const
{
	return northTRD_;
}

double TargetHedgerNFadj::getNorthRST() const
{
	return northRST_;
}

} // namespace gtlib
