#include <gtlib_signal/TargetHedgerFullLenNF.h>
#include <jl_lib/jlutil.h>
#include <vector>
using namespace std;

namespace gtlib {

TargetHedgerFullLenNF::TargetHedgerFullLenNF(int nTarget, int n1sec, double bp, double trd, double rst)
	:northBP_(bp),
	northTRD_(trd),
	northRST_(rst),
	vvHedgedTarget_(vector<vector<double>>(nTarget, vector<double>(n1sec, 0.))),
	vHedgedTarget11Close_(vector<double>(n1sec, 0.)),
	vHedgedTarget71Close_(vector<double>(n1sec, 0.)),
	vHedgedTargetClose_(vector<double>(n1sec, 0.)),
	hedgedTarON_(0.),
	hedgedTarClcl_(0.)
{
}

TargetHedgerFullLenNF::TargetHedgerFullLenNF(int nTarget, int n1sec)
	:northBP_(0.),
	northTRD_(0.),
	northRST_(0.),
	vvHedgedTarget_(vector<vector<double>>(nTarget, vector<double>(n1sec, 0.))),
	vHedgedTarget11Close_(vector<double>(n1sec, 0.)),
	vHedgedTarget71Close_(vector<double>(n1sec, 0.)),
	vHedgedTargetClose_(vector<double>(n1sec, 0.)),
	hedgedTarON_(0.),
	hedgedTarClcl_(0.)
{
}

void TargetHedgerFullLenNF::setHedgedTarget(int iT, int iSec, double hedgedTarget)
{
	vvHedgedTarget_[iT][iSec] = hedgedTarget;
}

void TargetHedgerFullLenNF::setHedgedTarget11Close(int iSec, double hedgedTarget)
{
	vHedgedTarget11Close_[iSec] = hedgedTarget;
}

void TargetHedgerFullLenNF::setHedgedTarget71Close(int iSec, double hedgedTarget)
{
	vHedgedTarget71Close_[iSec] = hedgedTarget;
}

void TargetHedgerFullLenNF::setHedgedTargetClose(int iSec, double hedgedTarget)
{
	vHedgedTargetClose_[iSec] = hedgedTarget;
}

void TargetHedgerFullLenNF::setHedgedTarON(double hedgedTarget)
{
	hedgedTarON_ = hedgedTarget;
}

void TargetHedgerFullLenNF::setHedgedTarClcl(double hedgedTarget)
{
	hedgedTarClcl_ = hedgedTarget;
}

double TargetHedgerFullLenNF::getHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const
{
	float ret = vvHedgedTarget_[iT][sec];
	return ret;
}

double TargetHedgerFullLenNF::getHedgedTarget11Close(double unHedged, int sec) const
{
	float ret = vHedgedTarget11Close_[sec];
	return ret;
}

double TargetHedgerFullLenNF::getHedgedTarget71Close(double unHedged, int sec) const
{
	float ret = vHedgedTarget71Close_[sec];
	return ret;
}

double TargetHedgerFullLenNF::getHedgedTargetClose(double unHedged, int sec) const
{
	float ret = vHedgedTargetClose_[sec];
	return ret;
}

double TargetHedgerFullLenNF::getHedgedTarON(double unHedged) const
{
	float ret = hedgedTarON_;
	return ret;
}

double TargetHedgerFullLenNF::getHedgedTarClcl(double unHedged) const
{
	float ret = hedgedTarClcl_;
	return ret;
}

double TargetHedgerFullLenNF::getNorthBP() const
{
	return northBP_;
}

double TargetHedgerFullLenNF::getNorthTRD() const
{
	return northTRD_;
}

double TargetHedgerFullLenNF::getNorthRST() const
{
	return northRST_;
}

} // namespace gtlib
