#ifndef __MMakePredIndexOLSTest__
#define __MMakePredIndexOLSTest__
#include <MFramework/MModuleBase.h>
#include <MSignal/IndexInputMaker.h>
#include <jl_lib/MovWinLinMod.h>
#include <gtlib_model/hff.h>
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

class CLASS_DECLSPEC MMakePredIndexOLSTest: public MModuleBase {
public:
	MMakePredIndexOLSTest(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MMakePredIndexOLSTest();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

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
	std::vector<Corr> vCorrProd_;
	std::vector<Corr> vCorrProdTarget_;
	std::vector<Corr> vCorrTestTarget_;
	std::vector<Acc> vAccErrProd_;
	std::vector<Acc> vAccErrTest_;
	std::string get_weight_path(const hff::IndexFilter& filter, std::string model, int idate, int lag0, int lagMult);
};

#endif
