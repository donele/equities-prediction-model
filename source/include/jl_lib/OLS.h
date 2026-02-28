#ifndef __OLS__
#define __OLS__
#include <jl_lib/jlutil.h>
#include <gtlib_linmod/CovMatSet.h>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC OLS {
public:
	OLS();
	OLS(int nInputs, double ridgeReg = 0., int iDecomp = 1);

	int nPts() const;
	int length() const;
	void print_coeffs() const;
	bool valid_coeffs() const;
	void write_coeffs(std::ostream& ofs) const;
	void write_coeffs(std::ostream& ofs, std::vector<float>& vSd) const;
	const std::vector<float>& copyCoeffs() const;
	double pred(const std::vector<float>& v, int offset = 0) const;
	double pred(const std::vector<float>& v, const std::vector<float>& vSd) const;
	const Corr& get_corr(int i, int j) const;
	double coeff(int i) const;

	void add(const float input, float target);
	void add(const std::vector<float>& input, float target);
	void addCorr(const gtlib::CovMatSet& cSet);
	void addCorr(const std::vector<Corr>& vCorr);
	void addCorr(const std::vector<std::vector<Corr> >& vvCorr);
	void addCorr(const std::vector<Corr>& vCorr, const std::vector<std::vector<Corr> >& vvCorr,
			const std::vector<Acc>& vAcc, const Acc& accY);
	std::vector<float> getCoeffs(bool ridge = false, bool use_cov = false, int verbose = 0);

private:
	int nInputs_;
	int iDecomp_;
	double ridgeReg_;
	std::vector<Corr> vCorr_;
	std::vector<std::vector<Corr> > vvCorr_;
	std::vector<Acc> vAcc_;
	Acc accY_;
	std::vector<float> coeffs_;
	friend FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const OLS& obj);
	friend FUNC_DECLSPEC std::istream& operator >>(std::istream& is, OLS& obj);
};


#endif
