#ifndef __MMercatorTrades__
#define __MMercatorTrades__
#include <MFramework/MModuleBase.h>
#include <MOrders/TradeQuantileInfo.h>
#include <jl_lib/Sessions.h>
#include <jl_lib/MercatorTrade.h>
#include <gtlib_signal/QuoteView.h>
#include <optionlibs/TickData.h>
#include <map>
#include <string>
#include <vector>

class CLASS_DECLSPEC MMercatorTrades: public MModuleBase {
public:
	MMercatorTrades(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MMercatorTrades();

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
	bool readNofill_;
	bool debug_tqi_;
	std::string source_;
	std::string groupBy_;
	double min_price_;
	double max_price_;
	int min_msso_;
	int max_msso_;
	std::vector<std::vector<std::string>> orders_;
	std::vector<std::string> vUniv_;
	std::vector<std::string> tickers_;
	std::map<std::string, float> mDayVolat_;
	std::map<std::string, float> mMedVolat_;
	std::map<std::string, float> mDayVolume_;
	std::map<std::string, float> mMedVolume_;
	std::vector<float> vVolatSurprise30s_;
	std::vector<float> vVolatSurprise300s_;

	bool requireValidQuote_;
	bool fillZeros_;
	bool allowCross_;
	int msecOpen_;
	int msecClose_;
	Sessions sessions_;

	void select_univ(const std::string& market, int idate);
	std::vector<std::vector<std::string>> read_orders(const std::string& market, int idate);
	std::vector<MercatorTrade> getMtrades(const std::string& ticker);
	void addTickInfo(const std::string& ticker, std::vector<MercatorTrade>& trades);
	void addTrades(const std::string& ticker);
	double getTimeFrac(int sec);
	TradeQuantileInfo getTradeQuantileInfo();
};

#endif
