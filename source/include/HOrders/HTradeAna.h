#ifndef __HTradeAna__
#define __HTradeAna__
#include <HLib/HModule.h>
#include <jl_lib/MercatorTrade.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GCurr.h>
#include <boost/thread.hpp>
#include "TH1.h"
#include "TProfile.h"
#include <map>
#include <string>
#include <vector>
#include <list>

class CLASS_DECLSPEC HTradeAna: public HModule {
public:
	HTradeAna(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HTradeAna();

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
		CloseInfo(double close_):close(close_), pos(0), nextOpen(0.), nextClose(0.), nextAdj(0.){}
		double close;
		int pos;
		double nextOpen;
		double nextClose;
		double nextAdj;
	};

	struct PnlSummary {
		Acc accPnlIntra;
		Acc accPnlClcl;
		Acc accPnlClclDaypos;
		Acc accPnlTotal1;

		Acc accBpsIntra;
		Acc accBpsClcl;
		Acc accBpsClclDaypos;
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
		PnlSum():pnlIntra(0),pnlClcl(0),pnlClclNext(0),pnlClclNotrade(0),pnlClclDaypos(0),pnlInter(0),
			pnlClop(0),pnlClopNext(0),pnlOpcl(0),pnlOpclNext(0),nTickers(0),
			nTrades(0),nTradesBuy(0),nTradesSell(0),nShares(0),dollarvol(0){}
		void adjust_exchrat(double exchrat) {pnlIntra *= exchrat; pnlClcl *= exchrat; pnlClclNext *= exchrat; pnlClclNotrade *= exchrat;
			pnlClclDaypos *= exchrat; pnlInter *= exchrat; pnlClop *= exchrat; dollarvol *= exchrat;}
		double pnlIntra;
		double pnlClcl;
		double pnlClclNext;
		double pnlClclNotrade;
		double pnlClclDaypos;
		double pnlInter;
		double pnlClop;
		double pnlClopNext;
		double pnlOpcl;
		double pnlOpclNext;

		double nTickers;
		double nTrades;
		double nTradesBuy;
		double nTradesSell;
		double nShares;
		double dollarvol;
	};

	class  CLASS_DECLSPEC TradeActivity {
	public:
		TradeActivity();
		TradeActivity(std::string market, int idate, int nBinsPerDay);
		~TradeActivity();
		void addTrade(double msecs, double sign, double vol, double prc, double pnlIntra, double pnlInter, double pnlClclNext);
		void endTickerDay(int lastClosePosition, double closePrice, double pnlTrcl,
				const std::vector<int>* msecQ, const std::vector<double>* midQ);
		double getCumPosChg();

		// Activity time series.
		double vNTradesBuy[400];
		double vNTradesSell[400];
		double vNShares[400];
		double vDollarvol[400];

		// Intraday pnl and Intersample pnl.
		double vPnlIntra[400];
		double vPnlInter[400];
		double vPnlClclNext[400];

		// Position time series
		double vSharePos[400];
		double vDollarPos[400];
		double vSharePosAbs[400];
		double vDollarPosAbs[400];

		double vPosChg[400];
		double vPrc[400];
	private:
		std::string market_;
		int nBinsPerDay_;
		int msecOpen_;
		int msecClose_;
	};

	class  CLASS_DECLSPEC TradeActivityAgg {
	public:
		TradeActivityAgg();
		TradeActivityAgg(std::string market, int idate, int nd, int nBinsPerDay);
		~TradeActivityAgg();
		void addTickerDay(const TradeActivity& trdAct, int iDay);

		// Activity time series.
		std::vector<double> vNTrades;
		std::vector<double> vNShares;
		std::vector<double> vDollarvol;
		std::vector<double> vSignedNTrades;

		// Intraday pnl and Intersample pnl.
		std::vector<double> vPnlIntra;
		std::vector<double> vPnlInter;
		std::vector<double> vPnlClclNext;

		// Position time series
		std::vector<double> vSharePos;
		std::vector<double> vDollarPos;
		std::vector<double> vSharePosAbs;
		std::vector<double> vDollarPosAbs;

		// Price series.
		std::vector<double> vPrc;
	private:
		std::string market_;
		int nBinsPerDay_;
		int msecOpen_;
		int msecClose_;
	};

	bool debug_;
	bool do_root_;
	int verbose_;
	int iDay_;
	int nDay_;
	int nBinsPerDay_;
	int nTickerDetailMax_;
	double nHours_;
	double fee_bpt_;
	int idate0_;
	std::string tradeModuleName_;
	std::map<std::string, CloseInfo> mClose_;
	std::map<std::string, CloseInfo> mCloseNext_;
	boost::mutex ps_mutex_;
	boost::mutex ta_mutex_;

	std::string id_;
	std::string msecName_;
	std::string prcName_;
	std::string volName_;
	std::string signName_;
	std::string schedName_;
	std::string midName_;
	std::string posName_;
	std::string sampleOpenName_;

	std::string condVar_;
	double minCondVar_;
	double maxCondVar_;
	std::vector<std::string> vCondTickers_;
	std::vector<std::string> vMostTraded_;
	std::vector<std::string> vMedTraded_;
	std::vector<std::string> vLeastTraded_;

	// Pnl summary
	PnlSum psumDay_;
	PnlSummary ps_;
	std::map<std::string, PnlSummary> mPs_;

	// Trade activity
	TradeActivityAgg trdActAgg_;
	std::map<std::string, TradeActivityAgg> mTrdActAgg_;

	// Tickerday hists.
	TH1F hPnlIntra_;
	TH1F hPnlClcl_;
	TH1F hPnlClclNext_;
	TH1F hPnlClclDaypos_;
	TH1F hNTrades_;
	TH1F hRatBuy_;
	TH1F hNShares_;
	TH1F hDollarvol_;
	TH1F hOpcl_;
	TH1F hClop_;
	TProfile pIntraOpcl1_;
	TProfile pIntraOpcl2_;
	TProfile pIntraOpclNext1_;
	TProfile pIntraOpclNext2_;
	TProfile pIntraClop1_;
	TProfile pIntraClop2_;
	TProfile pIntraClopNext1_;
	TProfile pIntraClopNext2_;
	TProfile pIntraDolpos_;
	TProfile pChgposLastpos_;

	// Vector for cum pnl plots.
	std::vector<double> vdPnlIntra_;
	std::vector<double> vdPnlClcl_;
	std::vector<double> vdPnlClclNext_;
	std::vector<double> vdPnlClclNotrade_;
	std::vector<double> vdPnlClclDaypos_;
	std::vector<double> vdPnlOpcl_;
	std::vector<double> vdPnlClopNext_;
	std::vector<double> vdPnlInter_;
	std::vector<double> vdPnlClop_;
	std::vector<double> vdPnlTotal_;

	void readClosePricePosition(std::map<std::string, CloseInfo>& mClose, int idate);
	void intra_day_stat2(const std::vector<MercatorTrade>* pvmt,
			const std::vector<int>* msecQ, const std::vector<double>* midQ, const std::string& ticker);
	TH1F make_hist(std::vector<double> v, std::string name, std::string title, std::string method, double xMax);
	void update_summary(PnlSummary& summary, PnlSum& sum);
	void plot_tradeActivity(std::string dir, TradeActivityAgg& taa, std::string market);
};


#endif
