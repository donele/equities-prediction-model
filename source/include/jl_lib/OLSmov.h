// OLS with moving window of fitDays_.
// Inputs are normalized.

#ifndef __OLSmov__
#define __OLSmov__
#include <jl_lib/OLS.h>
#include <jl_lib/jlutil.h>
#include <set>
#include <map>
#include <vector>
#include <jl_lib/crossCompile.h>

class CLASS_DECLSPEC OLSmov {
public:
	OLSmov();
	OLSmov(int nInput, double ridgeReg, int iDecomp = 1);
	int nPts();
	void initOLS();
	void getCoeffs(bool ridge, bool use_cov, int verbose);
	void clear();
	void add(const std::vector<float>& input, float target, int offset = 0);
	const OLS& getOLS();
	void write_coeffs(std::ostream& os);
	double pred(const std::vector<float>& input);
	void saveCorrs(std::ostream& os);
	void loadCorrs(std::istream& is, bool valid = true);

private:
	int nInputs_;
	double ridgeReg_;
	int iDecomp_;
	OLS ols_;

	std::set<int> sIdate_;
	std::vector<Corr> vCorr_;
	std::vector<std::vector<Corr> > vvCorr_;
	std::vector<Acc> vAcc_;
	Acc accY_;
	std::vector<Acc> vAccRaw_;
	std::vector<Acc> vAccRawTot_;
	std::vector<float> vSd_;
};

#endif
