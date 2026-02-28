#ifndef __HNNSampleNewsFile__
#define __HNNSampleNewsFile__
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

class CLASS_DECLSPEC HNNSampleNewsFile: public HModule {
public:
	HNNSampleNewsFile(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HNNSampleNewsFile();

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
	int msecOpen_;
	int msecClose_;
	bool target_residual_;
	bool write_sample_;
	std::string input_dir_;
	std::string sample_dir_;
	std::vector<std::string> inputNames_;
	std::vector<int> indx_;
	std::vector<std::string> labels_;
	std::ofstream ofs_;
	InputCounter ic_;

	std::set<std::string> sEUmarkets_;
	std::vector<double> secRetCum_;
	std::vector<std::pair<int, int> > sessions_;
	std::map<std::string, double> mVol_;
	double input_n_;
	std::vector<double> input_sum_;
	std::vector<double> input_sum2_;

	std::vector<TH1F*> h_input_;
	std::vector<TH1F*> h_input_all_;
	std::vector<TH2F*> h_corr_;
	std::vector<TH2F*> h_corr_all_;
	std::vector<TProfile*> p_corr_;
	std::vector<TProfile*> p_corr_all_;
	MultiRegress mr_;
	MultiRegress mr_all_;
	TH1F h_r2_with_news_;

	void get_sample(std::string ticker, const TickSeries<QuoteInfo>* ptsQ, std::vector<hnn::Sample>& sample);
	void get_quotes(const TickSeries<QuoteInfo>* ptsQ, std::vector<QuoteInfo>& vq);
	void get_sample(std::string ticker, std::vector<QuoteInfo>& vq, std::vector<hnn::Sample>& sample);
	void get_sample(std::string ticker, const std::vector<double>* msecQ1s, const std::vector<double>* midQ1s,
		const std::vector<double>* msecN1s, const std::vector<double>* valN1s, std::vector<hnn::Sample>& sample);
	void print_mr(MultiRegress& mr, std::string market, std::string flag = "");
	void update_hist(double r2, std::string market, std::string flag);
	void find_index(std::vector<std::string>& v);
	void write_sample(std::vector<hnn::Sample>& sample, std::string lastTicker);
	void add_sample(std::vector<std::string>& v, std::vector<hnn::Sample>& sample, std::string ticker, float resid);
	void get_tickers(std::vector<std::string>& tickers, std::string market, int idate);
};

#endif

