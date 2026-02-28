#ifndef __gtlib_fitana_AnaObjectBySprd__
#define __gtlib_fitana_AnaObjectBySprd__
#include <gtlib_fitana/AnaObject.h>
#include <gtlib_fitana/fitanaObjs.h>

namespace gtlib {

class AnaObjectBySprd: public AnaObject {
public:
	AnaObjectBySprd();
	AnaObjectBySprd(float minSprd, float maxSprd, int nTrees, float fee);
	virtual ~AnaObjectBySprd(){}
	virtual void addData(TargetPred* pTragetPred);
	virtual void writeByDay(std::ostream& os, int idate);
	virtual void writeSumm(std::ostream& os);
	virtual std::pair<int, float> getNtreeMbias();
private:
	float minSprd_;
	float maxSprd_;
	int nTrees_;
	float fee_;
	std::map<int, AnaDayStat> mStat_;
};

} // namespace gtlib

#endif
