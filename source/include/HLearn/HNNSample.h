#ifndef __HNNSample__
#define __HNNSample__
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

class CLASS_DECLSPEC HNNSample: public HModule {
public:
	HNNSample(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HNNSample();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	class Preds {
	public:
		Preds();
		Preds(std::string pred_dir, std::string market, int idate);
		double get_pred1m(std::string ticker, int msecs);
		double get_pred40m(std::string ticker, int msecs);
		double get_pred41m(std::string ticker, int msecs);

	private:
		std::string market_;
		int idate_;
		int N_;
		std::map<std::string, std::vector<double> > mTickerPred1m_;
		std::map<std::string, std::vector<double> > mTickerPred40m_;

		void flush_pred(std::string uid, std::vector<double>& vPred1m, std::vector<double>& vPred40m);
		void add_line(std::vector<std::string>& v, std::vector<double>& vPred1m, std::vector<double>& vPred40m);
		double get_pred(std::map<std::string, std::vector<double> >& m, std::string ticker, int msecs);
	};

private:
	int nInput_;
	int sampleInterval_;
	int msecOpen_;
	int msecClose_;
	int lotSize_;
	int nSecInterval_;
	//bool adjust_market_;
	bool hedge_inputs_;
	bool hedge_target_;
	bool log_share_;
	int lot_size_;
	std::string sample_dir_;
	std::string pred_dir_;
	std::vector<std::string> inputNames_;
	std::ofstream ofs_;

	int targetTime_;
	std::set<int> pastRtnTime_;
	int nPastRtn_;
	bool input_bidSize_;
	bool input_askSize_;
	bool input_spread_;
	bool input_quoteImb_;
	bool input_volume_;
	bool input_pred1m_;
	bool input_pred1m_log_;
	bool input_pred40m_;
	bool input_pred40m_log_;
	bool input_pred41m_;
	Preds preds_;

	std::vector<double> secRetCum_;
	std::vector<std::pair<int, int> > sessions_;
	std::map<std::string, double> mVol_;
	std::vector<TH2F*> h_corr_;
	double input_n_;
	std::vector<double> input_sum_;
	std::vector<double> input_sum2_;

	void get_sample(std::string ticker, const TickSeries<QuoteInfo>* ptsQ, std::vector<hnn::Sample>& sample);
	void get_quotes(const TickSeries<QuoteInfo>* ptsQ, std::vector<QuoteInfo>& vq);
	void get_sample(std::string ticker, std::vector<QuoteInfo>& vq, std::vector<hnn::Sample>& sample);
};

#endif

