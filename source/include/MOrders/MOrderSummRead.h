#ifndef __MOrderSummRead__
#define __MOrderSummRead__
#include <MFramework/MModuleBase.h>
#include <jl_lib.h>
#include <boost/thread.hpp>
#include <optionlibs/TickData.h>
#include <map>
#include <jl_lib.h>
#include <string>
#include <vector>

class CLASS_DECLSPEC MOrderSummRead: public MModuleBase {
public:
	MOrderSummRead(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MOrderSummRead();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();
	void beginMarketDay(std::string market, int idate);

private:
	bool debug_;
	bool quotedDest_;
	bool tokyo_colo_chicago_logged_;
	bool use_test_data_;
	int verbose_;
	int latency_offset_;
	int driftMax_;
	int fakeMsecsMax_;
	int q2oMin_;
	int q2oMax_;
	int q2dMin_;
	int q2dMax_;
	int q2oExtra_;
	int msecClose_;
	int targetMsecs_;
	int nticker_;
	std::string region_;
	std::string tickSource_;
	std::string dest_;
	boost::mutex db_mutex_;
	boost::mutex tick_mutex_;
	boost::mutex hist_mutex_;
	std::map<int, std::vector<std::string>> m_us_dest_routing_;
	std::map<int, int> m_us_dbdest_;
	std::string routing_;

	TickAccessMulti<QuoteInfo> taQ_;
	TickAccessMulti<OrderData> taO_;
	TickAccessMulti<TradeInfo> taT_;

	std::vector<std::string> vUniv_;
	std::vector<std::string> tickers_;
	boost::mutex quote_mutex_;
	std::vector<std::vector<std::string> > vvo_;
	std::map<std::string, double> mRetON_;

	std::string get_selMarket(std::string market);
	bool exchangeMatch(int tickEx, int dbEx);
	void set_ticker_list(int idate, std::string market);
	void read_orders(std::vector<std::vector<std::string> >& vvo, int idate, std::string market, std::string ticker = "");
	void select_orders(std::vector<std::vector<std::string> >& vvo, std::vector<MercatorOrder>& vMOrderBuy, std::vector<MercatorOrder>& vMOrderSell,
		std::string market, std::string ticker, int idate);
	void set_latency_offset(int idate, std::string market);
	void read_quote_info( std::vector<MercatorOrder>& vMOrder, std::string market, std::string ticker, std::string bs,
		TickSeries<QuoteInfo>& tsQ, TickSeries<OrderData>& tsO, TickSeries<TradeInfo>& tsT);
	bool is_valid( QuoteInfo& quote );
	int get_price_change( double last, double now );
	bool valid_price( double price );
	void get_o2x( std::vector<MercatorOrder>& MOrder, int m1, int m2, double price, int priceDeteriorated );
	void calc_returns(TickSeries<QuoteInfo>& tsQ, int orderMsecs, std::vector<MercatorOrder>::iterator& itb, const int mult, int closeMsecs, double reton);
	void find_match(QuoteInfo& theQuote, TickSeries<QuoteInfo>& tsQ, int orderMsecs, std::vector<MercatorOrder>::iterator& itb, std::string bs);
	void get_retON(std::string markte, int idate);
};

#endif
