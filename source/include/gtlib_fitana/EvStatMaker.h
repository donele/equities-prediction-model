#ifndef __gtlib_fitana_EvStatMaker__
#define __gtlib_fitana_EvStatMaker__
#include <gtlib_fitana/fitanaObjs.h>
#include <jl_lib/jlutil.h>

namespace gtlib {

class EvStatMaker {
public:
	EvStatMaker();
	EvStatMaker(const std::vector<float>& vTarget, const std::vector<float>& vPred);
	~EvStatMaker(){}
	EvStat getEvStat();
private:
	double evQuantile_;
	Acc accTargetBottom_;
	Acc accPredBottom_;
	Acc accTargetTop_;
	Acc accPredTop_;

	void addBottom(double target, double pred);
	void addTop(double target, double pred);
	double econVal();
	double bias();
	double malpred();
};

} // namespace gtlib

#endif
