#ifndef __NN_bptt_tanh__
#define __NN_bptt_tanh__
#include "NN.h"
#include "optionlibs/TickData.h"
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC NN_bptt_tanh : public NN{
public:
	NN_bptt_tanh(std::string filename);
	NN_bptt_tanh(std::vector<int>& N, double learn = 1, double maxWgt = 1, int verbose = 0, double biasOut = 0, double costMult = 1,
		double sigP = 1, double poslim = 0, double maxPos = 0, int lotSize = 1, bool maxRet = true);
	virtual ~NN_bptt_tanh();

	virtual std::vector<double> forward(std::vector<double>& input, QuoteInfo& quote);
	virtual std::vector<double> forward_trade(std::vector<double>& input, QuoteInfo& quote);
	virtual void backprop(double error = 0);

	virtual void start_ticker(double position);
	virtual void set_position(double position);

private:
	double last_mid_;
	double outScale_;
	bool maxRet_;
	QuoteInfo quote_;

	double F_;
	double P_; // position
	double P_p_; // previous position
	std::vector<std::vector<std::vector<double> > > pFpw_;
	std::vector<std::vector<std::vector<double> > > dFdw_;
	std::vector<std::vector<std::vector<double> > > p_dFdw_;

	double get_R0();
	double get_R();
	double get_g();

	double get_pRpF();
	double get_pRpF_p();
	double get_pgpF();
	double get_pgpF_p();
	double get_pFpF_p();
};

#endif