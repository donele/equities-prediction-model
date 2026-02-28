#ifndef __HNNSampleCommodity__
#define __HNNSampleCommodity__
#include <HLearnObj/Sample.h>
#include <boost/thread.hpp>
#include "HLib.h"
#include "optionlibs/TickData.h"
#include "TH2.h"
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

class CLASS_DECLSPEC HNNSampleCommodity: public HModule {
public:
	HNNSampleCommodity(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HNNSampleCommodity();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	std::vector<int> vNorthFieldSector_;
	std::string leadingTicker_;
	std::string leadingTickerMarket_;
	std::set<int> pastRtnTime_;
	//bool adjust_market_;
	bool hedge_inputs_;
	bool hedge_target_;
	int targetTime_;
	int sampleInterval_;
	std::string sample_dir_;

	int nInput_;
	int msecOpen_;
	int msecClose_;
	std::vector<std::string> inputNames_;
	std::ofstream ofs_;

	int nPastRtn_;

	std::vector<double> secRetCum_;
	std::vector<std::pair<int, int> > sessions_;
	std::map<std::string, double> mVol_;
	std::vector<TH2F*> h_corr_;
	double input_n_;
	std::vector<double> input_sum_;
	std::vector<double> input_sum2_;

	void get_sample(std::string ticker, const std::vector<double>* msecQ1s, const std::vector<double>* midQ1s,
		const std::vector<double>* msecQ1sL, const std::vector<double>* midQ1sL, std::vector<hnn::Sample>& sample);
	void get_quotes(const TickSeries<QuoteInfo>* ptsQ, std::vector<QuoteInfo>& vq);
};

#endif

