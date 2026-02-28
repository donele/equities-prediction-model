#ifndef __Bpnn0__
#define __Bpnn0__
#include "RLmodel.h"
#include <vector>
#include <fstream>

/* Bpnn with no hidden unit.
*/

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC Bpnn0: public RLmodel {
public:
	Bpnn0();
	Bpnn0(int rewardTime, int interval, int lenSeries, int maxPos, double learn, double thres, double thresIncr,
		double cost, double adap, double maxWgt, int verbose, bool trainOnline);
	~Bpnn0();

	virtual void evaluate(RLticker& rt);
	virtual void beginDay();

private:
	int lenSeries_;
	int rewardTime_;
	bool trainOnline_;
	std::ofstream* ofs_;
	std::vector<double> dWgt_;

	double get_x(RLticker& rt);
	double get_F(RLticker& rt);
	void update_wgt(RLticker& rt);
	double err(RLtrade& trade, RLticker& rt);
	double dFdw(int k, RLtrade& trade, RLticker& rt);
};

#endif