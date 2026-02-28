#ifndef __MSelectTickers__
#define __MSelectTickers__
#include <MFramework/MModuleBase.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <jl_lib/crossCompile.h>

class CLASS_DECLSPEC MSelectTickers: public MModuleBase {
public:
	MSelectTickers(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MSelectTickers();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginMarketDay();
	virtual void endMarket();
	virtual void endJob();

private:
	int verbose_;
	std::string source_;
	std::string update_;
	bool inUniv_;
	int maxNticker_;
	std::vector<std::string> sectype_;
	std::map<std::string, std::vector<std::string> > marketTickers_;

	void set_ticker_list();
	void set_uid_list();
	void set_ticker_list_orders();
	std::string sel_univ();
	int get_idate_from(const std::string& market, int idate);
	int get_idate_to(const std::string& market, int idate);
	std::vector<std::string> select_sectype(const std::vector<std::string>& tickers);
};

#endif
