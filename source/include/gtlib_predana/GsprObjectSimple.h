#ifndef __gtlib_predana_GsprObjectSimple__
#define __gtlib_predana_GsprObjectSimple__
#include <gtlib_predana/GsprStatDef.h>
#include <iostream>
#include <vector>

namespace gtlib {

class GsprObjectSimple {
public:
	GsprObjectSimple(){}
	~GsprObjectSimple(){}
	void addData(float control, float sprdAdj, float pred, float target);
	void calculate();
	void print(std::ostream& os);
private:
	std::vector<float> vControl_;
	std::vector<float> vSprdAdj_;
	std::vector<float> vPred_;
	std::vector<float> vTarget_;
	BasicStat bStat_;
	PredStat pStat_;
	TopBottomStat tbStat_;
	TradableStat trdStat_;
	GsprStat gStat_;
};

} // namespace gtlib

#endif
