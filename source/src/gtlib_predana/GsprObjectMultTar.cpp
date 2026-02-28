#include <gtlib_predana/GsprObjectMultTar.h>
#include <gtlib_predana/GsprFtns.h>
using namespace std;

namespace gtlib {

void GsprObjectMultTar::addData(float control, float sprdAdj, float predTot,
		float predOm, float predTm, float targetTot, float targetOm, float targetTm, float intraTar, float price)
{
	if( sprdAdj > 0. )
	{
		vControl_.push_back(control);
		vSprdAdj_.push_back(sprdAdj);
		vPredTot_.push_back(predTot);
		vPredOm_.push_back(predOm);
		vPredTm_.push_back(predTm);
		vTargetTot_.push_back(targetTot);
		vTargetOm_.push_back(targetOm);
		vTargetTm_.push_back(targetTm);
		vIntraTar_.push_back(intraTar);
		vPrice_.push_back(price);
	}
}

void GsprObjectMultTar::calculate()
{
	calculateThres(0.);
}

void GsprObjectMultTar::print(ostream& os)
{
	gsprPrintMultTar(os, bStat_, pStatTot_, pStatOm_, pStatTm_, tbStatTot_, trdStatTot_, trdStatOm_, trdStatTm_, gStat_, gStatIntra_, gStatIntraPrice_);
}

void GsprObjectMultTar::calcPrintThres(ostream& os, double thres)
{
	calculateThres(thres);
	printThres(os);
}

void GsprObjectMultTar::calculateThres(double thres)
{
	thres_ = thres;
	bStat_ = calculateBasicStat(vControl_);
	tbStatTot_ = calculateTopBottomStat(vPredTot_, vTargetTot_);
	trdStatTot_ = calculateTradableStat(vPredTot_, vTargetTot_, vSprdAdj_, thres);
	trdStatOm_ = calculateTradableStat(vPredOm_, vPredTot_, vTargetOm_, vSprdAdj_, thres);
	trdStatTm_ = calculateTradableStat(vPredTm_, vPredTot_, vTargetTm_, vSprdAdj_, thres);
	gStat_ = calculateGsprStat(vPredTot_, vTargetTot_, vSprdAdj_, thres);
	gStatIntra_ = calculateGsprStat(vPredTot_, vIntraTar_, vSprdAdj_, thres);
	gStatIntraPrice_ = calculateGsprStat(vPredTot_, vIntraTar_, vSprdAdj_, vPrice_, vPrice_, thres);
}

void GsprObjectMultTar::printThres(ostream& os)
{
	gsprPrintMultTarThres(os, thres_, bStat_, trdStatTot_, trdStatOm_, trdStatTm_, gStat_, gStatIntra_, gStatIntraPrice_);
}

} // namespace gtlib
