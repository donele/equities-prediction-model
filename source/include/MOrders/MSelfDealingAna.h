#ifndef __MSelfDealingAna__
#define __MSelfDealingAna__
#include <MFramework/MModuleBase.h>
#include <MOrders/TradeQuantileInfo.h>
#include <jl_lib/MercatorTrade.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GCurr.h>
#include <boost/thread.hpp>
#include <map>
#include <string>
#include <vector>
#include <list>

class CLASS_DECLSPEC MSelfDealingAna: public MModuleBase {
public:
	MSelfDealingAna(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MSelfDealingAna();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginMarketDay();
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
		PnlSummary():bpsHori(0),bpsIntra(0),bpsNofillHori(0),bpsNofillIntra(0),
			bpsClcl(0),bpsClclNext(0),bpsTotal1(0),bpsInter(0),bpsClop(0),
			bpsTotal2(0){}
		Acc accPnlHori;
		Acc accPnlIntra;
		Acc accPnlNofillHori;
		Acc accPnlNofillIntra;
		Acc accPnlClcl;
		Acc accPnlClclNext;
		Acc accPnlTotal1;
		Acc accPnlInter;
		Acc accPnlClop;
		Acc accPnlTotal2;

		Acc accDv;
		Acc accNTrades;
		Acc accDvNofill;
		Acc accNTradesNofill;

		double bpsHori;
		double bpsIntra;
		double bpsNofillHori;
		double bpsNofillIntra;
		double bpsClcl;
		double bpsClclNext;
		double bpsTotal1;
		double bpsInter;
		double bpsClop;
		double bpsTotal2;
	};

	struct PnlSum {
		PnlSum():pnlHori(0),pnlIntra(0),pnlNofillHori(0),pnlNofillIntra(0),pnlClcl(0),pnlClclNext(0),
			pnlInter(0), pnlClop(0),pnlClopNext(0),pnlOpcl(0),pnlOpclNext(0),nTickers(0),
			dv(0),nTrades(0),nTradesBuy(0),nTradesSell(0),nShares(0),dvNofill(0),nTradesNofill(0){}
		void adjust_exchrat(double exchrat) {pnlHori *= exchrat; pnlIntra *= exchrat;
	 		pnlNofillHori *= exchrat; pnlNofillIntra *= exchrat;
			pnlClcl *= exchrat; pnlClclNext *= exchrat;
			pnlInter *= exchrat; pnlClop *= exchrat;
			pnlClopNext *= exchrat;
			pnlOpcl *= exchrat; pnlOpclNext *= exchrat;
			dv *= exchrat; dvNofill *= exchrat;}
		double pnlHori;
		double pnlIntra;
		double pnlNofillHori;
		double pnlNofillIntra;
		double pnlClcl;
		double pnlClclNext;
		double pnlInter;
		double pnlClop;
		double pnlClopNext;
		double pnlOpcl;
		double pnlOpclNext;

		double nTickers;
		double dv;
		double nTrades;
		double nTradesBuy;
		double nTradesSell;
		double nShares;

		double dvNofill;
		double nTradesNofill;
	};

	bool debug_;
	int verbose_;
	int minTimeSinceLastOrder_;
	std::vector<char> execType_;
	std::vector<int> sign_;
	std::vector<int> schedType_;
	double fee_bpt_;
	std::map<std::string, CloseInfo> mClose_;
	std::map<std::string, CloseInfo> mCloseNext_;
	boost::mutex ps_mutex_;
	boost::mutex ta_mutex_;

	std::string condVar_;
	int iQuantile_;

	PnlSum psumDay_;
	PnlSummary ps_;
	std::map<std::string, PnlSummary> mPs_;
	const TradeQuantileInfo* tqi_;

	std::vector<std::string> getCondTickers();
	void readClosePricePosition(std::map<std::string, CloseInfo>& mClose, int idate);
	std::vector<int> selectTradeIndex(const std::vector<MercatorTrade>* pvmt);
	void intra_day_stat(const std::vector<MercatorTrade>* pvmt, const std::string& ticker);
	void update_summary(PnlSummary& summary, PnlSum& sum);
};


#endif
