#ifndef __GenBPNN__
#define __GenBPNN__
#include "TRandom3.h"
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC GenBPNN {
public:
	GenBPNN();
	GenBPNN(std::string filename);
	GenBPNN(std::vector<int>& N, double learn = 1, double maxWgt = 1, int verbose = 0, int useBias = 0);
	~GenBPNN();

	void train(std::vector<double>& input, std::vector<double>& taget);
	std::vector<double> forward(std::vector<double>& input);
	void backprop(std::vector<double>& error);
	void backpropTD(std::vector<double>& error);
	void update_wgt();

	int get_nwgts();
	std::vector<double> get_wgts();
	std::vector<int> get_nnodes();
	void reset_grad();
	void alt();
	void save();
	void restore();
	void scale_learn(double f);

private:
	double lambda_;
	double learn_;
	double maxWgt_;
	double biasOut_;
	int verbose_;
	int useBias_;

	int nOutput_;
	int L_;
	std::vector<int> N_;
	std::vector<std::vector<std::vector<double> > > w_;
	std::vector<std::vector<std::vector<double> > > wOld_;
	std::vector<std::vector<std::vector<double> > > w2_;
	std::vector<std::vector<std::vector<double> > > dw_;
	std::vector<std::vector<std::vector<double> > > grad_; // Used for TD update.
	std::vector<std::vector<double> > f_;
	std::vector<std::vector<double> > e_;
	std::vector<std::vector<double> > f2_;
	int nwgts_;

	TRandom3* tr_;
};

FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, GenBPNN& obj);

#endif