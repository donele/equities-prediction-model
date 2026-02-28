#ifndef __NNobj__
#define __NNobj__
#include "optionlibs/TickData.h"
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

struct CLASS_DECLSPEC NNsample {
	NNsample(const NNsample& ns);
	NNsample(std::vector<double>& vi, double& t, QuoteInfo& qt):input(vi), target(t), quote(qt){}
	std::vector<double> input;
	double target;
	QuoteInfo quote;
};

struct CLASS_DECLSPEC NNinputInfo {
	NNinputInfo(){}
	NNinputInfo(const NNinputInfo& nni);
	NNinputInfo(std::string filename);
	NNinputInfo(int n_):n(n_), mean(std::vector<double>(n)), stdev(std::vector<double>(n)){}
	int n;
	std::vector<double> mean;
	std::vector<double> stdev;
};

struct CLASS_DECLSPEC NNperformance {
	NNperformance():n(0),buf1(0),buf2(0),sum1(0),sum2(0){}
	void clear() {bufn = 0; buf1 = 0; buf2 = 0; n = 0; sum1 = 0; sum2 = 0;}
	void add(double v);
	void add(double v, double s);
	double bufn;
	double buf1;
	double buf2;
	double n;
	double sum1;
	double sum2;
	double get_n();
	double get_sum2();
	double endofdaysum();
	double endofdayerr();
	double endofdayavg();
	double endofdayrat();
	double resultsum();
	double resulterr();
	double resultavg();
	double resultrat();
};

FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, NNinputInfo& obj);

#endif