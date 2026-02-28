#ifndef __HOrderSummRead_test__
#define __HOrderSummRead_test__
#include <HLib/HModule.h>
#include <jl_lib/MercatorOrder.h>
#include <HOrders.h>
#include <boost/thread.hpp>
#include <optionlibs/TickData.h>
#include <map>
#include <TH1.h>
#include <TGraphErrors.h>
#include <string>
#include <vector>

class CLASS_DECLSPEC HOrderSummRead_test: public HModule {
public:
	HOrderSummRead_test(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HOrderSummRead_test();

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
	bool quotedDest_;
	int verbose_;
	int latency_offset_;
	int driftMax_;
	int forwardMatchMax_;
	int fakeMsecsMax_;
	int q2tMax_;
	int q2oExtra_;
	int msecClose_;
	int targetMsecs_;
	int nticker_;
	std::string tickSource_;
	std::string dest_;
	boost::mutex tick_mutex_;
	boost::mutex hist_mutex_;
	std::map<int, std::string> m_us_dest_routing_;
	std::map<int, int> m_us_dbdest_;
	std::string routing_;

	TickAccessMulti<QuoteInfo> taQ_;
	TickAccessMulti<OrderData> taO_;
	TickAccessMulti<TradeInfo> taT_;

	std::vector<std::string> vUniv_;
	std::vector<std::string> tickers_;
	boost::mutex quote_mutex_;
	std::vector<std::vector<std::string> > vvo_;

	std::string get_selMarket(std::string market);
	void set_ticker_list(int idate, std::string market);
	void read_orders(int idate, std::string market);
	void read_orders(std::vector<MercatorOrder>& vMOrderBuy, std::vector<MercatorOrder>& vMOrderSell,
		std::string market, std::string ticker, int idate);
	void set_latency_offset(int idate, std::string market);
	void read_quote_info( std::vector<MercatorOrder>& vMOrder, std::string market, std::string ticker, std::string bs,
		TickSeries<QuoteInfo>& tsQ, TickSeries<OrderData>& tsO, TickSeries<TradeInfo>& tsT);
	bool is_valid( QuoteInfo& quote );
	int get_price_change( double last, double now );
	bool valid_price( double price );
	void get_o2x( std::vector<MercatorOrder>& MOrder, int m1, int m2, double price, int priceDeteriorated );
};

#endif
