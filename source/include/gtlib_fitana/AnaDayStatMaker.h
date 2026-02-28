#ifndef __gtlib_fitana_AnaDayStatMaker__
#define __gtlib_fitana_AnaDayStatMaker__
#include <gtlib_fitana/fitanaObjs.h>
#include <vector>

namespace gtlib {

class AnaDayStatMaker {
public:
	AnaDayStatMaker();
	~AnaDayStatMaker(){}
	void addData(float target, float bmpred, float pred, float cost);
	AnaDayStat getDayStat();
private:
	std::vector<float> vTarget_;
	std::vector<float> vBmpred_;
	std::vector<float> vPred_;
	std::vector<float> vCost_;
	EvStat getEvStat(const std::vector<float>& vTarget, const std::vector<float>& vPred);
	MevStat getMevStat(const std::vector<float>& vTarget, const std::vector<float>& vPred, const std::vector<float>& vCost);
};

} // namespace gtlib

#endif
