#ifndef __HRavenPackAggMarketRet__
#define __HRavenPackAggMarketRet__
#include <HLib/HModule.h>
#include <HNews.h>
//#include <HSecRet.h>
#include <jl_lib/NewsCatRP.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
#include <map>
#include <string>
#include <set>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HRavenPackAggMarketRet: public HModule {
public:
	HRavenPackAggMarketRet(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HRavenPackAggMarketRet();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	struct DailySummary {
		std::map<int, Acc> idateAcc;
		Acc agg;
	};

private:
	class NewsAggregate {
	public:
		NewsAggregate(std::string market, int nDayAgg, int interval);
		void beginDay(int idate, int msecOpen, int msecClose, int nInterval);
		void add_data(std::string ticker, std::string sector, int msecs, std::vector<std::string>& sl/*, int& avg*/);
		void aggregate(int idate);
		int get_volume(std::string ticker);
		int get_ticker_sent(std::string ticker);
		int get_ticker_sent_lag(std::string ticker);
		std::vector<int> get_sector_sent(std::string sector);
		std::vector<double> get_market_sent();
		void get_tickers_without_news(std::vector<std::string>& tickers);
	private:
		int nDayAgg_;
		int msecOpen_;
		int msecClose_;
		int interval_;
		int nInterval_;
		std::string market_;

		std::map<std::string, Acc> tickerVol_;
		std::map<std::string, Acc> tickerAcc_;
		std::map<std::string, std::vector<Acc> > mvSectorAcc_;
		std::vector<Acc> vMarketAcc_;

		std::map<std::string, DailySummary> tickerDailyVol_;
		std::map<std::string, DailySummary> tickerDailySummary_;
		std::map<std::string, DailySummary> sectorDailySummary_;
		DailySummary marketDailySummary_;

		double get_avg_sent(DailySummary& ds, Acc& acc);
		void add_acc_today(DailySummary& ds, int idate, std::string ticker);
		void add_acc_today_market(int idate);
		void aggregate(DailySummary& ds, int beginIdate);
	};

private:
	int verbose_;
	std::set<int> aggregationWindows_;
	int nWindow_;
	int msecOpen_;
	int msecClose_;
	int interval_; // seconds.
	int nInterval_;
	//int category_size_;
	//std::string category_;
	std::string market_;
	std::string country_;
	//std::string outdirBase_;
	std::string outdir_;
	NewsCatRP newsCatRP_;
	CompaniesRP companies_;
	OneSecRet ret_;

	std::map<std::string, TickSeries<QuoteInfo> > mTs_;
	std::map<int, NewsAggregate> mNdayNewsagg_;

	std::string get_outdir();
	void process_market(int idate);
	int get_msecs(QuoteTime qt);
	void reset_times(QuoteTime linetimeUTC, QuoteTime& nextOpenUTC, QuoteTime& nextCloseUTC, int& msecsFirstData);

	bool msecs_valid(int msecs, std::vector<std::string>& sl);
	int get_idate(std::string& line);
	std::vector<std::string> get_tickers(int idate);
	QuoteTime get_lineTime(std::string& t);

	void aggregate(int idate);
	void write_tickdata(QuoteTime nextCloseUTC, int msecsFirstData, std::set<std::string>& sectors);
	//void import_sectors(TickStorage<QuoteInfo>& tsQ, std::set<std::string>& sectors);
	void import_market(TickStorage<QuoteInfo>& tsQ, int idate);
	void add_data(std::string ticker, std::string sector, int msecs, std::vector<std::string>& sl);
	//void add_tickers_without_news(int idate);
};

#endif
