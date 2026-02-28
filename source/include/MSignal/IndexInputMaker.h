#ifndef __IndexInputMaker__
#define __IndexInputMaker__
#include <vector>
#include <optionlibs/TickData.h>

class IndexInputMaker {
	typedef std::vector<ReturnData>::const_iterator retIt;
public:
	IndexInputMaker(){}
	IndexInputMaker(int lag0, int lagMult);
	void createInput(std::vector<float>& input, int t, const std::vector<ReturnData>* p, int length);
	float createTarget(int t, const std::vector<ReturnData>* p, int horizon);
	int getNinputs(int length);
private:
	int lag0_;
	int lagMult_;
	float sum_returns(std::vector<ReturnData>::const_iterator& it1, std::vector<ReturnData>::const_iterator& it2);
};

#endif
