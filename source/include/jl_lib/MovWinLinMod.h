#ifndef __MovWinLinMod__
#define __MovWinLinMod__
#include <jl_lib/OLS.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <fstream>
#include <set>
#include <vector>
#include <jl_lib/crossCompile.h>

class CLASS_DECLSPEC MovWinLinMod {
public:
	MovWinLinMod();
	MovWinLinMod(int nInput, double ridgeReg = 0., int iDecomp = 1);
	MovWinLinMod(const std::string& path);
	int getNInputs();
	int nPts();
	void getCoeffs(bool ridge, bool use_cov, int verbose);
	void endDay(int idate, int idateFirst, const std::string& path, bool doWrite);
	void add(const std::vector<float>& input, float target);
	const OLS& getOLS();
	void write_coeffs(std::ostream& os);
	float pred(const std::vector<float>& input);

private:
	int nInputs_;
	double ridgeReg_;
	int iDecomp_;
	OLS ols_;

	CorrInfo cInfo_;
	std::map<int, CorrInfo> cInfoArch_;
};

#endif
