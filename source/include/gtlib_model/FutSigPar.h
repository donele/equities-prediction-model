#ifndef __FutSigPar__
#define __FutSigPar__
#include <vector>
#include <string>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC FutSigPar {
public:
	int openMsecs;
	int closeMsecs;
	int gridInterval;
	int n1secs;
	int gpts;
	int num_time_slices;
	int om_num_basic_sig;
	int om_num_sig;
	int om_num_err_sig;
	int skip_first_secs_;
	int om_ntrees;
	int tm_ntrees;
	int nOmHorizon;
	int nTmHorizon;
	std::string projName;
	std::vector<std::pair<int, int> > vOmHorizonLag;
	std::vector<std::pair<int, int> > vTmHorizonLag;
	std::vector<int> useom;

	FutSigPar(){}
	FutSigPar(std::string projName);

	void addOmHorizon(int horizon, int lag);
	void addTmHorizon(int horizon, int lag);

private:

};

#endif
