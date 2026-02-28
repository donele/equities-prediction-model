#include <MSignal/IndexInputMaker.h>
#include <vector>
using namespace std;

IndexInputMaker::IndexInputMaker(int lag0, int lagMult)
	:lag0_(lag0),
	lagMult_(lagMult)
{
}

int IndexInputMaker::getNinputs(int length)
{
	int n = 0;
	int endPos = length;
	for( int i = 0; i < length; ++i )
	{
		int beginPos = max(0., endPos - ceil(lag0_ * pow(lagMult_, i) - 0.5));
		++n;
		if( beginPos == 0 )
			break;
		endPos = beginPos;
	}
	return n;
}

void IndexInputMaker::createInput(vector<float>& inputs, int t, const vector<ReturnData>* p, int length)
{
	// (*p)[0] is the return from sec = 0 to sec 1.

	retIt sBeg = p->begin();
	int endPos = t;
	int lowerBoundPos = max(0, t - length);
	for( int i = 0; i < length; ++i )
	{
		int beginPosTemp = endPos - ceil(lag0_ * pow(lagMult_, i) - 0.5);
		int beginPos = max(beginPosTemp, lowerBoundPos);
		retIt from = sBeg + beginPos;
		retIt to = sBeg + endPos;
		float input = sum_returns(from, to);
		inputs[i] = input;
		if( beginPos == lowerBoundPos )
			break;
		endPos = beginPos;
	}
	return;
}

float IndexInputMaker::createTarget(int t, const vector<ReturnData>* p, int horizon)
{
	retIt tBeg = p->begin();
	retIt from = tBeg + t;
	int N = p->size();
	int toOffset = min(t + horizon, N);
	retIt to = tBeg + toOffset;
	return sum_returns(from, to);
}

float IndexInputMaker::sum_returns(vector<ReturnData>::const_iterator& it1, vector<ReturnData>::const_iterator& it2)
{
	float ret = 0.;
	for( ; it1 != it2; ++it1 )
		ret += it1->ret;
	return ret;
}

