#ifndef __BiLinearModelWeights__
#define __BiLinearModelWeights__
#include <jl_lib/QueryAggregator.h>
#include <jl_lib/crossCompile.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>

class CLASS_DECLSPEC BiLinearModelWeights {
public:
	BiLinearModelWeights();
	BiLinearModelWeights(int nHorizon, int num_time_slices, int om_num_sig, int om_num_err_sig);

	void useSigmoid();
	void useSigmoid(const std::vector<float>& range, const std::vector<float>& rangeErr);
	void read_range(int idate, const std::string& weight_dir);
	void read_weights(int idate, const std::string& weight_dir);
	void read_db_weights(const std::string& model, const std::string& market, int idate, char mCode,
			std::unordered_map<std::string, std::string>& mTickerUid, int gpts, const std::string& dbname);
	void write_weights_beginDay(int idate, int dbdate, const std::string& model,
			const std::vector<std::string>& markets, bool db_debug, int gpts);
	void write_weights_ticker(const std::string& model, const std::string& market,
			const std::string& uid, const std::string& ticker, int iHorizon,
			double volume, double close, double volat, double medSprd, int inUniv, double nFFundRST, double nFFundTRD, double nFFundBP,
			const std::string& secType, const std::string& exchangeChar, bool debug_noss = false);
	void write_weights_endDay(const std::string& model, const std::string& market, char mCode, bool freeze_univ);
	void endDay_random_univ(const std::string& model, bool freeze_univ);
	void endDay_paramsOK(const std::string& model, const std::string& market, char mCode);

	double pred(int iT, int timeIdx, const std::vector<float>& v);
	double predIndex(int iT, int timeIdx, const std::vector<float>& v);
	double predErr(int iT, const std::string& uid, const std::vector<float>& v);
	double predErrIndex(int iT, const std::string& uid, const std::vector<float>& v);
	double predDB(int timeIdx, const std::string& uid, const std::vector<float>& v);

	double getPartialPred(int i, const std::vector<float>& wgts, const std::vector<float>& v);
	double getPartialPredErr(int i, const std::vector<float>& wgts, const std::vector<float>& v);

private:
	std::string dbname_;
	bool dbdate_monday_;
	bool efficient_write_;
	bool use_sigmoid_;
	int dbdate_;
	int dbdate_p_;
	int nHorizon_;
	int num_time_slices_;
	int om_num_sig_;
	int om_num_err_sig_;
	QueryAggregator qa_;
	std::set<std::string> portSyms_;
	std::vector<int> timeSliceStart_;
	std::map<int, int> mItime_;
	std::vector<std::string> vSymbolInuniv_;

	std::vector<float> range_;
	std::vector<float> rangeErr_;

	std::vector<std::vector<std::vector<float> > > weights_;
	std::vector<std::map<std::string, std::vector<float> > > weightsErr_;

	std::vector<std::map<std::string, std::vector<float> > > weightsDB_;
	std::vector<std::vector<float> > weightsDBsprd_;

	void set_time_slices(int gpts);
	std::string get_aggregated_range_file(int idate, const std::string& weight_dir);
	std::string get_weight_path(int idate, const std::string& weight_dir);
	std::string select_exchange(const std::vector<std::string>& markets);
};

#endif
