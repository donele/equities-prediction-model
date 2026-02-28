#include <gtlib_fitting/FitData.h>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
using namespace std;

namespace gtlib {

FitData::FitData(const DailyDataCount& ddCount,
		const vector<string>& inputNames_,
		const vector<string>& spectatorNames_)
	:ddCount_(ddCount),
	inputNames(inputNames_),
	spectatorNames(spectatorNames_),
	pWeight(nullptr)
{
	nInputFields = inputNames.size();
	nSpectators = spectatorNames.size();
	nSamplePoints = ddCount.getNSamplePoints();

	pTarget = new vector<float>(nSamplePoints, 0.);
	pCostBid = new vector<float>(nSamplePoints, 0.);
	pCostAsk = new vector<float>(nSamplePoints, 0.);
	pBmpred = new vector<float>(nSamplePoints, 0.);
	pInput = new vector<vector<float>>(nInputFields, vector<float>(nSamplePoints, 0.));
	pSpectator = new vector<vector<float>>(nSpectators, vector<float>(nSamplePoints, 0.));
}

FitData::~FitData()
{
	delete pTarget;
	delete pBmpred;
	delete pInput;
	delete pSpectator;
	if(pWeight != nullptr)
		delete pWeight;
}

void FitData::cloneTarget()
{
	pTarget = new vector<float>(*pTarget);
}

float FitData::avgCost(int i)
{
	float sprd = spectator("sprd", i);
	float fee = .5*(spectator("costBid", i) + spectator("costAsk", i));
	return .5 * sprd + fee;
}

void FitData::cloneCostWgtTarget()
{
	pTarget = new vector<float>(*pTarget);
	int N = pTarget->size();
	for(int i = 0; i < N; ++i)
	{
		//float sprd = spectator("sprd", i);
		//float fee = .5*(spectator("costBid", i) + spectator("costAsk", i));
		float cost = avgCost(i);
		if(cost > 0.)
			(*pTarget)[i] /= cost;
		else
			(*pTarget)[i] = 0.;
	}
}

DailyDataCount& FitData::getDailyDataCount()
{
	return ddCount_;
}

vector<int> FitData::getIdates()
{
	return ddCount_.getIdates();
}

vector<float> FitData::getSample(int iSample) const
{
	vector<float> v(nInputFields);
	for( int i = 0; i < nInputFields; ++i )
		v[i] = (*pInput)[i][iSample];
	return v;
}

vector<float>& FitData::input(int iInput)
{
	return (*pInput)[iInput];
}

float& FitData::input(int iInput, int iSample)
{
	return (*pInput)[iInput][iSample];
}

float& FitData::costBid(int iSample)
{
	return (*pCostBid)[iSample];
}

float& FitData::costAsk(int iSample)
{
	return (*pCostAsk)[iSample];
}

float& FitData::weight(int iSample)
{
	if(pWeight == nullptr)
		pWeight = new vector<float>(pTarget->size(), 1.);
	return (*pWeight)[iSample];
}

int FitData::getInputIndex(const string& inputName)
{
	int inputIndex = find(begin(inputNames), end(inputNames), inputName) - begin(inputNames);
	if( inputIndex >= inputNames.size() )
		inputIndex = -1;
	return inputIndex;
}

int FitData::getSpectatorIndex(const string& spectatorName)
{
	int spectatorIndex = find(begin(spectatorNames), end(spectatorNames), spectatorName) - begin(spectatorNames);
	if( spectatorIndex >= spectatorNames.size() )
		spectatorIndex = -1;
	return spectatorIndex;
}

float& FitData::spectator(int iSpec, int iSample)
{
	return (*pSpectator)[iSpec][iSample];
}

float& FitData::spectator(const string& name, int iSample)
{
	int iSpec = getSpectatorIndex(name);
	return spectator(iSpec, iSample);
}

float& FitData::target(int iSample)
{
	return (*pTarget)[iSample];
}

float& FitData::bmpred(int iSample)
{
	return (*pBmpred)[iSample];
}

void FitData::printReadSummary()
{
	vector<int> idates = ddCount_.getIdates();
	int ndays = idates.size();
	int idateFrom = *idates.begin();
	int idateTo = *idates.rbegin();
	cout << ndays << " days from " << idateFrom << " to " << idateTo << "\n";
	cout << "input = " << nInputFields << " sample = " << nSamplePoints << "\n";
}

} // namespace gtlib
