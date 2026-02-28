#ifndef __Butil__
#define __Butil__
#include "optionlibs/TickData.h"
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

struct CLASS_DECLSPEC Bsample {
	Bsample(std::vector<double>& vi, std::vector<double>& vt, QuoteInfo& qt):input(vi), target(vt), quote(qt){}
	std::vector<double> input;
	std::vector<double> target;
	QuoteInfo quote;
};

struct CLASS_DECLSPEC BnormInfo {
	BnormInfo(){}
	BnormInfo(std::string filename);
	BnormInfo(int n_):n(n_), mean(std::vector<double>(n)), stdev(std::vector<double>(n)){}
	int n;
	std::vector<double> mean;
	std::vector<double> stdev;
};

FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, BnormInfo& obj);

class CLASS_DECLSPEC Butil {
public:
	static int get_nInput(int iModel);
	static int get_nOutput(int iModel);
	static std::string get_inputName(int iModel, int i);
};

#endif