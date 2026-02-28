#ifndef __MFitIndexOLS__
#define __MFitIndexOLS__
#include <MFramework/MModuleBase.h>
#include <MFramework.h>
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

class CLASS_DECLSPEC MFitIndexOLS: public MModuleBase {
public:
	MFitIndexOLS(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MFitIndexOLS();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool single_thread_;
	int lag0_;
	int lagMult_;
	int sampleRate_;
	int nDays_;
	std::map<std::string, MovWinLinMod> mLinMod_;
	IndexInputMaker iim_;

	void add_data(int iFilter);
	std::string get_weight_path(const hff::IndexFilter& filter, const std::string& model, int idate);
};

#endif
