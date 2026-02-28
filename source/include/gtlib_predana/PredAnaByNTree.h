#ifndef __gtlib_sigread_PredAnaByNTree__
#define __gtlib_sigread_PredAnaByNTree__
#include <gtlib_sigread/SignalBuffer.h>
#include <gtlib_predana/PredWholeDay.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <functional>

// Single Target. Many trees.

namespace gtlib {

class PredAnaByNTree {
public:
	PredAnaByNTree(){}
	PredAnaByNTree(const std::string& baseDir, const std::string& fitDir,
			const std::string& predModel, const std::string& sigModel,
			const std::string& targetName, int udate, const std::vector<int>& testdates, const std::string& fitDesc, float flatFee);
	~PredAnaByNTree(){}
	void anaNTree();
	void anaDay();

private:
	bool useFlatFee_;
	float flatFee_;
	std::string model_;
	std::vector<int> testdates_;
	std::string statDir_;
	std::vector<float> vSprdAdj_; // .
	std::vector<float> vTarget_; // .
	std::vector<std::vector<float>> vvPred_; // .
	std::vector<int> vPredSub_; // .
	std::map<int, std::pair<int, int>> mDayOffset_;

	std::string getOutputPath();
	std::string getOutputPathDay();
	gtlib::SignalBuffer* getBinSigBuf(const std::string& baseDir,
			const std::string& predModel, const std::string& sigModel,
			const std::string& targetName, int udate, int idate, const std::string& fitDesc);
	void getData(gtlib::PredWholeDay* pPredWholeDay);
};

} // namespace gtlib

#endif
