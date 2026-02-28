#ifndef __HNNSampleNews__
#define __HNNSampleNews__
#include <HLearnObj/Sample.h>
#include <HLearn/MCPred.h>
#include <boost/thread.hpp>
#include <jl_lib/InputCounter.h>
#include <HLib.h>
#include <optionlibs/TickData.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <string>
#include <map>
#include <set>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HNNSampleNews: public HModule {
public:
	HNNSampleNews(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HNNSampleNews();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	int nInput_;
	int minMsecs_;
	int maxMsecs_;
	int sampleInterval_;
	int msecOpen_;
	int msecClose_;
	int lotSize_;
	//bool adjust_market_;
	bool hedge_inputs_;
	bool hedge_target_;
	bool log_share_;
	bool target_residual_;
	bool write_sample_;
	bool require_news_;
	int lot_size_;
	double newsAdjust_;
	std::string sample_dir_;
	std::string pred_dir_;
	std::vector<std::string> inputNames_;
	std::vector<std::string> labels_;
	std::vector<std::string> labels_won_;
	std::ofstream ofs_;
	InputCounter ic_;

	int targetTime_;
	std::set<int> pastRtnTime_;
	std::set<int> pastNewsTime_;
	int nPastRtn_;
	int nPastNews_;
	bool input_bidSize_;
	bool input_askSize_;
	bool input_spread_;
	bool input_quoteImb_;
	bool input_volume_;
	bool input_pred1m_;
	bool input_pred40m_;
	bool input_pred41m_;
	MCPred preds_;

	std::set<std::string> sEUmarkets_;
	std::vector<double> secRetCum_;
	std::vector<std::pair<int, int> > sessions_;
	std::map<std::string, double> mVol_;
	double input_n_;
	std::vector<double> input_sum_;
	std::vector<double> input_sum2_;

	std::vector<TH1F*> h_input_;
	std::vector<TH1F*> h_input_eu_;
	std::vector<TH1F*> h_input_all_;
	std::vector<TH2F*> h_corr_;
	std::vector<TH2F*> h_corr_eu_;
	std::vector<TH2F*> h_corr_all_;
	std::vector<TProfile*> p_corr_;
	std::vector<TProfile*> p_corr_eu_;
	std::vector<TProfile*> p_corr_all_;
	MultiRegress mr_;
	MultiRegress mr_eu_;
	MultiRegress mr_all_;
	MultiRegress mr_won_; // withou news inputs.
	MultiRegress mr_eu_won_;
	MultiRegress mr_all_won_;
	TH1F h_r2_with_news_;
	TH1F h_r2_without_news_;

	void get_sample(std::string ticker, const TickSeries<QuoteInfo>* ptsQ, std::vector<hnn::Sample>& sample);
	void get_quotes(const TickSeries<QuoteInfo>* ptsQ, std::vector<QuoteInfo>& vq);
	void get_sample(std::string ticker, std::vector<QuoteInfo>& vq, std::vector<hnn::Sample>& sample);
	void get_sample(std::string ticker, const std::vector<double>* msecQ1s, const std::vector<double>* midQ1s,
	const std::vector<double>* msecN1s, const std::vector<double>* valN1s, std::vector<hnn::Sample>& sample);
	void print_mr(MultiRegress& mr, std::string market, std::string flag = "");
	void update_hist(double r2, std::string market, std::string flag);
};

#endif

