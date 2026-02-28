#ifndef __RmLin__
#define __RmLin__
#include "RLmodel.h"
#include <vector>
#include <fstream>

/* Linear model with reinforcements for an action after rewardTime_ time units.
*/

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC RmLin: public RLmodel {
public:
	RmLin();
	RmLin(int rewardTime, int interval, int lenSeries, int maxPos, double learn, double thres, double thresIncr,
		double cost, double adap, double maxWgt, int verbose, std::string opt, bool trainOnline);
	~RmLin();

	virtual void evaluate(RLticker& rt);
	virtual void beginDay();

private:
	int lenSeries_;
	int rewardTime_;
	bool trainOnline_;
	std::string opt_;
	double At_p_;
	double Bt_p_;
	std::ofstream* ofs_;
	std::vector<double> dWgt_;

	double get_x(RLticker& rt);
	double get_F(RLticker& rt);
	double get_Rt(RLtrade& trade, RLticker& rt);
	double get_unitRt(RLtrade& trade, RLticker& rt);
	void update_wgt(RLticker& rt);
	double dRdF(RLtrade& trade, RLticker& rt);
	double dFdw(int k, RLtrade& trade, RLticker& rt);
};

#endif