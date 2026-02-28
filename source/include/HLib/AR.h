#ifndef __AR__
#define __AR__
#include <vector>
#include <jl_lib/jlutil.h>
#include <jl_lib/crossCompile.h>

class CLASS_DECLSPEC AR {
public:
	AR(){}
	AR(int filterLen, int horizon = 1);

	void add(int lag, double v0, double v1);
	std::vector<double> getCoeffs(std::string target = "sum");
	double pred(std::vector<double>& v);

private:
	int filterLen_;
	int horizon_;
	std::vector<Corr> corrs_;
	std::vector<double> coeffs_;
	friend FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const AR& obj);
	friend FUNC_DECLSPEC std::istream& operator >>(std::istream& is, AR& obj);
};

#endif
