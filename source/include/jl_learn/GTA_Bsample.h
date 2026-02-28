#ifndef __GTA_Bsample__
#define __GTA_Bsample__
#include "GTModule.h"
#include "optionlibs/TickData.h"
#include "Butil.h"
#include "TH2.h"
#include <string>
#include <map>
#include <vector>

class __declspec(dllexport) GTA_Bsample: public GTModule {
public:
	GTA_Bsample(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~GTA_Bsample();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void doTicker();
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	int iMarket_;
	int iModel_;
	int nInput_;
	int nOutput_;
	int timeFutureRet_;
	int interval_;
	double gamma_;
	bool normalize_;
	bool now_normalizing_;
	bool do_corr_;
	std::string norm_file_;

	std::vector<std::pair<int, int> > sessions_;
	std::vector<TH2F*> h_corr_;
	double input_n_;
	std::vector<double> input_sum_;
	std::vector<double> input_sum2_;

	void get_sample(TickSeries<QuoteInfo>* ptsQ, std::vector<Bsample>& sample);
	void get_quotes(TickSeries<QuoteInfo>* ptsQ, std::vector<QuoteInfo>& vq);
	void get_sample_10t(std::vector<QuoteInfo>& vq, std::vector<Bsample>& sample);
	void get_sample_1m(std::vector<QuoteInfo>& vq, std::vector<Bsample>& sample);
	void get_sample_1m_v2(std::vector<QuoteInfo>& vq, std::vector<Bsample>& sample);
	void normalize_sample(std::vector<Bsample>& sample);
};

#endif