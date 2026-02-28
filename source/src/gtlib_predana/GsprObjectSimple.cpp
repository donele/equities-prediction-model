#include <gtlib_predana/GsprObjectSimple.h>
#include <gtlib_predana/GsprFtns.h>
using namespace std;

namespace gtlib {

void GsprObjectSimple::addData(float control, float sprdAdj, float pred, float target)
{
	if( sprdAdj > 0. )
	{
		vControl_.push_back(control);
		vSprdAdj_.push_back(sprdAdj);
		vPred_.push_back(pred);
		vTarget_.push_back(target);
	}
}

void GsprObjectSimple::calculate()
{
	bStat_ = calculateBasicStat(vControl_);
	pStat_ = calculatePredStat(vPred_, vTarget_);
	tbStat_ = calculateTopBottomStat(vPred_, vTarget_);
	trdStat_ = calculateTradableStat(vPred_, vTarget_, vSprdAdj_);
	gStat_ = calculateGsprStat(vPred_, vTarget_, vSprdAdj_);
}

void GsprObjectSimple::print(ostream& os)
{
	gsprPrintSimple(os, bStat_, pStat_, tbStat_, trdStat_, gStat_);
}

} // namespace gtlib
