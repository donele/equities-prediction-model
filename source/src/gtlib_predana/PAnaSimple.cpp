#include <gtlib_predana/PAnaSimple.h>
#include <gtlib_predana/GsprFtns.h>
#include <jl_lib/jlutil.h>
using namespace std;

namespace gtlib {

PAnaSimple::PAnaSimple(const vector<float>* vControl,
		const vector<int>* vMsecs, const vector<float>* vSprdAdj,
		const vector<float>* vPred, const vector<float>* vTarget,
		const vector<float>* vIntraTar, const vector<float>* vFee,
		const vector<float>* vBid, const vector<float>* vAsk,
		const vector<float>* vClosePrc, const vector<int>& vIndx, bool debug)
	:debug_(debug),
	brk_(0),
	thres_(0.),
	maxPos_(0.),
	vMsecs_(vMsecs),
	vControl_(vControl),
	vSprdAdj_(vSprdAdj),
	vPred_(vPred),
	vTarget_(vTarget),
	vIntraTar_(vIntraTar),
	vFee_(vFee),
	vBid_(vBid),
	vAsk_(vAsk),
	vClosePrc_(vClosePrc),
	vIndx_(vIndx)
{
}

void PAnaSimple::calculate(float thres, float brk, float maxPos)
{
	thres_ = thres;
	brk_ = brk;
	maxPos_ = maxPos;
	bStat_ = calculateBasicStat(*vControl_, vIndx_);
	pStat_ = calculatePredStat(*vPred_, *vTarget_, vIndx_);
	tbStat_ = calculateTopBottomStat(*vPred_, *vTarget_, vIndx_);
	trdStat_ = calculateTradableStatSubset(*vMsecs_, *vPred_, *vTarget_, *vSprdAdj_, vIndx_, thres, brk, maxPos_);
	gStat_ = calculateGsprStatSubset(*vMsecs_, *vPred_, *vTarget_, *vSprdAdj_, vIndx_, thres, brk, maxPos_);
	gStatIntra_ = calculateGsprStatSubset(*vMsecs_, *vPred_, *vIntraTar_, *vSprdAdj_, vIndx_, thres, brk, maxPos_, debug_);
	gStatPrice_ = calculateGsprStatSubset(*vMsecs_, *vPred_, *vTarget_, *vSprdAdj_, *vBid_, *vAsk_, vIndx_, thres, brk, maxPos_);
	gStatIntraPrice_ = calculateGsprStatSubset(*vMsecs_, *vPred_, *vIntraTar_, *vSprdAdj_, *vBid_, *vAsk_, vIndx_, thres, brk, maxPos_);
	gStatExactIntra_ = calculateGsprStatExactSubset(*vMsecs_, *vPred_, *vIntraTar_, *vSprdAdj_, *vFee_, *vBid_, *vAsk_, *vClosePrc_, vIndx_, thres, brk, maxPos_);
}

void PAnaSimple::print(ostream& os)
{
	gsprPrintExtend(os, bStat_, pStat_, tbStat_, trdStat_, gStat_, gStatIntra_, gStatPrice_, gStatIntraPrice_, gStatExactIntra_);
}

void PAnaSimple::calculateThres(float thres)
{
	calculate(thres, 0., 0.);
}

void PAnaSimple::printThres(ostream& os)
{
	gsprPrintThres(os, thres_, bStat_, trdStat_, gStat_, gStatIntra_, gStatPrice_, gStatIntraPrice_);
}

void PAnaSimple::calculateBreak(float brk)
{
	calculate(0., brk, 0.);
}

void PAnaSimple::printBreak(ostream& os)
{
	gsprPrintThres(os, brk_, bStat_, trdStat_, gStat_, gStatIntra_, gStatPrice_, gStatIntraPrice_);
}

void PAnaSimple::calculateMaxPos(float maxPos)
{
	calculate(0., 0., maxPos);
}

void PAnaSimple::printMaxPos(ostream& os)
{
	gsprPrintThres(os, maxPos_, bStat_, trdStat_, gStat_, gStatIntra_, gStatPrice_, gStatIntraPrice_);
}

} // namespace gtlib
