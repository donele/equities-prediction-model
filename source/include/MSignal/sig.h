#ifndef __sig__
#define __sig__
#include <jl_lib/Sessions.h>
#include <vector>
#include <unordered_map>
#include <MFramework.h>

namespace sig {
std::unordered_map<std::string, std::string> get_uid_map(const std::vector<std::string>& markets, int idate, const std::set<std::string>& uids);
bool read_chara_data(hff::SigC& sig, const std::string& model, const std::string& market, const std::string& uid,
		int idate, int idate_p, int idate_pp, int idate_n);
bool read_chara_weight(hff::SigC& sig, const std::string& model, const std::string& market, const std::string& uid, int idate_p);
void get_trade_stateinfo(hff::SigC& sig, const std::vector<TradeInfo>* trades);
void get_quote_stateinfo(hff::SigC& sig, const std::vector<QuoteInfo>* quotes, bool debug_sprd);
void get_filImb_stateinfo(hff::SigC& sig, const std::vector<TradeInfo>* trades, const std::vector<QuoteInfo>* quotes);
void get_tob_stateinfo(TickTobAcc* tta, hff::SigC& sig, int idate, const std::string& ticker);
void get_book_stateinfo(std::map<std::string, OrderBkAcc<OrderData> >& obaMap, hff::SigC& sig,
		const std::string& market, int idate, const std::string& ticker, const std::vector<std::string>& okECNs);
void get_targets_stateinfo(hff::SigC& sig, const std::vector<QuoteInfo>* quotes);
void get_signals(hff::SigC& sig, Sessions& sessions);
void get_MI_signals(hff::SigC& sig, const std::vector<hff::OrderQty>* trades);
void get_news_signals(hff::SigC& sig, const std::string& ticker);

void calcBookDepthSigsMO(int nBidLevels, const OrderData** bidSide, int nAskLevels, const OrderData** askSide,
		double midPrice, double spreadOff, double dailyVolume, unsigned nSigs, const double* volumeFrac, std::vector<double>& signals);
void calcBookDepthSigsQI(int nBidLevels, const OrderData** bidSide, int nAskLevels, const OrderData** askSide,
		double midPrice, double medSprd, unsigned nSigs, const double* bins,
		std::vector<double>& signals, std::vector<double>& vsignals, double dailyVolume);
bool get_persistent(const QuoteInfo& qFuture, hff::StateInfo& sI);
bool get_persistent(int sec, int dsec, const std::vector<QuoteInfo>* quotes, const std::vector<int>& vQindx1s, hff::StateInfo& sI);
double get_mret(int sec, int length, int firstQtSec, const std::vector<QuoteInfo>* quotes,
		const std::vector<int>& vQindx1s, double mqut, double firstMidQt, double medSprd);
double get_voluMom( int sec, int length, const std::vector<double>& trdVolu1sCum );
double get_volQty( int sec, int length, const std::vector<int>& tradeVolCum );
double get_vwap_sig( int sec, int length, const std::vector<double>& sumVolTrdPrc1sCum,
		const std::vector<double>& trdVolu1sCum, double minVolu = 0 );
double get_vwap( int sec, int length, const std::vector<double>& sumVolTrdPrc1sCum,
		const std::vector<double>& trdVolu1sCum, double minVolu = 0 );
void get_cum(std::vector<double>& vCum, const std::vector<double>& v);
void get_cum(std::vector<int>& vCum, const std::vector<int>& v);
};

#endif
