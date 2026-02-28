#ifndef __gtlib_predana_GsprObjectMultTar__
#define __gtlib_predana_GsprObjectMultTar__
#include <gtlib_predana/GsprStatDef.h>
#include <iostream>
#include <vector>

// GsprObjecMultTar is going to replace GsprObjectPartialPred.

namespace gtlib {

class GsprObjectMultTar {
public:
	GsprObjectMultTar():thres_(0.){}
	~GsprObjectMultTar(){}
	void addData(float control, float sprdAdj, float predTot, float predOm, float predTm,
			float targetTot, float targetOm, float targetTm, float intraTar, float price);
	void calculate();
	void print(std::ostream& os);
	void calcPrintThres(std::ostream& os, double thres);
	void calculateThres(double thres);
	void printThres(std::ostream& os);
private:
	double thres_;
	std::vector<float> vControl_;
	std::vector<float> vSprdAdj_;
	std::vector<float> vPredTot_;
	std::vector<float> vPredOm_;
	std::vector<float> vPredTm_;
	std::vector<float> vTargetTot_;
	std::vector<float> vTargetOm_;
	std::vector<float> vTargetTm_;
	std::vector<float> vIntraTar_;
	std::vector<float> vPrice_;
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
