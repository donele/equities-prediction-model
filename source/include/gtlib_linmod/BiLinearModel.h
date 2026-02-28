#ifndef __BiLinearModel__
#define __BiLinearModel__
#include <gtlib_model/hff.h>
#include <jl_lib.h>
#include <vector>
#include <unordered_map>
#include <boost/thread.hpp>

class BiLinearModel {
public:
	BiLinearModel();
	BiLinearModel(const std::string& covDir, const std::string& weightDir, int minPts,
			int om_univ_fit_days, int om_err_fit_days,
			int nHorizon, int num_time_slices, int om_num_sig, int om_num_err_sig,
			double ridgeRegUniv, double ridgeRegSS, const std::set<std::string>& uids, const std::vector<int>& allIdates,
			bool writeWeights = true, bool debug_wgt = false, int verbose = 0);
	void endJob();

	void set_verbose();
	void beginDay(int idate);
	void clear_cov(int idate);
	void endDay(int idate);
	void update(int iT, int timeIdx, const std::vector<float>& v, double target);
	void updateErr(int iT, const std::string& uid, const std::vector<float>& v, double target);
	double pred(int iT, int timeIdx, const std::vector<float>& v);
	double predIndex(int iT, int timeIdx, const std::vector<float>& v);
	double predErr(int iT, const std::string& uid, const std::vector<float>& v);
	double predErrIndex(int iT, const std::string& uid, const std::vector<float>& v);
	bool goodUnivModel();
	bool goodErrModel();
	bool goodModel();
	void setWriteSigmoid(bool tf);
	void useSigmoid();
	void read_range(int idate);

	struct LinRegStatTicker {
		LinRegStatTicker(int num_time_slices, int om_num_sig, int om_num_err_sig, double ridgeRegUniv, double ridgeRegSS);
		std::vector<OLSmovMM> olsmov;
		OLSmovMM olsmovErr;
	};
	LinRegStatTicker* getNewLinRegStatTicker();
	void update(LinRegStatTicker* linRegInfo, const std::string& uid);
	std::string get_daily_range_file(int idate);

private:
	int verbose_;
	int minPts_;
	int num_time_slices_;
	int om_num_sig_;
	int om_num_err_sig_;
	int univFitDays_;
	int errFitDays_;
	int nHorizon_;
	double ridgeRegUniv_;
	double ridgeRegSS_;
	bool debug_wgt_;
	bool use_sigmoid_;
	bool coeffsNeverCalculated_;
	bool writeWeights_;
	bool writeRanges_;
	bool enoughDataUniv_;
	bool enoughDataErr_;
	bool goodUnivModel_;
	bool goodErrModel_;
	std::string weight_dir_;
	std::string cov_dir_;
	std::vector<int> allIdates_;
	std::vector<std::vector<OLSmovMM> > olsmov_;
	std::vector<std::unordered_map<std::string, OLSmovMM> > olsmovErr_;

	std::vector<float> range_;
	std::vector<float> rangeErr_;

	void write_weights(int idate);
	int getPrevIdate(int idate);
	int getFirstDateInclusive(const std::vector<int>& idates, int lastIdate, int length);
	std::string get_cov_file(int idate);
	std::string get_weight_file(int idate);
	std::string get_aggregated_range_file(int idate);
	void write_cov(int idate);
	void write_range(int idate);
	void init_OLS();
	void read_cov(int idate);
	bool valid_univ_weights();
	bool valid_err_weights();
	void calculate_coeffs();

	double getPartialPred(int i, const std::vector<float>& wgts, const std::vector<float>& v);
	double getPartialPredErr(int i, const std::vector<float>& wgts, const std::vector<float>& v);
};

#endif
