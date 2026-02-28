#ifndef __MMakePredIndexOLS__
#define __MMakePredIndexOLS__
#include <MFramework/MModuleBase.h>
#include <MSignal/IndexInputMaker.h>
#include <jl_lib/MovWinLinMod.h>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MMakePredIndexOLS: public MModuleBase {
public:
	MMakePredIndexOLS(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MMakePredIndexOLS();

	virtual void beginJob();
	virtual void beginDay();

	void read_filter();
	void make_prediction();

private:
	bool debug_;
	int verbose_;
	int lag0_;
	int lagMult_;
	std::string fmodel_;
	std::string fitAlg_;
	std::vector<MovWinLinMod> vLinMod_;
	IndexInputMaker iim_;
};

#endif
