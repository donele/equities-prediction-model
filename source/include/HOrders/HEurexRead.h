#ifndef __HEurexRead__
#define __HEurexRead__
#include <HLib/HModule.h>
#include <HOrders.h>
#include <boost/thread.hpp>
#include <optionlibs/TickData.h>
#include <map>
#include <TH1.h>
#include <TProfile2D.h>
#include <TGraphErrors.h>
#include <string>
#include <vector>

class CLASS_DECLSPEC HEurexRead: public HModule {
public:
	HEurexRead(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HEurexRead();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginMarketDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

	struct MMtrade {
	public:
		MMtrade(){}
		MMtrade(std::string t, int o, int e, double p, int q, int bs, double b, double a, int as)
			:ticker(t), orderMsecs(o), eventMsecs(e), price(p), qty(q), bidSize(bs), bid(b), ask(a), askSize(as){}
		std::string ticker;
		int orderMsecs;
		int eventMsecs;
		double price;
		double qty;
		int bidSize;
		double bid;
		double ask;
		int askSize;
	};

private:
	int verbose_;
	int n_pnl_bins_;
	int half_pnl_bins_;
	int maxQty_;
	int max_o2q_;
	int min_t2e_;
	std::string db_name_;
	std::string tickSource_;
	boost::mutex tick_mutex_;
	boost::mutex hist_mutex_;

	int nTot_;
	int nTradeTickMatch_;
	int nTradetickMatchUniq_;
	int nAddTradeTickMatch_;
	int nAddTradetickMatchUniq_;
	TH1F hQty_;
	TH1F h_o2e_;
	TH1F h_o2t_;
	TH1F h_o2t_best_;
	TH1F h_o2t_uniq_;
	TH2F h_o2e_o2t_;
	TH1F hCand_;
	TH1F hCandA_;
	TProfile hCandDrift_;
	TProfile hCandQty_;
	TProfile2D hCand2d_;

	TH1F h_t2e_;
	TH1F h_t2e_best_;
	TH1F h_t2e_uniq_;
	TH1F h_rpnl_o_ms_;
	TH1F h_rpnl_s_;
	TH1F h_rpnl_ms_;
	TProfile p_rpnl_;
	TH1F h_dTicksize_;
	TH1F h_rpnl_dTicksize_;
	TProfile p_dTicksize_t2e_;
	TH1F h_nLooser_t2e_;
	std::vector<double> v_rpnl_o_ms_;
	std::vector<double> v_rpnl_s_;
	std::vector<double> v_rpnl_ms_;

	std::vector<std::string> tickers_;
	boost::mutex quote_mutex_;
	std::vector<std::vector<std::string> > vvoL_;
	std::vector<std::vector<std::string> > vvoA_;
	std::map<std::string, std::vector<TradeInfo> > mTickerVtrade_;
	std::map<std::string, std::vector<QuoteInfo> > mTickerVquote_;

	void read_all_tickdata(int idate);
	void set_ticker_list(int idate, std::string market);
	void read_orders(int idate, std::string market);
	void read_orders(std::vector<EurexOrder>& vMOrderBuy, std::vector<EurexOrder>& vMOrderSell,
		std::string market, std::string ticker, int idate);
	double get_rpnl(int msecs, std::string ticker, QuoteInfo quote0, char side, int qty);
	std::vector<MMtrade> get_matching_MMtrades(std::string ticker, int orderMsecs);
	std::vector<TradeInfo> get_matching_trade(std::string ticker, int orderMsecs, int eventMsecs, double price, int qty);
	std::vector<TradeInfo> get_matching_trade_A(std::string ticker, int orderMsecs, int eventMsecs, double price, int qty);
	void read_quote_info( std::vector<EurexOrder>& vMOrder, std::string market, std::string ticker, std::string bs);
	QuoteInfo get_quote(std::string ticker, int msecs);
};

#endif
