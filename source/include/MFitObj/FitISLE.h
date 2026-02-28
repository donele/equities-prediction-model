#ifndef __FitISLE__
#define __FitISLE__
#include <MFitObj/DTBase.h>
#include <string>
#include <vector>
#include <jl_lib.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC FitISLE {
public:
	FitISLE(){}
	FitISLE(int ntrees, double shrinkage, int maxNodes, int minPtsNode, int maxLevels, int nthreads, double step);
	void setDir(std::string dir, int udate);
	void fit(int iTree, std::vector<std::vector<float> >* pvvInputTarget, std::vector<std::vector<float> >* pvvInputTargetOOS);
	void postProcess(std::vector<std::vector<float> >& vvInputTarget, std::vector<std::vector<float> >& vvInputTargetOOS);

private:
	int ntrees_;
	bool sort_single_thread_;
	double shrinkage_;
	int udate_;
	int nrows_;
	int nInput_;
	std::string coefDir_;
	std::string statDir_;
	std::string predDir_;
	std::string predInsDir_;
	std::vector<double> wts_;

	double step_;
	double gradThres_; 
	double stopThres_;
	std::vector<double> covy_;
	std::vector<std::vector<double> > covx_;
	//std::vector<double> grad_;

	std::vector<DTBase*> dts_;
	std::vector<std::vector<float> >* pvvInputTarget_;
	std::vector<std::vector<int> > vvInputTargetIndx_;
	std::vector<std::vector<float> >* pvvInputTargetOOS_;
	std::vector<float> vTarget_;

	void get_gradFilter(std::vector<int>& vGradFilter, std::vector<double>& vGrad);
	double get_predRisk(int ncols, int ncolsPostProcess, std::vector<std::vector<float> >& vvInputTarget, std::vector<std::vector<double> >& vvPred);
	void calcResiduals(int i);
	void sort_input();
	void sort_input(int ir);
};

#endif