#ifndef __MReadBeta__
#define __MReadBeta__
#include <MFramework/MModuleBase.h>
#include <jl_lib/crossCompile.h>
#include <set>
#include <string>

class CLASS_DECLSPEC MReadBeta: public MModuleBase {
public:
	MReadBeta(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MReadBeta();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	int verbose_;
	std::string bmodel_;
	std::set<std::string> indexNames_;
};

#endif
