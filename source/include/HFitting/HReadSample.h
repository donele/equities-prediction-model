#ifndef __HReadSample__
#define __HReadSample__
#include <HLib/HModule.h>
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

class CLASS_DECLSPEC HReadSample: public HModule {
public:
	HReadSample(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HReadSample();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool index_;
};

#endif