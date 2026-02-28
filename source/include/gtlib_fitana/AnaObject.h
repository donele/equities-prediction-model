#ifndef __gtlib_fitana_AnaObject__
#define __gtlib_fitana_AnaObject__
#include <gtlib_fitana/TargetPred.h>
#include <ostream>

namespace gtlib {

class AnaObject {
public:
	AnaObject();
	~AnaObject(){}
	virtual void addData(TargetPred* pTargetPred) = 0;
	virtual void writeByDay(std::ostream& os, int idate) = 0;
	virtual void writeSumm(std::ostream& os) = 0;
	virtual std::pair<int, float> getNtreeMbias(){return std::pair<int, float>(0, 0.);}
	friend std::ostream& operator <<(std::ostream& os, const AnaObject& rhs);
};

} // namespace gtlib

#endif
