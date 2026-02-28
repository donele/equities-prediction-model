#ifndef __HCloseToCloseAna__
#define __HCloseToCloseAna__
#include <HLib/HModule.h>
#include <jl_lib/jlutil.h>
#include <boost/thread.hpp>
#include "TH1.h"
#include "TProfile.h"
#include <map>
#include <string>
#include <vector>
#include <list>

class CLASS_DECLSPEC HCloseToCloseAna: public HModule {
public:
	HCloseToCloseAna(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HCloseToCloseAna();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	struct PastTrade {
		PastTrade():msecs(0),iday(0),pos(0){}
		PastTrade(int m, int i, int p):msecs(m), iday(i), pos(p){}
		int msecs;
		int iday;
		int pos;
	};
	struct CloseInfo {
		CloseInfo(){}
		CloseInfo(double close_):close(close_), pos(0), nextOpen(0.), nextClose(0.), nextAdj(0.){}
		double close;
		int pos;
		double nextOpen;
		double nextClose;
		double nextAdj;
	};

	bool debug_;
	int verbose_;
	int nBinsPerDay_;
	int secsPerBin_;
	int iday_;
	int openMsecs_;
	int closeMsecs_;
	std::string tradeModuleName_;
	std::map<std::string, CloseInfo> mCloseNext_;
	boost::mutex ptrades_mutex_;

	std::string id_;
	std::string msecName_;
	std::string prcName_;
	std::string volName_;
	std::string signName_;
	std::string schedName_;
	std::string midName_;
	std::string posName_;
	std::string sampleOpenName_;

	TH1F hNTickers_;
	TH1F hNShares_;
	TH1F hDollarvol_;
	TH1F hNSharesAbs_;
	TH1F hDollarvolAbs_;

	TH1F hPnlSumClcl_;
	TH1F hPnlSumClop_;
	TH1F hPnlSumOpcl_;

	TProfile pPnlClcl_;
	TProfile pPnlClop_;
	TProfile pPnlOpcl_;

	TProfile pRtnClcl_;
	TProfile pRtnClop_;
	TProfile pRtnOpcl_;

	void readClosePricePosition(std::map<std::string, CloseInfo>& mClose, int idate);
	//std::map<std::string, std::list<PastTrade> > mTickerTrade_;
	void add_trade(std::list<PastTrade>& ptrades, PastTrade newTrade);
};


#endif
