#ifndef __HFillTime__
#define __HFillTime__
#include <HLib/HModule.h>
#include <jl_lib/MercatorOrder.h>
#include <HOrders.h>
#include <HLib.h>
#include <optionlibs/TickData.h>
#include <map>
#include <TH1.h>
#include <TGraphErrors.h>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HFillTime: public HModule {
public:
	HFillTime(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HFillTime();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	int verbose_;
	int latency_offset_;
	int q2oMax_;
	int driftMax_;
	int forwardMatchMax_;
	int fakeMsecsMax_;
	std::string procName_;
	int orderSchedType_;
	std::string tickSource_;
	char dest_;

	std::vector<std::string> tickers_;
	boost::mutex quote_mutex_;
	std::vector<std::vector<std::string> > vvo_;

	void set_ticker_list(int idate, std::string market);
	void read_orders(int idate, std::string market);
	void read_orders(std::vector<MercatorOrder>& vMOrderBuy, std::vector<MercatorOrder>& vMOrderSell,
		std::string market, std::string ticker, int idate);
	void set_latency_offset(int idate, std::string market);
	void read_quote_info( std::vector<MercatorOrder>& vMOrder, std::string market, std::string ticker, std::string bs,
		TickSeries<QuoteInfo>& tsQ, TickSeries<OrderData>& tsO);
	bool is_valid( QuoteInfo& quote );
	int get_price_change( double last, double now );
	bool valid_price( double price );
	void get_o2x( std::vector<MercatorOrder>& MOrder, int m1, int m2, double price, int priceDeteriorated );
};

#endif
