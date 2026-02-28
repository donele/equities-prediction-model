#ifndef __DTMT__
#define __DTMT__
#include <MFitObj/DTBase.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC DTMT : public DTBase {
public:
	DTMT(){}
	DTMT(const std::vector<std::string>& lines, int iTree);
	DTMT(int maxNodes, int minPtsNode, int treeMaxLevels, int nthreads);
	virtual void fit();

private:
	bool single_thread_;

	void fit_node_stat();
	void fit_node_cut();
	void fit_node_cut_input(int ir);
	void fit_node_cut_recombine(int in);
	double TnodeCutFn(const Acc& acc, const Acc& accTot, int cutpts, int weirdFactor);
};

#endif
