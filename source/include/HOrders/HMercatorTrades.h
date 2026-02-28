#ifndef __HMercatorTrades__
#define __HMercatorTrades__
#include <HLib/HModule.h>
#include <jl_lib/MercatorTrade.h>
#include <optionlibs/TickData.h>
#include <map>
#include <string>
#include <vector>

class CLASS_DECLSPEC HMercatorTrades: public HModule {
public:
	HMercatorTrades(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HMercatorTrades();

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
	bool debug_;
	std::string source_;
	std::string source_ext_;
	std::string execType_;
	double min_price_;
	double max_price_;
	int min_msso_;
	int max_msso_;
	std::vector<std::vector<std::string>> trades_;
	std::vector<std::vector<std::string>> orders_;
	std::vector<std::string> vUniv_;
	std::vector<std::string> tickers_;

	int open_msecs_;
	void select_univ(const std::string& market, int idate);
	void read_trades(const std::string& market, int idate);
	void read_orders(const std::string& market, int idate);
	std::vector<MercatorTrade> getMtrades(const std::string& ticker);
	std::vector<MercatorTrade> getMorders(const std::string& ticker);
};

#endif
