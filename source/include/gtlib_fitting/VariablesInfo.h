#ifndef __gtlib_fitting_VariablesInfo__
#define __gtlib_fitting_VariablesInfo__
#include <jl_lib/jlutil.h>
#include <vector>
#include <memory>

namespace gtlib {

struct VariablesInfo {
	std::vector<std::string> inputNames;
	std::vector<std::string> targetNames;
	std::vector<std::string> bmpredNames;
	std::vector<std::string> spectatorNames;
	std::vector<float> targetWeights;
	std::vector<int> inputIndex;
	std::vector<int> targetIndex;
	std::vector<int> spectatorIndex;
	int nTargets();
	int maxHorizon();
	std::string fullTargetName();
};

//struct TargetInfo {
//	TargetInfo(){}
//	TargetInfo(const std::vector<std::string>& tn, const std::vector<float>& tw,
//			const std::vector<std::string>& bpn);
//	int nTargets();
//	int maxHorizon();
//	std::string fullTargetName();
//	std::vector<std::string> targetNames;
//	std::vector<float> targetWeights;
//	std::vector<std::string> bmpredNames;
//};

}
#endif
