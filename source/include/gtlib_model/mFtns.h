#ifndef __mFtns__
#define __mFtns__
#include <gtlib_signal/MercatorVolume.h>
#include <jl_lib/Sessions.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <optionlibs/TickData.h>

namespace gtlib {

std::vector<int> getAllIdates(const std::string& model);
std::map<std::string, std::vector<std::string>> getMarketTickers(const std::string& model2,
		const std::vector<std::string>& markets, int idateFrom, int idateTo, int nTickerMax = 1000000);
std::map<std::string, std::vector<std::string>> getMarketTickersHighVol(const std::string& model2,
		const std::vector<std::string>& markets, int idateFrom, int idateTo, double x);
std::string sel_univ(const std::string& model2);
std::vector<float> getBreakSeries();
std::vector<float> getThresSeries();
std::vector<float> getMaxPosSeries();
std::vector<std::pair<float, float>> getSprdSimpleRanges(const std::string& model);
std::vector<std::pair<float, float>> getSprdFineRanges(const std::string& model);
std::vector<std::pair<float, float>> getSprdRanges(const std::string& model);
std::vector<std::pair<float, float>> getSprdMoreRanges(const std::string& model);
bool valid_quote(const QuoteInfo& quote, const double medMedSprd = 0., const double minSpreadMMS = 0., const double maxSpreadMMS = 0.);
//bool valid_trade(const TradeInfo& trade);
bool valid_trade(const TradeInfo& trade, const QuoteInfo& quote);
void get_mid_series(std::vector<double>& vMid1s, const std::vector<QuoteInfo>* quotes,
	int openMsecs, int closeMsecs, int lagMsecs = 0, bool fill = true, bool remove_cross = false);
void get_mid_series_smooth(std::vector<double>& vMid1s, const std::vector<QuoteInfo>* quotes,
	int openMsecs, int closeMsecs, int lagMsecs = 0, bool fill = true, bool remove_cross = false);
std::unordered_map<std::string, float> getClose(const std::string& model, int idate);
std::unordered_map<std::string, float> getVolatNorm(const std::string& model, int idate);
std::unordered_map<std::string, float> getLogVol(const std::string& model, int idate);
std::unordered_map<std::string, float> getVolDepThres(const std::string& model, int idate);
std::unordered_set<std::string> getTradedTickers(const std::string& model, int idate);
std::vector<int> getSampleMsecs(int openMsecs, int closeMsecs, int stepSec, int seed = 0);
std::vector<int> getSampleMsecsIrreg(const std::string& ticker, int idate, int openMsecs, int closeMsecs, int stepSec);
std::vector<int> getSampleMsecs(const MercatorVolume& mercVol, const std::string& ticker, int openMsecs, int closeMsecs, int stepSec);
std::vector<int> getSampleMsecsVolat(int openMsecs, int closeMsecs, int stepSec, float medVolat,
		const std::vector<TradeInfo>* trades, Sessions& sessions);

} // namespace gtlib

#endif
