#ifndef __mto__
#define __mto__
#include <jl_lib/crossCompile.h>
#include <string>
#include <vector>

namespace mto {
	FUNC_DECLSPEC std::string ex(const std::string& m);
	FUNC_DECLSPEC std::string country(const std::string& m);
	FUNC_DECLSPEC std::string MIC(const std::string& m);
	FUNC_DECLSPEC std::string exRIC(const std::string& m);
	FUNC_DECLSPEC std::string tz(const std::string& m, int idate = 0);
	FUNC_DECLSPEC std::string region(const std::string& m);
	FUNC_DECLSPEC bool isInternational(const std::string& m);
	FUNC_DECLSPEC std::string region_long(const std::string& m);
	FUNC_DECLSPEC std::string code(const std::string& m);
	FUNC_DECLSPEC int exFlag(const std::string& m);
	FUNC_DECLSPEC bool longTicker(const std::string& m);
	FUNC_DECLSPEC std::string uidHead(const std::string& m);
	FUNC_DECLSPEC std::string hf(const std::string& m);
	FUNC_DECLSPEC std::string hfpar(const std::string& m, int idate);
	FUNC_DECLSPEC std::string hfdbg(const std::string& m);
	FUNC_DECLSPEC std::string hfo(const std::string& m, int idate = 0);
	FUNC_DECLSPEC std::string ok(const std::string& m);
	FUNC_DECLSPEC std::string th(const std::string& m);
	FUNC_DECLSPEC std::string bindir(const std::string& m, int idate);
	FUNC_DECLSPEC std::string bindirBook(const std::string& m, int idate);
	FUNC_DECLSPEC std::string bindirReturn(const std::string& m, int idate);
	FUNC_DECLSPEC std::string bindirFuture(const std::string& m, int idate);
	std::vector<std::string> dests(const std::string& m);
	FUNC_DECLSPEC std::string nbbodir(const std::string& m, int idate);
	FUNC_DECLSPEC std::vector<std::string> bindirs(const std::string& m, int idate);
	FUNC_DECLSPEC std::vector<std::string> nbbodirs(const std::string& m, int idate);
	FUNC_DECLSPEC std::vector<std::string> bindirsBook(const std::string& m, int idate);
	FUNC_DECLSPEC std::string extel(const std::string& m);
	FUNC_DECLSPEC std::string extelCountry(const std::string& m);
	FUNC_DECLSPEC std::string city(const std::string& m);
	FUNC_DECLSPEC std::string currQAI(const std::string& m);
	FUNC_DECLSPEC std::string currISO(const std::string& m);
	FUNC_DECLSPEC std::string currChara(const std::string& m);
	FUNC_DECLSPEC double currWgt(const std::string& m);
	FUNC_DECLSPEC double currWgtMerc(const std::string& m);
	FUNC_DECLSPEC double fee_bpt(const std::string& m, double price = 0.);
	double fee_bpt(const std::string& m, char primex, double price);
	FUNC_DECLSPEC std::string selChara(int idate);
	FUNC_DECLSPEC std::string selChara(const std::string& m, int idate);
	FUNC_DECLSPEC std::string selChara(const std::string& m, int idate0, int idata1);
	std::string selTradeTime(const std::string& m, int idate);
	FUNC_DECLSPEC std::string selOrder(const std::string& m, int idate);
	FUNC_DECLSPEC std::string selVal(const std::string& m);
	FUNC_DECLSPEC std::string selInfo(const std::string& m);
	FUNC_DECLSPEC std::string selType(const std::string& m);
	FUNC_DECLSPEC std::string selTypeTight(const std::string& m);
	FUNC_DECLSPEC std::string ts(const std::string& m);
	FUNC_DECLSPEC std::string compTicker(const std::string& m);
	FUNC_DECLSPEC std::string compTicker(const std::string& m, int idate);
	FUNC_DECLSPEC std::string compTicker(const std::string& ticker, const std::string& market);
	FUNC_DECLSPEC std::string compTicker(const std::string& ticker, const std::string& market, int idate);
	FUNC_DECLSPEC std::vector<std::string> compTickers(const std::vector<std::string>& tickers, const std::string& market);
	FUNC_DECLSPEC std::string retName(const std::string& m);
	FUNC_DECLSPEC std::string andMarketSel(const std::string& m);
	FUNC_DECLSPEC int msecOpen(const std::string& m, int idate = 99999999);
	FUNC_DECLSPEC int msecClose(const std::string& m, int idate = 99999999);
	FUNC_DECLSPEC std::vector<std::pair<int, int> > breaks(const std::string& market, int delay = 0, int idate = 99999999);
	FUNC_DECLSPEC std::vector<std::pair<int, int> > sessions(const std::string& market, int delayOpen = 0, int delayClose = 0, int delayOther = 0, int idate = 99999999);
	FUNC_DECLSPEC bool dataOK(const std::string& market, int idate);
	FUNC_DECLSPEC std::vector<int> dates(const std::string& market, int d1, int d2);
	FUNC_DECLSPEC int lotSize(const std::string& market, int idate);
	FUNC_DECLSPEC double convertPriceQAItoChara(double qai_price, const std::string& currQAI);
}

#endif
