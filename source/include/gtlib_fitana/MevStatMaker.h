#ifndef __gtlib_fitana_MevStatMaker__
#define __gtlib_fitana_MevStatMaker__
#include <gtlib_fitana/fitanaObjs.h>
#include <jl_lib/jlutil.h>
#include <vector>

namespace gtlib {

class MevStatMaker {
public:
	MevStatMaker(){}
	MevStatMaker(const std::vector<float>& vTarget, const std::vector<float>& vPred, const std::vector<float>& vCost, double thres);
	~MevStatMaker(){}
	MevStat getMevStat();
private:
	Acc accTar_;
	Acc accPred_;
};

} // namespace gtlib

#endif
