#ifndef __MTickReadFut__
#define __MTickReadFut__
#include <MFramework/MModuleBase.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
#include <boost/thread.hpp>
#include <vector>
#include <string>
#include <map>
#include <jl_lib/crossCompile.h>

class CLASS_DECLSPEC MTickReadFut: public MModuleBase {
public:
	MTickReadFut(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MTickReadFut();

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
		std::string dir;
	};

private:
	int verbose_;
	bool longTicker_;
	int nnSubsetSize_;
	int nThreads_;

	std::vector<std::string> products_;
	SourceInfo siQuote_;
	SourceInfo siTrade_;
	SourceInfo siOrder_;

	template<class T> void add_tickdata(std::string ticker, std::string desc, TickSeries<T>& ts);
};

template<class T>
void MTickReadFut::add_tickdata(std::string ticker, std::string desc, TickSeries<T>& ts)
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
	MEvent::Instance()->add<std::vector<T> >(ticker, desc, v);
}

#endif
