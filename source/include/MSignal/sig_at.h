#ifndef __sig_at__
#define __sig_at__
#include <vector>
#include <MFramework.h>
#include <jl_lib/jlutil.h>

namespace sig {
void get_trade_stateinfo_at(hff::SigC& sig, const std::vector<TradeInfo>* trades, const std::vector<int>& vTindx1s);
void get_quote_stateinfo_at(hff::SigC& sig, const std::vector<QuoteInfo>* quotes, const std::vector<int>& vQindx1s);
void get_filImb_stateinfo_at(hff::SigC& sig, const std::vector<TradeInfo>* trades, const std::vector<QuoteInfo>* quotes,
		const std::vector<int>& vTindx1s, const std::vector<int>& vQindx1s);
void get_tob_stateinfo_at(TickTobAcc* tta, hff::SigC& sig, int idate, const std::string& ticker);
void get_book_stateinfo_at(std::map<std::string, OrderBkAcc<OrderData> >& obaMap, hff::SigC& sig,
		const std::string& market, int idate, const std::string& ticker, const std::vector<std::string>& okECNs);
void get_targets_stateinfo_at(hff::SigC& sig, const std::vector<QuoteInfo>* quotes);
void get_signals_at(hff::SigC& sig, Sessions& sessions);
void get_MI_signals_at(hff::SigC& sig, const std::vector<hff::OrderQty>* trades);

double get_mret_exact(int msecs, int openMsecs, int length_msec, int firstQtMsec, double firstMidQt,
		const std::vector<QuoteInfo>* quotes, const std::vector<int>& vQindx1s, double mqut, int lag, bool backCompat);
double get_dret(int msecs, int openMsecs, int firstQtMsec,
		const std::vector<QuoteInfo>* quotes, const std::vector<int>& vQindx1s,
		const std::vector<QuoteInfo>* sip, const std::vector<int>& vSindx1s);
double get_mret_1s(int msecs, int openMsecs, int length_msec, int firstQtMsec, double firstMidQt,
		const std::vector<double>& vMid1s, double mqut, int lag = 0);
double get_voluMom( int msecs, int length, const std::vector<TradeInfo>* trades, int idxStart );
double get_voluMom( int msecs, int openMsecs, int length,
		const std::vector<double>& trdVolu1sCum, const std::vector<TradeInfo>* trades, const std::vector<int>& vTindx1s );
double get_voluMom(int msecs, int length, const std::vector<TradeInfo>& trades);
double get_vwap_sig( int msecs, int length, const std::vector<TradeInfo>* trades, int idxStart, double minVolu );
double get_vwap_sig( int msecs, int openMsecs, int length,
		const std::vector<double>& sumTrdPrc1sCum, const std::vector<double>& nTrds1sCum,
		const std::vector<TradeInfo>* trades, const std::vector<int>& vTindx1s, double minVolu );
double get_vwap_sig(int msecs, int length, const std::vector<TradeInfo>& trades, double minVolu);
int get_max_bidSize(int msecs, int openMsecs, const std::vector<int>& vBidSize1s,
		int lag, const std::vector<QuoteInfo>* quotes, const std::vector<int>& vQindx1s);
int get_max_askSize(int msecs, int openMsecs, const std::vector<int>& vAskSize1s,
		int lag, const std::vector<QuoteInfo>* quotes, const std::vector<int>& vQindx1s);
int get_max_bidSize(int msecs, int length, const std::vector<QuoteInfo>& quotes);
int get_max_askSize(int msecs, int length, const std::vector<QuoteInfo>& quotes);
};

#endif
