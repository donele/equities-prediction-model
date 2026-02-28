#ifndef __DTMTpre__
#define __DTMTpre__
#include <MFitObj/DTBase.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

// Decision Tree with single threaded training.

class CLASS_DECLSPEC DTMTpre : public DTBase {
public:
	DTMTpre(){}
	DTMTpre(int maxNodes, int minPtsNode, int treeMaxLevels, int nthreads);
	virtual void fit();

private:
	void fit_node_stat();
	void fit_node_cut();
	double TnodeCutFn(Acc& acc, Acc& accTot, int cutpts, int weirdFactor);
};

#endif
