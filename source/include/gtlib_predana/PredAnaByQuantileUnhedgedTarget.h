#ifndef __gtlib_sigread_PredAnaByQuantileUnhedgedTarget__
#define __gtlib_sigread_PredAnaByQuantileUnhedgedTarget__
#include <gtlib_sigread/SignalBuffer.h>
#include <gtlib_sigread/SignalWholeDay.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <functional>

namespace gtlib {

class PredAnaByQuantileUnhedgedTarget {
public:
	PredAnaByQuantileUnhedgedTarget(){}
	PredAnaByQuantileUnhedgedTarget(const std::string& baseDir, const std::string& fitDir,
			const std::vector<std::string>& vPredModel, const std::vector<std::string>& vSigModel,
			const std::vector<std::string>& vTargetName, int udate, const std::vector<int>& testdates,
			const std::string& fitDesc, float flatFee);
	PredAnaByQuantileUnhedgedTarget(const std::string& baseDir, const std::string& fitDir, const std::string& model,
			const std::string& targetName, int udate, const std::vector<int>& testdates, const std::string& fitDesc, float flatFee);
	~PredAnaByQuantileUnhedgedTarget(){}
	void anaQuantile(int nBins);

private:
	bool useFlatFee_;
	float flatFee_;
	int nData_;
	std::string model_;
	std::vector<int> testdates_;
	std::vector<std::string> inputNames_;
	std::string statDir_;
	std::string controlVar_;
	std::vector<float> vControl_;
	std::vector<float> vSprd_;
	std::vector<float> vSprdAdj_;
	std::vector<float> vPrice_;
	std::vector<float> vTarget_;
	std::vector<float> vBmpred_;
	std::vector<float> vPred_;

	std::vector<std::string> getTargetReplacementNames(const std::string& targetName);
	std::string getOutputPath();
	gtlib::SignalBuffer* getBinSigBuf(const std::string& baseDir,
			const std::string& predModel, const std::string& sigModel,
			const std::string& targetName, const std::vector<std::string>& vTargetReplacement,
			const std::vector<std::string>& inputNames, int udate, int idate, const std::string& fitDesc);
	void getData(gtlib::SignalWholeDay* pSigWholeDay);
};

} // namespace gtlib

#endif
