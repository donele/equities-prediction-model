#ifndef __RLmodel__
#define __RLmodel__
#include "RLobj.h"
#include "TRandom3.h"
#include <vector>
#include <map>

/*  Abstract class for reinforcement learning classes.
*/

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC RLmodel {
public:
	RLmodel();
	RLmodel(int interval, int maxPos, double learn, double thres, double thresIncr, double cost, double adap,
		double maxWgt, int verbose);
	~RLmodel();

	virtual void evaluate(RLticker& rt) = 0;
	virtual void beginDay(){}
	int get_nwgts();
	std::vector<double> get_wgts();

protected:
	int verbose_;
	int maxPos_;
	int interval_;
	double cost_;
	double adap_;
	double learn_;
	double thres_;
	double thresIncr_;
	double maxWgt_;
	double dFiF_;
	std::vector<double> wgt_;
	TRandom3* tr_;

	void initWgt();
	void initWgt(int i);
	int FtoInt(double F);
};

#endif