#ifndef __BPTT__
#define __BPTT__
#include "TRandom3.h"
#include "optionlibs/TickData.h"
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC BPTT {
public:
	BPTT();
	BPTT(std::string filename);
	BPTT(std::vector<int>& N, double learn = 1, double maxWgt = 1, int verbose = 0, int useBias = 0);
	~BPTT();

	void start_ticker(double position);
	void set_position(double position);
	std::vector<double> forward(std::vector<double>& input, QuoteInfo& quote);
	void backprop();

	int get_nwgts();
	std::vector<double> get_wgts();
	std::vector<int> get_nnodes();

	void save();
	void restore();
	void scale_learn(double f);

private:
	double learn_;
	double maxWgt_;
	double biasOut_;
	double poslim_;
	int verbose_;
	int useBias_;
	TRandom3* tr_;
	QuoteInfo quote_;

	int L_;
	double P_;
	double P_p_;
	double F_;
	double F_p_;
	double sigP_;
	std::vector<double> local_learn_;
	std::vector<int> N_;
	std::vector<std::vector<double> > ff_;
	std::vector<std::vector<double> > e_;
	std::vector<std::vector<std::vector<double> > > pFpw_;
	std::vector<std::vector<std::vector<double> > > dFdw_;
	std::vector<std::vector<std::vector<double> > > p_dFdw_;
	std::vector<std::vector<std::vector<double> > > dw_;
	std::vector<std::vector<std::vector<double> > > w_;
	std::vector<std::vector<std::vector<double> > > wOld_;
	int nwgts_;
	double last_mid_;

	void init_wgts();

	double get_R();
	double get_g();

	// dw = pR/pw
	//    = pR/pP pP/pF dF/dw + pR/pP_p pP_p/pF_p dF_p/dw_p
	double get_pRpP();
	double get_pRpP_p();
	double get_pgpP();
	double get_pgpP_p();
	double get_pFpF_p();
};

FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, BPTT& obj);

#endif