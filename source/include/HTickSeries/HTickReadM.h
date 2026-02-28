#ifndef __HTickReadM__
#define __HTickReadM__
#include <HLib/HModule.h>
#include <HLib/HEvent.h>
#include "optionlibs/TickData.h"
#include <TStopwatch.h>
#include <boost/thread.hpp>
#include <vector>
#include <string>
#include <map>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HTickReadM: public HModule {
public:
	HTickReadM(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HTickReadM();

	virtual void beginJob();
	virtual void beginMarketDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endMarketDay();
	virtual void endJob();

	struct SourceInfo {
		SourceInfo():do_read(false), dir_given(false){}
		bool do_read;
		bool dir_given;
		std::vector<std::string> dirs;
		std::map<std::string, std::vector<std::string> > mktDirs;
	};

private:
	int verbose_;
	bool longTicker_;
	int nnSubsetSize_;
	int nThreads_;

	SourceInfo siQuote_;
	SourceInfo siNbbo_;
	SourceInfo siTrade_;
	SourceInfo siOrder_;
	bool nbbo_trade_;

	std::string ticker_choice_;
	std::map<std::string, std::vector<std::string> > marketTickers_;

	// Used to get the names, and to read tickdata in a single thread mode.
	TickAccessMulti<QuoteInfo> taQ_;
	TickAccessMulti<QuoteInfo> taN_;
	TickAccessMulti<TradeInfo> taT_;
	TickAccessMulti<OrderData> taO_;
	TickSeries<QuoteInfo> tsQ_;
	TickSeries<QuoteInfo> tsN_;
	TickSeries<TradeInfo> tsT_;
	TickSeries<OrderData> tsO_;

	// Used to read tickdata in a multi thread mode.
	std::map<boost::thread::id, TickAccessMulti<QuoteInfo> > mtaQ_;
	std::map<boost::thread::id, TickAccessMulti<QuoteInfo> > mtaN_;
	std::map<boost::thread::id, TickAccessMulti<TradeInfo> > mtaT_;
	std::map<boost::thread::id, TickAccessMulti<OrderData> > mtaO_;
	std::map<boost::thread::id, TickSeries<QuoteInfo> > mtsQ_;
	std::map<boost::thread::id, TickSeries<QuoteInfo> > mtsN_;
	std::map<boost::thread::id, TickSeries<TradeInfo> > mtsT_;
	std::map<boost::thread::id, TickSeries<OrderData> > mtsO_;
	boost::mutex mtaQ_mutex_;
	boost::mutex mtaN_mutex_;
	boost::mutex mtaT_mutex_;
	boost::mutex mtaO_mutex_;
	boost::mutex mtsQ_mutex_;
	boost::mutex mtsN_mutex_;
	boost::mutex mtsT_mutex_;
	boost::mutex mtsO_mutex_;

	void set_directories(std::string market, int idate);
	void init_TickAccess();

	TickAccessMulti<QuoteInfo>& get_TickAccess_quote();
	TickAccessMulti<QuoteInfo>& get_TickAccess_nbbo();
	TickAccessMulti<TradeInfo>& get_TickAccess_trade();
	TickAccessMulti<OrderData>& get_TickAccess_order();

	TickSeries<QuoteInfo>& get_TickSeries_quote();
	TickSeries<QuoteInfo>& get_TickSeries_nbbo();
	TickSeries<TradeInfo>& get_TickSeries_trade();
	TickSeries<OrderData>& get_TickSeries_order();

	template<class T> void add_tickdata(std::string ticker, std::string desc, TickSeries<T>& ts);
};

template<class T>
void HTickReadM::add_tickdata(std::string ticker, std::string desc, TickSeries<T>& ts)
{
	int nTicks =  ts.NTicks();
	std::vector<T> v;
	v.reserve(nTicks);
	ts.StartRead();
	T object;
	for( int i=0; i<nTicks; ++i )
	{
		ts.Read(&object);
		v.push_back(object);
	}
	HEvent::Instance()->add<std::vector<T> >(ticker, desc, v);
}

#endif