#include <gtlib_predana/PObjMultTar.h>
#include <gtlib_predana/GsprFtns.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/sort_util.h>
#include <cmath>
#include <algorithm>
using namespace std;

namespace gtlib {

PObjMultTar::PObjMultTar(float control, vector<float>* vSprdAdj,
		vector<float>* vPredTot, vector<float>* vPredOm, vector<float>* vPredTm,
		vector<float>* vTargetTot, vector<float>* vTargetOm, vector<float>* vTargetTm,
		vector<float>* vIntraTar, vector<float>* vPrice)
	:deleteAll_(false),
	thres_(0.),
	vControl_(new vector<float>(vSprdAdj->size(), control)),
	vSprdAdj_(vSprdAdj),
	vPredTot_(vPredTot),
	vPredOm_(vPredOm),
	vPredTm_(vPredTm),
	vTargetTot_(vTargetTot),
	vTargetOm_(vTargetOm),
	vTargetTm_(vTargetTm),
	vIntraTar_(vIntraTar),
	vPrice_(vPrice)
{
}

PObjMultTar::PObjMultTar(float control,
		vector<float>::iterator vSprdAdjBeg, vector<float>::iterator vSprdAdjEnd,
		vector<float>::iterator vPredTotBeg, vector<float>::iterator vPredTotEnd,
		vector<float>::iterator vPredOmBeg, vector<float>::iterator vPredOmEnd,
		vector<float>::iterator vPredTmBeg, vector<float>::iterator vPredTmEnd,
		vector<float>::iterator vTargetTotBeg, vector<float>::iterator vTargetTotEnd,
		vector<float>::iterator vTargetOmBeg, vector<float>::iterator vTargetOmEnd,
		vector<float>::iterator vTargetTmBeg, vector<float>::iterator vTargetTmEnd,
		vector<float>::iterator vIntraTarBeg, vector<float>::iterator vIntraTarEnd,
		vector<float>::iterator vPriceBeg, vector<float>::iterator vPriceEnd)
	:deleteAll_(false),
	thres_(0.),
	vSprdAdj_(new vector<float>(vSprdAdjBeg, vSprdAdjEnd)),
	vPredTot_(new vector<float>(vPredTotBeg, vPredTotEnd)),
	vPredOm_(new vector<float>(vPredOmBeg, vPredOmEnd)),
	vPredTm_(new vector<float>(vPredTmBeg, vPredTmEnd)),
	vTargetTot_(new vector<float>(vTargetTotBeg, vTargetTotEnd)),
	vTargetOm_(new vector<float>(vTargetOmBeg, vTargetOmEnd)),
	vTargetTm_(new vector<float>(vTargetTmBeg, vTargetTmEnd)),
	vIntraTar_(new vector<float>(vIntraTarBeg, vIntraTarEnd)),
	vPrice_(new vector<float>(vPriceBeg, vPriceEnd))
{
	vControl_ = new vector<float>(vSprdAdj_->size(), control);
}

PObjMultTar::~PObjMultTar()
{
	if(deleteAll_)
	{
		delete vSprdAdj_;
		delete vPredTot_;
		delete vPredOm_;
		delete vPredTm_;
		delete vTargetTot_;
		delete vTargetOm_;
		delete vTargetTm_;
		delete vIntraTar_;
		delete vPrice_;
	}
}

void PObjMultTar::calculate(bool debug)
{
	bStat_ = calculateBasicStat(*vControl_);
	pStatTot_ = calculatePredStat(*vPredTot_, *vTargetTot_);
	pStatOm_ = calculatePredStat(*vPredOm_, *vTargetOm_);
	pStatTm_ = calculatePredStat(*vPredTm_, *vTargetTm_);
	tbStatTot_ = calculateTopBottomStat(*vPredTot_, *vTargetTot_);
	trdStatTot_ = calculateTradableStat(*vPredTot_, *vTargetTot_, *vSprdAdj_);
	trdStatOm_ = calculateTradableStat(*vPredOm_, *vPredTot_, *vTargetOm_, *vSprdAdj_);
	trdStatTm_ = calculateTradableStat(*vPredTm_, *vPredTot_, *vTargetTm_, *vSprdAdj_);
	gStat_ = calculateGsprStat(*vPredTot_, *vTargetTot_, *vSprdAdj_);
	gStatIntra_ = calculateGsprStat(*vPredTot_, *vIntraTar_, *vSprdAdj_);
	gStatIntraPrice_ = calculateGsprStat(*vPredTot_, *vIntraTar_, *vSprdAdj_, *vPrice_, *vPrice_, 0., debug);
}

void PObjMultTar::print(ostream& os)
{
	gsprPrintMultTar(os, bStat_, pStatTot_, pStatOm_, pStatTm_, tbStatTot_, trdStatTot_, trdStatOm_, trdStatTm_, gStat_, gStatIntra_, gStatIntraPrice_);
}

void PObjMultTar::calcPrintThres(ostream& os, double thres)
{
	calculateThres(thres);
	printThres(os);
}

void PObjMultTar::calculateThres(const vector<float>& vThres)
{
	bStat_ = calculateBasicStat(*vControl_);
	tbStatTot_ = calculateTopBottomStat(*vPredTot_, *vTargetTot_);
	trdStatTot_ = calculateTradableStat(*vPredTot_, *vTargetTot_, *vSprdAdj_, vThres);
	trdStatOm_ = calculateTradableStat(*vPredOm_, *vPredTot_, *vTargetOm_, *vSprdAdj_, vThres);
	trdStatTm_ = calculateTradableStat(*vPredTm_, *vPredTot_, *vTargetTm_, *vSprdAdj_, vThres);
	gStat_ = calculateGsprStat(*vPredTot_, *vTargetTot_, *vSprdAdj_, vThres);
	gStatIntra_ = calculateGsprStat(*vPredTot_, *vIntraTar_, *vSprdAdj_, vThres);
	gStatIntraPrice_ = calculateGsprStat(*vPredTot_, *vIntraTar_, *vSprdAdj_, *vPrice_, *vPrice_, vThres);
}

void PObjMultTar::calculateThres(double thres, bool debug)
{
	if(debug)
	{
		float po = (*vPredOm_)[0];
		float pt = (*vPredTm_)[0];
		float psum = po + pt;
		int N = vPredTot_->size();
		for(int i = 0; i < N; ++i )
		printf("AAA %d predTot %.4f predOm %.4f predTm %.4f predOm+predTm %.4f\n",
				i,
				 (*vPredTot_)[i]*basis_pts_, (*vPredOm_)[i]*basis_pts_, (*vPredTm_)[i]*basis_pts_, ((*vPredOm_)[i]+(*vPredTm_)[i])*basis_pts_);
	}
	thres_ = thres;
	bStat_ = calculateBasicStat(*vControl_);
	tbStatTot_ = calculateTopBottomStat(*vPredTot_, *vTargetTot_);
	trdStatTot_ = calculateTradableStat(*vPredTot_, *vTargetTot_, *vSprdAdj_, thres);
	trdStatOm_ = calculateTradableStat(*vPredOm_, *vPredTot_, *vTargetOm_, *vSprdAdj_, thres);
	trdStatTm_ = calculateTradableStat(*vPredTm_, *vPredTot_, *vTargetTm_, *vSprdAdj_, thres);
	gStat_ = calculateGsprStat(*vPredTot_, *vTargetTot_, *vSprdAdj_, thres);
	gStatIntra_ = calculateGsprStat(*vPredTot_, *vIntraTar_, *vSprdAdj_, thres, debug);
	gStatIntraPrice_ = calculateGsprStat(*vPredTot_, *vIntraTar_, *vSprdAdj_, *vPrice_, *vPrice_, thres);
}

void PObjMultTar::printThres(ostream& os)
{
	gsprPrintMultTarThres(os, thres_, bStat_, trdStatTot_, trdStatOm_, trdStatTm_, gStat_, gStatIntra_, gStatIntraPrice_);
}

} // namespace gtlib
