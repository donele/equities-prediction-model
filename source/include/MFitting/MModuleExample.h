#ifndef __MModuleExample__
#define __MModuleExample__
#include <MFitting/MModuleExample.h>
#include <MFramework.h>
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

class CLASS_DECLSPEC MModuleExample: public MModuleBase {
public:
	MModuleExample(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MModuleExample();

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