#ifndef __MWriteTree__
#define __MWriteTree__
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

class CLASS_DECLSPEC MWriteTree: public MModuleBase {
public:
	MWriteTree(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MWriteTree();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool write_debug_;
	bool one_query_;
	int nTargets_;
	std::string coefModel_;
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
