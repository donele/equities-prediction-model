#ifndef __MReadSignal__
#define __MReadSignal__
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>
#include <jl_lib/crossCompile.h>

class CLASS_DECLSPEC MReadSignal: public MModuleBase {
public:
	MReadSignal(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MReadSignal();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	bool etsTicker_;
	int ndays_;
	int nSpectator_;
	int cntFitData_;
	int cntOOSData_;
	int totFitRead_;
	int totOOSRead_;
	std::string fitDesc_;
	std::string sigType_;
	std::map<int, int> vNrowsFit_;
	std::map<int, int> vNrowsOOS_;
	std::vector<std::string> spectatorNames_;
	std::vector<std::string> targetNames_;
	std::vector<std::string> predNames_;
	std::vector<std::string> vTickers_;
	int nTargets_;

	//TargetPred targetPred_;
	void read_target_pred();

	std::vector<std::vector<std::vector<float> > > vvvSpectator_;
	std::vector<std::vector<std::vector<float> > > vvvSpectatorOOS_;
	std::map<int, std::pair<int, int> > fitOffset_;
	std::map<int, std::pair<int, int> > oosOffset_;
	std::map<int, std::pair<int, int> > fitOffsetRead_;
	std::map<int, std::pair<int, int> > oosOffsetRead_;

	void get_ets_tickers(int idate);
	int read_preds(const std::string& model, int idate, bool is_oos, int dayOffset, int nrows);
};

#endif
