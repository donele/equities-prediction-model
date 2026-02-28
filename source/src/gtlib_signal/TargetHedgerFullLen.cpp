#include <gtlib_signal/TargetHedgerFullLen.h>
#include <vector>
using namespace std;

namespace gtlib {

TargetHedgerFullLen::TargetHedgerFullLen(int nTarget, int n1sec)
	:vvMarketRet_(vector<vector<double>>(nTarget, vector<double>(n1sec, 0.))),
	vMarketRet11Close_(vector<double>(n1sec, 0.)),
	vMarketRet71Close_(vector<double>(n1sec, 0.)),
	vMarketRetClose_(vector<double>(n1sec, 0.)),
	marketRetON_(0.),
	marketRetClcl_(0.)
{
}

void TargetHedgerFullLen::setMarketRet(int iT, int iSec, double mean)
{
	vvMarketRet_[iT][iSec] = mean;
}

void TargetHedgerFullLen::setMarketRet11Close(int iSec, double mean)
{
	vMarketRet11Close_[iSec] = mean;
}

void TargetHedgerFullLen::setMarketRet71Close(int iSec, double mean)
{
	vMarketRet71Close_[iSec] = mean;
}

void TargetHedgerFullLen::setMarketRetClose(int iSec, double mean)
{
	vMarketRetClose_[iSec] = mean;
}

void TargetHedgerFullLen::setMarketRetON(double mean)
{
	marketRetON_ = mean;
}

void TargetHedgerFullLen::setMarketRetClcl(double mean)
{
	marketRetClcl_ = mean;
}

double TargetHedgerFullLen::getHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const
{
	double sumMarketRet = vvMarketRet_[iT][sec];
	return unHedged - sumMarketRet;
}

double TargetHedgerFullLen::getHedgedTarget11Close(double unHedged, int sec) const
{
	double sumMarketRet = vMarketRet11Close_[sec];
	return unHedged - sumMarketRet;
}

double TargetHedgerFullLen::getHedgedTarget71Close(double unHedged, int sec) const
{
	double sumMarketRet = vMarketRet71Close_[sec];
	return unHedged - sumMarketRet;
}

double TargetHedgerFullLen::getHedgedTargetClose(double unHedged, int sec) const
{
	double sumMarketRet = vMarketRetClose_[sec];
	return unHedged - sumMarketRet;
}

double TargetHedgerFullLen::getHedgedTarON(double unHedged) const
{
	double sumMarketRet = marketRetON_;
	return unHedged - sumMarketRet;
}

double TargetHedgerFullLen::getHedgedTarClcl(double unHedged) const
{
	double sumMarketRet = marketRetClcl_;
	return unHedged - sumMarketRet;
}

} // namespace gtlib
