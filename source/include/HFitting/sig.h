#ifndef __sig__
#define __sig__
#include <vector>
#include <HLib.h>

namespace sig {
	struct HedgeInfo {
		HedgeInfo(){}
		HedgeInfo(std::string path, int gpts);
		bool valid();

		std::vector<std::vector<float> > vMean;
		std::vector<float> mean60Intra;
		std::vector<float> meanClose;
		std::vector<float> meanNextClose;
	};

	std::string get_hedge_path(std::string model, int idate);

	bool valid_quote(const QuoteInfo& quote);

	bool read_chara_data(hff::SigC& sig, std::string market, std::string uid, int idate, int idate_p, int idate_pp, int idate_n);
	void get_trade_stateinfo(hff::SigC& sig, const std::vector<TradeInfo>* trades);
	void get_quote_stateinfo(hff::SigC& sig, const std::vector<QuoteInfo>* quotes);
	void get_filImb_stateinfo(hff::SigC& sig, const std::vector<TradeInfo>* trades, const std::vector<QuoteInfo>* quotes);
	void get_tob_stateinfo(TickTobAcc* tta, hff::SigC& sig, int idate, std::string ticker);
	void get_book_stateinfo(std::map<std::string, OrderBkAcc<OrderData> >& obaMap, hff::SigC& sig,
		std::string market, int idate, std::string ticker, std::vector<std::string> okECNs);
	void get_targets_stateinfo(hff::SigC& sig, const std::vector<QuoteInfo>* quotes);
	void get_signals(hff::SigC& sig);
	void get_MI_signals(hff::SigC& sig, const std::vector<hff::OrderQty>* trades);

	void calcBookDepthSigsMO(int nBidLevels, const OrderData** bidSide, int nAskLevels, const OrderData** askSide,
		double midPrice, double spreadOff, double dailyVolume, unsigned nSigs, const double* volumeFrac, std::vector<double>& signals);
	void calcBookDepthSigsQI(int nBidLevels, const OrderData** bidSide, int nAskLevels, const OrderData** askSide,
		double midPrice, double medSprd, unsigned nSigs, const double* bins, std::vector<double>& signals, std::vector<double>& vsignals, double dailyVolume);
	void get_mid_series(std::vector<double>& vMid, const std::vector<QuoteInfo>* quotes, int lagMsecs = 0);
	void get_quote_index_1s(std::vector<int>& vQindx1s, const std::vector<QuoteInfo>* quotes);
	double get_mret(int sec, int length, int firstQtSec, const std::vector<QuoteInfo>* quotes, const std::vector<int>& vQindx1s, double mqut, double firstMidQt);
	double get_voluMom( int sec, int length, const std::vector<double>& trdVolu1s );
	double get_volQty( int sec, int length, const std::vector<int>& tradeVol );
	double get_vwap( int sec, int length, const std::vector<double>& sumVolTrdPrc1s, const std::vector<double>& trdVolu1s,
		const std::vector<double>& sumVolTrdPrc1sAcc, const std::vector<double>& trdVolu1sAcc, double minVolu = 0 );
	void get_acc(std::vector<double>& vAcc, const std::vector<double>& v);
};

#endif
 
