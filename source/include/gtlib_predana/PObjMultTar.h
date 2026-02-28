#ifndef __gtlib_predana_PObjMultTar__
#define __gtlib_predana_PObjMultTar__
#include <gtlib_predana/GsprStatDef.h>
#include <iostream>
#include <vector>

// GsprObjecMultTar is going to replace GsprObjectPartialPred.

namespace gtlib {

class PObjMultTar {
public:
	PObjMultTar(){}
	PObjMultTar(float control, std::vector<float>* vSprdAdj, std::vector<float>* vPredTot,
			std::vector<float>* vPredOm, std::vector<float>* vPredTm,
			std::vector<float>* vTargetTot, std::vector<float>* vTargetOm, std::vector<float>* vTargetTm,
			std::vector<float>* vIntraTar, std::vector<float>* vPrice);
	PObjMultTar(float control,
		std::vector<float>::iterator vSprdAdjBeg, std::vector<float>::iterator vSprdAdjEnd,
		std::vector<float>::iterator vPredTotBeg, std::vector<float>::iterator vPredTotEnd,
		std::vector<float>::iterator vPredOmBeg, std::vector<float>::iterator vPredOmEnd,
		std::vector<float>::iterator vPredTmBeg, std::vector<float>::iterator vPredTmEnd,
		std::vector<float>::iterator vTargetTotBeg, std::vector<float>::iterator vTargetTotEnd,
		std::vector<float>::iterator vTargetOmBeg, std::vector<float>::iterator vTargetOmEnd,
		std::vector<float>::iterator vTargetTmBeg, std::vector<float>::iterator vTargetTmEnd,
		std::vector<float>::iterator vIntraTarBeg, std::vector<float>::iterator vIntraTarEnd,
		std::vector<float>::iterator vPriceBeg, std::vector<float>::iterator vPriceEnd);
	~PObjMultTar();
	void calculate(bool debug = false);
	void print(std::ostream& os);
	void calcPrintThres(std::ostream& os, double thres);
	void calculateThres(double thres, bool debug = false);
	void calculateThres(const std::vector<float>& vThres);
	void printThres(std::ostream& os);
private:
	bool deleteAll_;
	double thres_;
	std::vector<float>* vControl_;
	std::vector<float>* vSprdAdj_;
	std::vector<float>* vPredTot_;
	std::vector<float>* vPredOm_;
	std::vector<float>* vPredTm_;
	std::vector<float>* vTargetTot_;
	std::vector<float>* vTargetOm_;
	std::vector<float>* vTargetTm_;
	std::vector<float>* vIntraTar_;
	std::vector<float>* vPrice_;
	BasicStat bStat_;
	PredStat pStatTot_;
	PredStat pStatOm_;
	PredStat pStatTm_;
	TopBottomStat tbStatTot_;
	TradableStat trdStatTot_;
	TradableStat trdStatOm_;
	TradableStat trdStatTm_;
	GsprStat gStat_;
	GsprStat gStatIntra_;
	GsprStat gStatIntraPrice_;
	RelativeStat rStat_;
};

} // namespace gtlib

#endif
