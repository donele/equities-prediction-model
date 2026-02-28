#ifndef __MTickReadIndex__
#define __MTickReadIndex__
#include <MFramework/MModuleBase.h>
#include <optionlibs/TickData.h>
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

class CLASS_DECLSPEC MTickReadIndex: public MModuleBase {
public:
	MTickReadIndex(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MTickReadIndex();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	int verbose_;
	bool bps_;
	bool fitting_index_;
	bool do_clip_;
	std::string return_dir_;
	std::map<std::string, std::string> m_return_dir_;
	std::string format_;
	std::set<std::string> retNames_;
	std::set<std::string> tickersRead_;

	void read_etf(const std::string& market, int idate);
	void read_dir(const std::string& dir, const std::string& market, int idate);
	void add_return(int n1sec, const std::string& name, TickSeries<ReturnData>& ts, int openMsecs, int idate);
	void remove_data(int idate);
};

#endif
