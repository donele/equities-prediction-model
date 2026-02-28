#ifndef __gtlib_fitting_FitData__
#define __gtlib_fitting_FitData__
#include <gtlib_fitting/DailyDataCount.h>
#include <jl_lib/jlutil.h>
#include <vector>
#include <memory>

namespace gtlib {

class FitData {
public:
	FitData(){}
	FitData(const DailyDataCount& ddCount,
			const std::vector<std::string>& inputNames_,
			const std::vector<std::string>& spectatorNames_);
	~FitData();
	void cloneTarget();
	void cloneCostWgtTarget();

	int nInputFields;
	int nSpectators;
	int nSamplePoints;
	std::vector<std::string> inputNames;
	std::vector<std::string> spectatorNames;

	DailyDataCount& getDailyDataCount();
	std::vector<int> getIdates();
	std::vector<float> getSample(int iInput) const;
	std::vector<float>& input(int iInput);
	int getInputIndex(const std::string& inputName);
	int getSpectatorIndex(const std::string& spectatorName);
	float& input(int iInput, int iSample);
	float& costBid(int iSample);
	float& costAsk(int iSample);
	float& weight(int iSample);
	float& spectator(int iInput, int iSample);
	float& spectator(const std::string& name, int iSample);
	float& target(int iSample);
	float& bmpred(int iSample);
	void printReadSummary();
	float avgCost(int i);

private:
	DailyDataCount ddCount_;
	std::vector<float>* pTarget;
	std::vector<float>* pCostBid;
	std::vector<float>* pCostAsk;
	std::vector<float>* pWeight;
	std::vector<float>* pBmpred;;
	std::vector<std::vector<float>>* pInput;
	std::vector<std::vector<float>>* pSpectator;
};

}
#endif
