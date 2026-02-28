#ifndef __MInitModel__
#define __MInitModel__
#include <MFramework/MModuleBase.h>
#include "optionlibs/TickData.h"
#include <boost/thread.hpp>
#include <vector>
#include <string>
#include <map>
#include <set>

class MInitModel: public MModuleBase {
public:
	MInitModel(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MInitModel();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	int verbose_;
	int cntDay_;
	bool multiThreadModule_;
	bool multiThreadTicker_;
	bool requireDataOK_;
	int nMaxThreadTicker_;
	int udate_;
	int d1_;
	int d2_;
	int ndatesFront_;
	int ndates_;
	int noosdates_;
	int elapsed_prev_;
	std::string model_;
	std::string ticker_choice_;
	std::vector<std::pair<int, int>> excludeRange_;
	std::map<std::string, std::vector<std::string> > marketTickers_;

	bool isExcludeDate(int idate);
	int getFirstDayToWrite(const std::string& market, const std::string& type);
	int arg_idate(const std::string& sdate);
	void set_idate_list_ok();
	void set_idate_list_trade();
	void set_ticker_list();
	void limitTickerList(int nTickerMax);
	void set_uid_list();
	std::string sel_univ();
	int get_idate_from(const std::string& market, int idate);
	int get_idate_to(const std::string& market, int idate);
};

#endif
