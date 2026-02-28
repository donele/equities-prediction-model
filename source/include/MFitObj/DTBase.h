#ifndef __DTBase__
#define __DTBase__
#include <MFitObj/DTNode.h>
#include <jl_lib/jlutil.h>
#include <fstream>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

struct CLASS_DECLSPEC DTStat {
	DTStat():var(0), cut(0.), diffmax(0.), tmpXold(0.){}
	void clear() {acc.clear(); var = -1; cut = -max_double_; leftMax = 0; diffmax = -max_double_;}

	Acc acc;
	int var;
	double cut;
	int leftMax;
	double diffmax;
	double tmpXold;
};

class CLASS_DECLSPEC DTBase {
public:
	static std::vector<int> viNode;

	// Used for Multithreading.
	static std::vector<Acc> vAcc;
	static std::vector<std::vector<DTStat> > vvStat;

	DTBase();
	DTBase(const std::vector<std::string>& lines, int iTree);
	DTBase(int maxNodes, int minPtsNode, int treeMaxLevels, int nthreads);
	void setData(std::vector<std::vector<float> >* vvInputTarget, std::vector<std::vector<float> >* vvInputTargetOOS,
		std::vector<std::vector<int> >* vvInputTargetIndx, std::vector<float>& vTarget);
	void setDataIns(std::vector<std::vector<float> >* vvInputTarget);
	void setDataOOS(std::vector<std::vector<float> >* vvInputTarget);
	virtual void fit(){}
	void dump(std::ofstream& ofs, int iTree, double weight);
	double get_pred(int ic);
	double get_pred_oos(int ic);
	double get_deviance();
	void deviance_improvement(std::vector<double>& stats);
	void write_tree(const std::string& dbname, const std::string& tablename, const std::string& mCode, int dbDate, int iTree, float wgt);
	void write_node(DTNode* pnode, int& count);
	std::vector<std::string> get_query(const std::string& mCode, int dbDate, int iTree, float wgt);
	void append_query(std::vector<std::string>& v, DTNode* pnode, int& count);

protected:
	int maxNodes_;
	int minPtsNode_;
	int maxLevels_;

	int iLine_;
	int nrows_;
	int ncols_;
	DTNode* pRoot_;
	std::vector<float> vInput_;

	std::vector<DTNode*> nodes_;
	int start_;
	int end_;

	std::vector<std::vector<float> >* pvvInputTarget_;
	std::vector<std::vector<float> >* pvvInputTargetOOS_;
	std::vector<std::vector<int> >* pvvInputTargetIndx_;
	std::vector<float>* pvTarget_;
	std::vector<std::vector<std::string> > lines_;

	// write
	std::string dbname_;
	std::string tablename_;
	std::string mCode_;
	int dbDate_;
	int iTree_;
	float wgt_;

	DTNode* make_node();
	double TnodeCutFn(double y0, double y1, double y2, double yy0, double yy1, double yy2, int indnum, int cutpts, int weirdFactor);
	bool fit_node_create();
	void dump_node(std::ofstream& ofs, DTNode* z, int iTree, double weight);
	double traverse_mu(DTNode* pnode);
	void traverse_deviance(DTNode* pnode, double& sumDeviance, int& npts);
	void traverse_improvement(DTNode* pnode, std::vector<double>& stats);
};

#endif
