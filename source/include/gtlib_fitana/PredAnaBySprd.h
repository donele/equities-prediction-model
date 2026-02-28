#ifndef __gtlib_fitana_PredAnaBySprd__
#define __gtlib_fitana_PredAnaBySprd__
#include <gtlib_fitana/PredAna.h>
#include <gtlib_fitana/TargetPred.h>
#include <gtlib_fitana/AnaObject.h>
#include <vector>

namespace gtlib {

class PredAnaBySprd: public PredAna {
public:
	PredAnaBySprd();
	PredAnaBySprd(TargetPred* pTargetPred,
			const std::vector<std::pair<float, float>>& vSprdRanges,
			const std::string& fitDir, int udate, float fee);
	virtual ~PredAnaBySprd();
	virtual void analyze();
	virtual void write();
	std::vector<std::pair<int, float>> getNtreeMbias();
private:
	float fee_;
	std::string statDir_;
	TargetPred* pTargetPred_;
	std::vector<std::pair<float, float>> vSprdRanges_;
	std::vector<AnaObject*> vpAnaObj_;
	std::vector<int> idates_;

	void createAnaObjs();
	AnaObject* newAnaObject(int iS, int iP);
	void addDataToAnaObjs();
	int getNTrees(int iPred);
	void writeByDay();
	std::string getByDayPath();
	void writeByDayHeader(std::ostream& os);
	void writeByDayContent(std::ostream& os);
	void writeSumm();
	std::string getSummPath();
	void writeSummHeader(std::ostream& os);
	void writeSummContent(std::ostream& os);
};

} // namespace gtlib

#endif
