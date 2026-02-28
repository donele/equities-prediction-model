#ifndef __HSignalNewsBin__
#define __HSignalNewsBin__
#include <HLearnObj/Signal.h>
#include <HLearn/MCPred.h>
#include <HLearn/NewsSignal.h>
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

class CLASS_DECLSPEC HSignalNewsBin: public HModule {
public:
	HSignalNewsBin(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HSignalNewsBin();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	struct Pred {
		Pred(){}
		Pred(float t, float p):target(t), tbpred(p), resid(p - t){}
		float target;
		float tbpred;
		float resid;
	};
private:
	int nSignal_;
	//int nSpectator_;
	int minMsecs_;
	int maxMsecs_;
	int msecOpen_;
	int msecClose_;
	int minNews_;
	int maxNews_;
	std::string newsCutName_;
	bool write_signal_;
	int reqNewsIndx_;
	std::string targetName_;
	std::string market_;
	std::string input_dir_;
	std::string news_dir_;
	std::string pred_dir_;
	std::string output_dir_;
	std::string mrTarget_;
	std::vector<std::string> signalNames_;
	std::vector<std::string> inputNames_;
	//std::vector<std::string> spectatorNames_;
	std::vector<int> inputIndx_;
	std::ofstream ofs_;
	InputCounter ic_;
	std::ifstream ifs_;

	int ncols_;
	std::map<int, std::map<std::string, int> > mDateTickerCnt_;
	std::map<int, std::vector<std::string> > mDateVTicker_;
	std::map<std::string, std::vector<Pred> > mTickerPred_;

	NewsSignal news_;
	std::set<std::string> sEUmarkets_;
	std::vector<double> secRetCum_;
	std::vector<std::pair<int, int> > sessions_;
	std::map<std::string, double> mVol_;
	int nInput_;
	double input_n_;
	std::vector<double> input_sum_;
	std::vector<double> input_sum2_;

	std::vector<std::string> mr_labels_;
	MultiRegress mr_;

	std::vector<TH1F*> h_signal_;
	std::vector<TH1F*> h_signal_all_;
	TH1F h_r2_with_news_;

	std::string get_sigName(std::string market, int idate);
	std::string get_sigTxtName(std::string market, int idate);
	std::string get_predName(std::string market, int idate);
	//std::string get_sigFile(int idate);
	//std::string get_predFile(int idate);
	//std::string get_ticker(std::vector<std::string>& v);
	void read_sample(std::string ticker, int idate);
	void set_ticker_list(std::string market, int idate);
	void read_pred(std::string market, int idate);
	void get_signal(std::string ticker, const TickSeries<QuoteInfo>* ptsQ, std::vector<hnn::Signal>& vSignal);
	void get_quotes(const TickSeries<QuoteInfo>* ptsQ, std::vector<QuoteInfo>& vq);
	void get_signal(std::string ticker, std::vector<QuoteInfo>& vq, std::vector<hnn::Signal>& vSignal);
	void get_signal(std::string ticker, const std::vector<double>* msecQ1s, const std::vector<double>* midQ1s,
	const std::vector<double>* msecN1s, const std::vector<double>* valN1s, std::vector<hnn::Signal>& vSignal);
	void print_mr(MultiRegress& mr, std::string market, std::string flag = "");
	void update_hist(double r2, std::string market, std::string flag);
	void find_index(std::vector<std::string>& v);
	void write_signal(std::vector<hnn::Signal>& vSignal, std::string lastTicker);
	void add_signal(std::vector<std::string>& v, std::vector<hnn::Signal>& vSignal, std::string ticker,
		float target, float tbpred, float resid, float sent5d, float sent20d, float cosent5d, float cosent20d, float autocosent4d, float autocosent19d);
	void add_signal(std::vector<std::string>& v, std::vector<hnn::Signal>& vSignal, std::string ticker, float target, float tbpred, float resid,
		std::vector<float>& newsSignal);
	//void get_tickers(std::vector<std::string>& tickers, int idate);
};

#endif
