#ifndef __MTurnover__
#define __MTurnover__
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>

class CLASS_DECLSPEC MTurnover: public MModuleBase {
public:
	MTurnover(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MTurnover();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool write_debug_;
	bool one_query_;
	int nTargets_;
	std::string sigType_;
	std::string tablename_;
	std::string targetName_;
	std::vector<std::string> targetNames_;
	std::vector<double> targetWeights_;

	int nTrees_;
	std::string fitDesc_;
	std::string fitDir_;

	void write_trees();
	int getDBDate(const std::string& market);
};

#endif
