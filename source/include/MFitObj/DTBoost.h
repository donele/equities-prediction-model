#ifndef __DTBoost__
#define __DTBoost__
#include <MFitObj/DTBase.h>
#include <vector>
#include <map>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC DTBoost {
public:
	DTBoost();
	DTBoost(int nTrees);
	DTBoost(int nTrees, bool isResTar, double evQuantile = 0.01);
	DTBoost(int ntrees, double shrinkage, int maxNodes, int minPtsNode, int maxLevels, int nthreads);
	void setData(std::vector<std::vector<float> >* pvvInputTarget, std::vector<std::vector<float> >* pvvInputTargetOOS,
		std::vector<std::vector<int> >* pvvInputTargetIndx);
	void setDir(const std::string& fitDir, int udate);
	void setDir(const std::string& dir, const std::string& coefDir, int udate);
	void fit();
	void stat(const std::vector<std::string>& inputNames);
	void analyze(std::vector<int>& testDates, std::vector<std::vector<float> >& vvInputTarget, std::vector<std::vector<float> >& vvSpectator,
		std::map<int, std::pair<int, int> >& mOffset, const std::string& model, const std::string& sample, bool rollingModelDate = false);
	void write_trees(const std::string& dbname, const std::string& tablename, const std::string& mCode,
			int udate, int dbDate, bool one_query = false);

private:
	int ntrees_;
	int n_tree_points_;
	int interval_ana_;
	double shrinkage_;
	bool isResTar_;
	int udate_;
	int dbDate_;
	double evQuantile_;
	std::string coefDir_;
	std::string statDir_;
	std::string predDir_;
	std::string predInsDir_;

	std::vector<double> wts_;
	std::vector<DTBase*> dts_;
	std::vector<std::vector<float> >* pvvInputTarget_;
	std::vector<std::vector<float> >* pvvInputTargetOOS_;
	std::vector<float> vTarget_;

	int get_iTree(int indx);
	int get_ana_indx(int iTree);
	void calcResiduals(int i);
	void fitTree(int i);
};

#endif
