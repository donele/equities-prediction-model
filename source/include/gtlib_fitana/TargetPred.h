#ifndef __gtlib_fitana_TargetPred__
#define __gtlib_fitana_TargetPred__
#include <vector>
#include <gtlib_fitting/DailyDataCount.h>
#include <gtlib_fitana/PredLine.h>

namespace gtlib {

class TargetPred {
public:
	TargetPred();
	TargetPred(const std::string& fitDir, std::vector<int> testDates, int udate);
	TargetPred(const std::string& fitDir, const std::string& coefFitDir, std::vector<int> testDates, int udate);
	~TargetPred();

	void readPred(const std::string& path, int idate);
	DailyDataCount& getDailyDataCount();
	float& sprd(int iSample);
	float& target(int iSample);
	float& bmpred(int iSample);
	float& pred(int iSample, int iSeries);
	int getNPredSeries();
	std::string getPredLabel(int iSeries);
	int getPredIndex(const std::string& label);
private:
	std::string header_;
	std::vector<std::string> vPredLabel_;
	DailyDataCount ddCount_;
	std::vector<float> vSprd_;
	std::vector<float> vTarget_;
	std::vector<float> vBmpred_;
	std::vector<std::vector<float>> vvPred_;

	void assertHeader(const std::string& header, PredLine& predLine);
	void addLine(PredLine& predLine);
};

} // namespace gtlib

#endif
