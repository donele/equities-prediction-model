#ifndef __MTickReadNext__
#define __MTickReadNext__
#include <MFramework/MModuleBase.h>
#include <MFramework/MEvent.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
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

class CLASS_DECLSPEC MTickReadNext: public MModuleBase {
public:
	MTickReadNext(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MTickReadNext();

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
	bool preload_;
	bool longTicker_;
	bool remove_break_;
	int nnSubsetSize_;
	int nThreads_;
	TickSources tSources_;

	MSessions sessions_;
	SourceInfo siQuote_;
	SourceInfo siNbbo_;
	SourceInfo siSip_;
	SourceInfo siTrade_;
	SourceInfo siOrder_;
	SourceInfo siNews_;
	bool nbbo_trade_;

	std::string ticker_choice_;
	std::map<std::string, std::vector<std::string>> marketTickers_;

	void set_directories(const std::string& market, int idate);
	void readTickData(const std::string& ticker);
	template<class T> void read_data(const std::vector<std::string>& dirs, const std::string& name, int& n, const std::string& ticker, int idate);
};

template<class T>
void MTickReadNext::read_data(const std::vector<std::string>& dirs, const std::string& name, int& n, const std::string& ticker, int idate)
{
	// Read
	TickAccessMulti<T> ta;
	for( std::vector<std::string>::const_iterator it = dirs.begin(); it != dirs.end(); ++it )
		ta.AddRoot(*it, longTicker_);

	TickSeries<T> ts(200000, 1.2);
	ta.GetTickSeries(ticker, idate, &ts);
	n = ts.NTicks();

	// Add
	std::vector<T> v;
	v.reserve(n);
	ts.StartRead();
	T object;
	for( int i = 0; i < n; ++i )
	{
		ts.Read(&object);
		if( !remove_break_ || sessions_.inSession(object.msecs) )
			v.push_back(object);
	}
	MEvent::Instance()->add<std::vector<T> >(ticker, name, v);
}

#endif
