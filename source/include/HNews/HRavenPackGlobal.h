#ifndef __HRavenPackGlobal__
#define __HRavenPackGlobal__
#include <HLib/HModule.h>
#include <jl_lib/NewsCatRPGlobal.h>
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

class CLASS_DECLSPEC HRavenPackGlobal: public HModule {
public:
	HRavenPackGlobal(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HRavenPackGlobal();

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
	NewsCatRPGlobal newsCatRPG_;

	std::string temp_line_;
	std::vector<std::string> lines_;
	std::map<std::string, std::map<std::string, TickSeries<QuoteInfo> > > mmTs_;

	bool msecs_valid(std::string market, int msecs, std::vector<std::string>& sl);
	int get_idate(std::string& line);
	std::vector<std::string> get_tickers(std::string market, int idate);
	std::string get_ticker(std::string sl1, std::string market, int idate);
	QuoteTime get_lineTime(std::string& t);
	void write_tickdata(std::map<std::string, TickSeries<QuoteInfo> >& mTs, std::string market, QuoteTime nextCloseUTC, int msecsFirstData);
	int get_msecs(QuoteTime qt, std::string market);
	void add_data(std::map<std::string, TickSeries<QuoteInfo> >& mTs, std::string ticker, int msecs, std::vector<std::string>& sl);
};

#endif