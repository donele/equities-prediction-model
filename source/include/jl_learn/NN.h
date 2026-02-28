#ifndef __NN__
#define __NN__
//#include "TRandom3.h"
#include "optionlibs/TickData.h"
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC NN {
public:
	NN();
	NN(std::string filename);
	NN(std::vector<int>& N, double learn, double maxWgt, int verbose, double biasOut, double sigP, double poslim,
		double maxPos, double costMult, int lotSize, bool recurrent);
	virtual ~NN();

	virtual std::vector<double> forward(std::vector<double>& input, QuoteInfo& quote) = 0;
	virtual std::vector<double> forward_trade(std::vector<double>& input, QuoteInfo& quote) = 0;
	virtual void backprop(double error = 0) = 0;

	int get_nwgts();
	std::vector<double> get_wgts();
	std::vector<int> get_nnodes();
	void save();
	void restore();
	void scale_learn(double f);
	double sigP();
	double poslim();
	double maxPos();
	double costMult();
	int lotSize();

	virtual void start_ticker(double position){}
	virtual void set_position(double position){}

protected:
	int nWgtWarnings_;
	double learn_;
	double maxWgt_;
	int verbose_;
	bool useBias_; // Set to false during the training if biasOut is zero.
	int lotSize_;
	double biasOut_;
	double sigP_;
	double poslim_;
	double maxPos_;
	double costMult_;
	//TRandom3* tr_;

	int L_;
	std::vector<double> local_learn_;
	std::vector<int> N_;
	std::vector<std::vector<double> > ff_;
	std::vector<std::vector<double> > e_;
	std::vector<std::vector<std::vector<double> > > dw_;
	std::vector<std::vector<std::vector<double> > > w_;
	std::vector<std::vector<std::vector<double> > > wOld_;
	int nwgts_;

	void init_wgts();
};

FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, NN& obj);

#endif