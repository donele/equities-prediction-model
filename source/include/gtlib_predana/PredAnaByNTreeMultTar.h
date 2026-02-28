#ifndef __gtlib_sigread_PredAnaByNTreeMultTar__
#define __gtlib_sigread_PredAnaByNTreeMultTar__
#include <gtlib_sigread/SignalBuffer.h>
#include <gtlib_predana/PredWholeDay.h>
#include <gtlib_predana/PObjMultTar.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <functional>

// Om and Tm targets. Many trees.

namespace gtlib {

class PredAnaByNTreeMultTar {
public:
	PredAnaByNTreeMultTar():debug_(false){}
	PredAnaByNTreeMultTar(const std::string& baseDir, const std::string& fitDir,
			const std::vector<std::string>& vPredModel, const std::vector<std::string>& vSigModel,
			const std::vector<std::string>& vTargetName, int udate, const std::vector<int>& testdates,
			float omWgt, float tmWgt, const std::string& fitDesc, float flatFee,
			char exch = '\0', bool debug = false, bool volDepThres = false);
	PredAnaByNTreeMultTar(const std::string& baseDir, const std::string& fitDir, const std::string& model,
			const std::string& targetName, int udate, const std::vector<int>& testdates,
			float omWgt, float tmWgt, const std::string& fitDesc, float flatFee);
	~PredAnaByNTreeMultTar(){}
	void anaNTree();
	void anaThres(const std::vector<float>& vConstThres = std::vector<float>());
	void anaDay();

private:
	bool debug_;
	bool useFlatFee_;
	bool volDepThres_;
	char exch_;
	float debugSum_;
	float flatFee_;
	int nOm_;
	int nTm_;
	float omWgt_;
	float tmWgt_;
	std::string model_;
	std::vector<int> testdates_;
	std::vector<std::string> inputNames_;
	std::string statDir_;
	std::map<int, std::pair<int, int>> mDayOffset_;
	std::vector<float> vConstThres_; // .
	std::unordered_map<std::string, float> mTickerVolDepThres_;

	std::vector<float> vSprdAdj_;
	std::vector<float> vPrice_; // . // T
	std::vector<float> vPredTot_; // . // T
	std::vector<float> vTargetTot_; // . // T
	std::vector<float> vTargetOm_; // . // T
	std::vector<float> vTargetTm_; // . // T
	std::vector<float> vIntraTar_; // . // T
	std::vector<std::vector<float>> vvPredOmT_; // nPred, nData // . // T
	std::vector<std::vector<float>> vvPredTmT_; // nPred, nData // . // T
	std::vector<int> vPredSubOm_; // . // T
	std::vector<int> vPredSubTm_; // . // T
	std::vector<float> vThres_; // .

	void init(PredWholeDay* pOm, PredWholeDay* pTm);
	std::vector<float> getPredTot(const std::vector<float>& vPredOm, const std::vector<float>& vPredTm);
	gtlib::SignalBuffer* getBinSigBuf(const std::string& baseDir,
			const std::string& predModel, const std::string& sigModel,
			const std::string& targetName, int udate, int idate, const std::string& fitDesc);
	void getData(gtlib::PredWholeDay* pOm, gtlib::PredWholeDay* pTm, int idate);
	void fetchData(PredWholeDay* pOm, PredWholeDay* pTm, const std::string& ticker, int msecs);

	PObjMultTar getAnaNTree(int iTO, int iTT);
	std::string getOutputPath();

	std::vector<PObjMultTar> getAnaThres(int iTO, int iTT, const std::vector<float> vConstThres);
	std::string getOutputPathThres();
	PObjMultTar getAnaDay(int idate, int iFrom, int iTo);
	std::string getOutputPathDay();
};

} // namespace gtlib

#endif
