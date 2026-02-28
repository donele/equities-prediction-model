#ifndef __HMakePredIndex__
#define __HMakePredIndex__
#include <HLib/HModule.h>
#include <HLib.h>
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

class CLASS_DECLSPEC HMakePredIndex: public HModule {
public:
	HMakePredIndex(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HMakePredIndex();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	int verbose_;
	std::string fitAlg_;
	std::vector<ARMulti> ar_;

	std::vector<QProfile> qp_;
};

#endif