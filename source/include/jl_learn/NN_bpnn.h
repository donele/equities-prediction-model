#ifndef __NN_bpnn__
#define __NN_bpnn__
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

class CLASS_DECLSPEC NN_bpnn : public NN{
public:
	NN_bpnn(std::string filename);
	NN_bpnn(std::vector<int>& N, double learn = 1, double maxWgt = 1, int verbose = 0, double biasOut = 0);
	virtual ~NN_bpnn();

	virtual std::vector<double> forward(std::vector<double>& input, QuoteInfo& quote);
	virtual std::vector<double> forward_trade(std::vector<double>& input, QuoteInfo& quote);
	virtual void backprop(double error = 0);

private:
	void update_wgt();
};

#endif