#ifndef __DTSimple__
#define __DTSimple__
#include <MFitObj/DTBase.h>

class __declspec(dllexport) DTSimple : public DTBase {
public:
	DTSimple(){}
	DTSimple(int maxNodes, int minPtsNode, int treeMaxLevels, int nthreads);
	virtual void fit();

private:
	void fit_node_stat();
	void fit_node_cut();
};

#endif
