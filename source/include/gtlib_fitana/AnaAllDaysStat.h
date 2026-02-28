#ifndef __gtlib_fitana_AnaAllDaysStat__
#define __gtlib_fitana_AnaAllDaysStat__
#include <gtlib_fitana/fitanaObjs.h>
#include <jl_lib/jlutil.h>
#include <map>

namespace gtlib {

class AnaAllDaysStat {
public:
	AnaAllDaysStat();
	AnaAllDaysStat(const std::map<int, AnaDayStat>& mStat);
	~AnaAllDaysStat(){}
	void addDayStat(const AnaDayStat& ds);
	float getMbias();
private:
	int nDays_;
	AnaDayStat ds_;
	Acc accPnlbm_;
	Acc accPnltb_;
	friend std::ostream& operator<<(std::ostream& os, const AnaAllDaysStat& rhs);
};

} // namespace gtlib

#endif
