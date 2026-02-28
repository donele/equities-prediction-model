#ifndef __Perf__
#define __Perf__
#include <MFitObj/DTBase.h>
#include <string>
#include <vector>

class Perf {
public:
	Perf(){}
	Perf(const std::string& statDir, int ncols, int ncolsOOS);
	void run(std::vector<DTBase*>& dts, std::vector<double>& wgts, int iTree, std::vector<float>& vTarget,
		std::vector<std::vector<float> >* pvvInputTarget, std::vector<std::vector<float> >* pvvInputTargetOOS);
	void run(std::vector<DTBase*>& dts, std::vector<double>& wgts, int iTree,
		std::vector<std::vector<float> >* pvvInputTarget, std::vector<std::vector<float> >* pvvInputTargetOOS,
		std::vector<std::vector<double> >& vvPred);

private:
	int lastTree_;
	char filename_[1000];
	std::vector<float> pred_;
	std::vector<float> predOOS_;
};

#endif

