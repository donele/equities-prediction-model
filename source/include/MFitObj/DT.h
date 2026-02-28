#ifndef __DT__
#define __DT__
#include <MFitObj/DTNode.h>
#include <fstream>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC DT {
public:
	static std::vector<int> rpartWhich;

	DT();
	DT(int maxNodes, int minPtsNode, int treeMaxLevels, int nthreads);
	void setData(std::vector<std::vector<float> >* vvInputTarget, std::vector<std::vector<float> >* vvInputTargetOOS,
		std::vector<std::vector<int> >* vvInputTargetIndx, std::vector<float>& vTarget);
	void fit();
	void dump(std::string filename, double weight);
	double get_pred(int ic);
	double get_pred_oos(int ic);
	double get_deviance();
	void deviance_improvement(std::vector<double>& stats);

private:
	int maxNodes_;
	int minPtsNode_;
	int maxLevels_;
	int nthreads_;

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
	//std::vector<std::vector<int> >* pvvInputTargetOOSIndx_;
	std::vector<float>* pvTarget_;

	void fit_node_stat();
	void fit_node_cut();
	bool fit_node_create();
	double TnodeCutFn(double y0, double y1, double y2, double yy0, double yy1,
				  double yy2, long i, long indnum, long cutpts, long weirdFactor);

	void dump_node(std::ofstream& ofs, DTNode* z, double weight);
	double traverse_mu(DTNode* pnode);
	void traverse_deviance(DTNode* pnode, double& sumDeviance, int& npts);
	void traverse_improvement(DTNode* pnode, std::vector<double>& stats);
};

#endif
