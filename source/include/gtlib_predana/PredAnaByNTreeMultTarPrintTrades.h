#ifndef __gtlib_sigread_PredAnaByNTreeMultTarPrintTrades__
#define __gtlib_sigread_PredAnaByNTreeMultTarPrintTrades__
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

class PredAnaByNTreeMultTarPrintTrades {
public:
	PredAnaByNTreeMultTarPrintTrades():debug_(false){}
	PredAnaByNTreeMultTarPrintTrades(const std::string& baseDir, const std::string& fitDir,
			const std::vector<std::string>& vPredModel, const std::vector<std::string>& vSigModel,
			const std::vector<std::string>& vTargetName, int udate, const std::vector<int>& testdates,
			float omWgt, float tmWgt, const std::string& fitDesc, float flatFee, float thres, bool debug = false);
	PredAnaByNTreeMultTarPrintTrades(const std::string& baseDir, const std::string& fitDir, const std::string& model,
			const std::string& targetName, int udate, const std::vector<int>& testdates,
			float omWgt, float tmWgt, const std::string& fitDesc, float flatFee, float thres);
	~PredAnaByNTreeMultTarPrintTrades(){}
	void printTrades();

private:
	bool debug_;
	bool useFlatFee_;
	float flatFee_;
	float thres_;
	int nOm_;
	int nTm_;
	float omWgt_;
	float tmWgt_;
	std::string model_;
	std::vector<int> testdates_;
	std::vector<std::string> inputNames_;
	std::string statDir_;
	std::ofstream ofs_;
	std::unordered_map<std::string, float> mTickerVolatNorm_;
	std::unordered_map<std::string, float> mTickerClose_;

	gtlib::SignalBuffer* getBinSigBuf(const std::string& baseDir,
			const std::string& predModel, const std::string& sigModel,
			const std::string& targetName, int udate, int idate, const std::string& fitDesc);
	void getData(gtlib::PredWholeDay* pOm, gtlib::PredWholeDay* pTm, int idate);
	void fetchData(PredWholeDay* pOm, PredWholeDay* pTm, int idate, const std::string& ticker, int msecs);

	std::string getOutputPath();
};

} // namespace gtlib

#endif
