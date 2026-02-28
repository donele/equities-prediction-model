#ifndef __gtlib_sigread_PredAnaByQuantile__
#define __gtlib_sigread_PredAnaByQuantile__
#include <gtlib_sigread/SignalBuffer.h>
#include <gtlib_predana/PredWholeDay.h>
#include <gtlib_predana/PAnaSimple.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <functional>

namespace gtlib {

class PredAnaByQuantile {
public:
	PredAnaByQuantile(){}
	PredAnaByQuantile(const std::string& baseDir, const std::string& fitDir,
			const std::vector<std::string>& vPredModel, const std::vector<std::string>& vSigModel,
			const std::vector<std::string>& vTargetName, const std::vector<float>& vWgt,
			int udate, const std::vector<int>& testdates, const std::string& fitDesc, float flatFee,
			bool volDepThres = false, bool tradedTickersOnly = false, bool debug = false);
	PredAnaByQuantile(const std::string& baseDir, const std::string& fitDir, const std::string& model,
			const std::string& targetName, float wgt, int udate,
			const std::vector<int>& testdates, const std::string& fitDesc, float flatFee);
	~PredAnaByQuantile(){}

	void anaQuantileThres(const std::string& var, int nBins);
	void anaQuantileBreak(const std::string& var, int nBins);
	void anaQuantileMaxPos(const std::string& var, int nBins);
	void anaQuantileFeatures(const std::string& var, int nBins, float anaThres, float anaBreak, float anaMaxPos);
	void anaQuantileNTree(const std::string& var, int nBins);
	void anaQuantile(const std::string& var, int nBins);

private:
	bool debug_;
	bool singleThread_;
	bool volDepThres_;
	bool tradedTickersOnly_;
	bool useFlatFee_;
	float flatFee_;
	int nData_;
	std::string model_;
	std::vector<int> testdates_;
	std::string statDir_;

	std::vector<float> vLogVol_;
	std::vector<float> vVolatNorm_;
	std::vector<float> vSprdAdj_;
	std::vector<float> vTarget_;
	std::vector<float> vIntraTar_;
	std::vector<float> vPred_;
	//std::vector<float> vPrice_;
	std::vector<float> vBid_;
	std::vector<float> vAsk_;
	std::vector<float> vTrdPrc_;
	std::vector<float> vClosePrc_;
	std::vector<int> vMsecs_;
	std::vector<float> vFee_;

	gtlib::SignalBuffer* getBinSigBuf(const std::string& baseDir,
			const std::string& predModel, const std::string& sigModel,
			const std::string& targetName, int udate, int idate, const std::string& fitDesc);
	void getData(gtlib::PredWholeDay* pPredWholeDay, int idate);

	const std::vector<float>* getControl(const std::string& var);
	std::vector<int> getSortedIndex(const std::string& var);
	std::vector<int> getIndx(const std::vector<int>& vSortedIndex, int iB, int nBins);
	PAnaSimple getAna(const std::vector<float>* vControl, int iB, int nBins,
			const std::vector<int>& vIndx);
	PAnaSimple getAnaFeatures(const std::vector<float>* vControl, int iB, int nBins,
			const std::vector<int>& vIndx, float anaThres, float anaBreak, float anaMaxPos);

	std::vector<PAnaSimple> getAnaThres(const std::vector<float>* vControl, int iB, int nBins,
		const std::vector<int>& vSortedIndex, const std::vector<float>& vThres);
	std::vector<PAnaSimple> getAnaBreak(const std::vector<float>* vControl, int iB, int nBins,
		const std::vector<int>& vSortedIndex, const std::vector<float>& vBreak);
	std::vector<PAnaSimple> getAnaMaxPos(const std::vector<float>* vControl, int iB, int nBins,
		const std::vector<int>& vSortedIndex, const std::vector<float>& vMaxPos);
	std::string getOutputPath(const std::string& var, std::string desc = "");
};

} // namespace gtlib

#endif
