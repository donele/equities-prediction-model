#ifndef __BiLinearModelN__
#define __BiLinearModelN__
#include <gtlib_model/hff.h>
#include <jl_lib.h>
#include <vector>
#include <map>

class BiLinearModelN {
public:
	BiLinearModelN();
	BiLinearModelN(const std::string& covDir, const std::string& weightDir, int minPts,
			int om_univ_fit_days, int om_err_fit_days,
			int nHorizon, int num_time_slices, int om_num_sig, int om_num_err_sig,
			double ridgeRegUniv, double ridgeRegSS, const std::set<std::string>& uids, const std::vector<int>& allIdates,
			bool writeWeights = true, bool debug_wgt = false, int verbose = 0);
	void endJob();

	void set_verbose();
	void beginDay(int idate);
	void endDay(int idate);
	void write_weights(int idate, bool check_valid = true);
	void update(int iT, int timeIdx, const std::vector<float>& v, double target);
	void updateErr(int iT, const std::string& uid, const std::vector<float>& v, double target);
	double pred(int iT, int timeIdx, const std::vector<float>& v);
	double predIndex(int iT, int timeIdx, const std::vector<float>& v);
	double predErr(int iT, const std::string& uid, const std::vector<float>& v);
	double predErrIndex(int iT, const std::string& uid, const std::vector<float>& v);
	bool haveEnoughData();

private:
	int verbose_;
	int minPts_;
	int univFitDays_;
	int errFitDays_;
	bool debug_wgt_;
	bool writeWeights_;
	std::string weight_dir_;
	std::string cov_dir_;
	std::vector<int> allIdates_;
	std::set<int> sIdate_;
	std::vector<std::vector<OLSmov> > olsmov_;
	std::vector<std::map<std::string, OLSmov> > olsmovErr_;

	std::string get_cov_file(int idate);
	void write_cov(int idate);
	void init_OLS();
	void read_cov(int idate);
	bool valid_weights();
	void calculate_coeffs();
};

#endif
