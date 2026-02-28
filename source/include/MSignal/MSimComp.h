#ifndef __MSimComp__
#define __MSimComp__
#include <MFramework/MModuleBase.h>
#include <MFramework.h>
#include <map>
#include <vector>
#include <string>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MSimComp: public MModuleBase {
public:
	MSimComp(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MSimComp();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool comp_mm_;
	int interval_;
	std::string event_;
	std::string mmfit_;
	std::map<std::string, std::vector<int> > mSymbolMsecs_;
};

#endif