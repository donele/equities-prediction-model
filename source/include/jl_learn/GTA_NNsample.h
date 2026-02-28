#ifndef __GTA_NNsample__
#define __GTA_NNsample__
#include "GTModule.h"
#include "optionlibs/TickData.h"
#include "NNobj.h"
#include "TH2.h"
#include <string>
#include <map>
#include <set>
#include <vector>

class __declspec(dllexport) GTA_NNsample: public GTModule {
public:
	GTA_NNsample(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~GTA_NNsample();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void doTicker();
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	int iMarket_;
	int nInput_;
	int sampleInterval_;
	int msecOpen_;
	int msecClose_;
	int lotSize_;
	bool normalize_;
	bool now_normalizing_;
	bool do_corr_;
	bool adjust_market_;
	bool log_share_;
	int lot_size_;
	std::string norm_file_;
	std::vector<std::string> inputNames_;

	int targetTime_;
	std::set<int> pastRtnTime_;
	int nPastRtn_;
	bool input_bidSize_;
	bool input_askSize_;
	bool input_spread_;
	bool input_quoteImb_;
	bool input_volume_;

	std::vector<double> secRetCum_;
	std::vector<std::pair<int, int> > sessions_;
	std::map<std::string, double> mVol_;
	std::vector<TH2F*> h_corr_;
	double input_n_;
	std::vector<double> input_sum_;
	std::vector<double> input_sum2_;

	void get_sample(TickSeries<QuoteInfo>* ptsQ, std::vector<NNsample>& sample);
	void get_quotes(TickSeries<QuoteInfo>* ptsQ, std::vector<QuoteInfo>& vq);
	void get_sample(std::vector<QuoteInfo>& vq, std::vector<NNsample>& sample);
	void normalize_sample(std::vector<NNsample>& sample);
};

#endif

