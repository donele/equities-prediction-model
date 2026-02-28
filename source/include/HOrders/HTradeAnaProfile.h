#ifndef __HTradeAnaProfile__
#define __HTradeAnaProfile__
#include <HLib/HModule.h>
#include <jl_lib/jlutil.h>
#include <boost/thread.hpp>
#include "TH1.h"
#include <map>
#include <string>
#include <vector>
#include <list>

class CLASS_DECLSPEC HTradeAnaProfile: public HModule {
public:
	HTradeAnaProfile(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HTradeAnaProfile();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	struct CloseInfo {
		CloseInfo(){}
		CloseInfo(double close_):close(close_), pos(0), nextOpen(0), nextClose(0){}
		double close;
		int pos;
		double nextOpen;
		double nextClose;
	};

	struct PnlSummary {
		Acc accPnlIntra;
		Acc accPnlClcl;
		Acc accPnlTotal1;

		Acc accBpsIntra;
		Acc accBpsClcl;
		Acc accBpsTotal1;

		Acc accPnlInter;
		Acc accPnlClop;
		Acc accPnlTotal2;

		Acc accBpsInter;
		Acc accBpsClop;
		Acc accBpsTotal2;

		Acc accDollarvol;
		Acc accNTrades;
	};

	struct PnlSum {
		PnlSum():pnlIntra(0),pnlClcl(0),pnlInter(0),pnlClop(0),nTrades(0),nTradesBuy(0),nTradesSell(0),nShares(0),dollarvol(0){}
		double pnlIntra;
		double pnlClcl;
		double pnlInter;
		double pnlClop;

		double nTrades;
		double nTradesBuy;
		double nTradesSell;
		double nShares;
		double dollarvol;
	};

	bool debug_;
	int verbose_;
	double costMult_;
	int iDay_;
	double nHours_;
	double fee_bpt_;
	int idate0_;
	int nP_;
	std::string tradeModuleName_;
	std::map<std::string, CloseInfo> mClose_;
	boost::mutex ps_mutex_;
	CharaProfile charaProfile_;
	std::string condVar_;

	std::string id_;
	std::string msecName_;
	std::string prcName_;
	std::string volName_;
	std::string signName_;
	std::string midName_;
	std::string posName_;
	std::string sampleOpenName_;

	// Pnl summary
	std::vector<PnlSum> psumDay_;
	std::vector<PnlSummary> ps_;
	std::map<std::string, PnlSummary> mPs_;

	void readClosePricePosition();
	void intra_day_stat(const std::vector<double>* msecT, const std::vector<double>* prcT, const std::vector<double>* volT,
		const std::vector<double>* signT, const std::vector<double>* midT,
		const std::vector<double>* msecQ, const std::vector<double>* midQ, std::string ticker);
	void update_summary(PnlSummary& summary, PnlSum& sum);
};


#endif