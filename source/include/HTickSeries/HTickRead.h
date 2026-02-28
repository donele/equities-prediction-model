#ifndef __HTickRead__
#define __HTickRead__
#include <HLib/HModule.h>
#include <HLib/HEnv.h>
#include <HLib/HEvent.h>
#include "optionlibs/TickData.h"
#include <TStopwatch.h>
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

class CLASS_DECLSPEC HTickRead: public HModule {
public:
	HTickRead(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HTickRead();

	virtual void beginJob();
	virtual void beginMarketDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endMarketDay();
	virtual void endJob();

private:
	int verbose_;
	int nnSubsetSize_;
	int dd_;
	int read_date_;
	std::string desc_;

	bool read_quote_;
	bool read_nbbo_;
	bool read_trade_;
	bool read_order_;
	bool read_return_;
	bool nbbo_trade_;

	bool quote_dir_given_;
	bool nbbo_dir_given_;
	bool trade_dir_given_;
	bool order_dir_given_;
	bool return_dir_given_;

	//std::string quote_name_;
	//std::string nbbo_name_;
	//std::string trade_name_;
	//std::string order_name_;
	std::vector<std::string> quote_dir_;
	std::vector<std::string> nbbo_dir_;
	std::vector<std::string> trade_dir_;
	std::vector<std::string> order_dir_;
	std::string return_dir_;

	std::map<std::string, std::vector<std::string> > m_quote_dir_;
	std::map<std::string, std::vector<std::string> > m_nbbo_dir_;
	std::map<std::string, std::vector<std::string> > m_trade_dir_;
	std::map<std::string, std::vector<std::string> > m_order_dir_;
	std::map<std::string, std::string> m_return_dir_;

	std::string format_;
	std::string ticker_choice_;
	std::map<std::string, std::vector<std::string> > marketTickers_;

	TStopwatch swThreadRead_;
	std::map<std::string, std::set<std::string> > tickReadLog_;
	boost::mutex quote_mutex_;
	boost::mutex nbbo_mutex_;
	boost::mutex trade_mutex_;
	boost::mutex order_mutex_;
	boost::mutex event_mutex_;

	TickAccessMulti<QuoteInfo>* ptaQ_;
	TickAccessMulti<QuoteInfo>* ptaN_;
	TickAccessMulti<TradeInfo>* ptaT_;
	TickAccessMulti<OrderData>* ptaO_;
	TickAccess<ReturnData>* ptaR_;

	std::string getName(const std::string& tickType, const std::string& storageType);
	template<class T> void add_tickdata_TS(std::string ticker, std::string desc, TickSeries<T>& ts);
	template<class T> void add_tickdata_vector(std::string ticker, std::string desc, TickSeries<T>& ts);
	std::vector<std::string> get_noss_tickers(std::string market, int idate);
};

template<class T>
void HTickRead::add_tickdata_TS(std::string ticker, std::string desc, TickSeries<T>& ts)
{
	boost::mutex::scoped_lock lock(event_mutex_);
	HEvent::Instance()->add<TickSeries<T> >(ticker, desc, ts);
}

template<class T>
void HTickRead::add_tickdata_vector(std::string ticker, std::string desc, TickSeries<T>& ts)
{
	boost::mutex::scoped_lock lock(event_mutex_);
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
