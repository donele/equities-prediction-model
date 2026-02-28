#ifndef __HNewsScope__
#define __HNewsScope__
#include <HLib/HModule.h>
#include <optionlibs/TickData.h>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HNewsScope: public HModule {
public:
	HNewsScope(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HNewsScope();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginMarket();
	virtual void endMarket();
	virtual void endDay();
	virtual void endJob();

private:
	int verbose_;
	std::string input_dir_;
	std::string input_file_;
	std::ifstream ifs_;
	std::string outdir_;

	std::string temp_line_;
	std::vector<std::string> lines_;
	std::map<std::string, std::map<std::string, TickSeries<QuoteInfo> > > mmTs_;
	std::map<std::string, std::string> mExtMarket_;
	std::map<std::string, std::string> mRICticker_;

	bool msecs_valid(std::string market, int msecs);
	void split_newsScope(std::vector<std::string>& sl, std::string& line);
	int get_idate(std::string& line);
	std::vector<std::string> get_tickers(std::string market, int idate);
	std::string get_RIC(std::string sl1);
	QuoteTime get_lineTime(std::string& t);
	std::string get_market(std::string& s);
	void write_tickdata(std::map<std::string, TickSeries<QuoteInfo> >& mTs, std::string market, QuoteTime nextCloseUTC, int msecsFirstData);
	int get_msecs(QuoteTime qt, std::string market);
	void add_data(std::map<std::string, TickSeries<QuoteInfo> >& mTs, std::string ticker, int msecs, std::vector<std::string>& sl);
};

#endif