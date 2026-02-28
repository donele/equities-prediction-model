#ifndef __MReadTickerState__
#define __MReadTickerState__
#include <MFramework.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <MFitting/HFTrees.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MReadTickerState: public MModuleBase {
public:
	MReadTickerState(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MReadTickerState();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

	struct TickerState {
		TickerState(){}
		TickerState(int m, int bs, float b, float a, int as, float t, float p, float po, float s, float c):msecs(m), bidSize(bs), bid(b), ask(a), askSize(as), target(t), pred(p), predON(po), sprd(s), cost(c){}
		int msecs;
		int bidSize;
		float bid;
		float ask;
		int askSize;
		float target;
		float pred;
		float predON;
		float sprd;
		float cost;
	};

private:
	bool debug_;
	bool addON_;
	bool rollingModelDate_;
	int interval_;
	int minMsecsON_;
	int maxMsecsON_;
	double fee_;
	std::string om_pred_name_;
	std::string tm_pred_name_;
	std::string om_model_;
	std::string tm_model_;
	std::string om_pred_model_;
	std::string tm_pred_model_;
	std::string on_model_;
	std::string om_desc_;
	std::string tm_desc_;
	Corr corr_;

	std::map<std::string, std::vector<TickerState> > mTickerState_;

	void get_ticker_state();
	void get_ticker_state_match_time();
};

#endif
