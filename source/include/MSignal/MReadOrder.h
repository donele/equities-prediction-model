#ifndef __MReadOrder__
#define __MReadOrder__
#include <MFramework/MModuleBase.h>
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

class CLASS_DECLSPEC MReadOrder: public MModuleBase {
public:
	MReadOrder(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MReadOrder();

	virtual void beginJob();
	virtual void beginMarketDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endMarketDay();
	virtual void endJob();

private:
	bool debug_;
	std::vector<std::vector<std::string> > orders_;
	std::vector<std::vector<std::string> > trades_;
};

#endif