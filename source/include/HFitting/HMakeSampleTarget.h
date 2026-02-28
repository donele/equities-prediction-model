#ifndef __HMakeSampleTarget__
#define __HMakeSampleTarget__
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

class CLASS_DECLSPEC HMakeSampleTarget: public HModule {
public:
	HMakeSampleTarget(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HMakeSampleTarget();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;

};

#endif