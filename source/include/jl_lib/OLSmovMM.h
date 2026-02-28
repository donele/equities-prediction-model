#ifndef __OLSmovMM__
#define __OLSmovMM__
#include <jl_lib/OLS.h>
#include <jl_lib/jlutil.h>
#include <gtlib_linmod/CovMatSet.h>
#include <map>
#include <set>
#include <vector>
#include <jl_lib/crossCompile.h>

class CLASS_DECLSPEC OLSmovMM {
public:
	OLSmovMM();
	OLSmovMM(int nInput, double ridgeReg = 0., int iDecomp = 1);
	int nPts();
	void initOLS();
	void getCoeffs(bool ridge, bool use_cov, int verbose);
	void clear();
	void add(const std::vector<float>& input, float target, int offset = 0);
	void add(const OLSmovMM& rhs);
	const OLS& getOLS();
	void write_coeffs(std::ostream& os);
	void saveCorrs(std::ostream& os);
	void loadCorrs(std::istream& is, bool valid = true);
	void loadCorrs(const gtlib::CovMatSet& cSet);
	void saveCorrsBlock(std::ostream& os, int blockSize);
	void loadCorrsBlock(std::istream& is, int blockSize, bool valid = true);

private:
	int nInputs_;
	double ridgeReg_;
	int iDecomp_;
	OLS ols_;

	std::vector<Corr> vCorr_;
	std::vector<std::vector<Corr> > vvCorr_;
	std::vector<Acc> vAcc_;
	Acc accY_;
};

#endif
