#ifndef __MCombineTree__
#define __MCombineTree__
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

class CLASS_DECLSPEC MCombineTree: public MModuleBase {
public:
	MCombineTree(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MCombineTree();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	struct ModelRow {
		int iTree;
		float weight;
		int var;
		float cut;
		int indnum;
		float mu;
		double deviance;
	};
	bool debug_;
	int nTargets_;
	std::string combinedTarget_;
	std::vector<std::string> targetNames_;
	std::vector<double> targetWeights_;

	std::vector<int> vNTrees_;
	std::string fitDesc_;
	std::string fitDir_;

	void write_model();
	ModelRow getRow(const std::string& line);
	std::string getCombinedTarget();
};

#endif
