#include <gtlib_signal/TargetHedgerNF.h>
#include <jl_lib/jlutil.h>
#include <vector>
using namespace std;

namespace gtlib {

TargetHedgerNF::TargetHedgerNF(int n1sec, double bp, double trd, double rst)
	:northBP_(bp),
	northTRD_(trd),
	northRST_(rst),
	vHedgedTarget_(vector<double>(n1sec, 0.))
{
}

TargetHedgerNF::TargetHedgerNF(int n1sec)
	:northBP_(0.),
	northTRD_(0.),
	northRST_(0.),
	vHedgedTarget_(vector<double>(n1sec, 0.))
{
}

void TargetHedgerNF::setHedgedTarget(int iSec, double hedgedTarget)
{
	vHedgedTarget_[iSec] = hedgedTarget;
}

double TargetHedgerNF::getHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const
{
	float ret = 0.;
	int iSecMax = vHedgedTarget_.size();
	for( int iSec = sec + lag; iSec < sec + len + lag && iSec < iSecMax - 1; ++iSec )
		ret += vHedgedTarget_[iSec];
	return ret;
}

double TargetHedgerNF::getMultHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const
{
	double ret = 1.;
	int iSecMax = std::min(static_cast<int>(vHedgedTarget_.size()) - 2, sec + len + lag - 1);
	for( int iSec = sec + lag; iSec <= iSecMax; ++iSec )
		ret *= (vHedgedTarget_[iSec] / basis_pts_ + 1.);
	ret -= 1.;
	ret *= basis_pts_;
	return ret;
}

double TargetHedgerNF::getNorthBP() const
{
	return northBP_;
}

double TargetHedgerNF::getNorthTRD() const
{
	return northTRD_;
}

double TargetHedgerNF::getNorthRST() const
{
	return northRST_;
}

} // namespace gtlib
