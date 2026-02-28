#ifndef __sigFtns__
#define __sigFtns__
#include <gtlib_model/hff.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <unordered_map>

namespace gtlib {

std::unordered_map<std::string, std::string> get_uid_map(const std::vector<std::string>& markets, int idate, const std::set<std::string>& uids);
bool read_chara_data(const hff::CharaContainer* ch, hff::SigC& sig, const std::string& model,
		const std::string& market, const std::string& uid, float on_target_clip, bool removeHardToBorrow, bool requireTickValid,
		int idate, int idate_p, int idate_p2, int idate_p3, int idate_p4, int idate_p5,
		int idate_p6, int idate_p7, int idate_p8, int idate_n);
float get_shareout(hff::CharaInfo& chara_p, hff::CharaInfo& chara_p2, hff::CharaInfo& chara_p3);
bool calculate_chara_p(hff::SigC& sig, hff::CharaInfo& chara, hff::CharaInfo& chara_p, float shareout,
		const std::string& model02, const std::string& market, bool removeHardToBorrow, bool requireTickValid);
void calculate_chara_p2(hff::SigC& sig, hff::CharaInfo& chara, hff::CharaInfo& chara_p, hff::CharaInfo& chara_p2, float shareout);
void calculate_chara_p3(hff::SigC& sig, hff::CharaInfo& chara, hff::CharaInfo& chara_p2, hff::CharaInfo& chara_p3);
void calculate_chara_p4(hff::SigC& sig, hff::CharaInfo& chara, hff::CharaInfo& chara_p3, hff::CharaInfo& chara_p4);
void calculate_chara_p5(hff::SigC& sig, hff::CharaInfo& chara, hff::CharaInfo& chara_p4, hff::CharaInfo& chara_p5);
void calculate_chara_p6(hff::SigC& sig, hff::CharaInfo& chara, hff::CharaInfo& chara_p5, hff::CharaInfo& chara_p6);
void calculate_chara_p7(hff::SigC& sig, hff::CharaInfo& chara, hff::CharaInfo& chara_p6, hff::CharaInfo& chara_p7);
void calculate_chara_p8(hff::SigC& sig, hff::CharaInfo& chara, hff::CharaInfo& chara_p7, hff::CharaInfo& chara_p8);
void calculate_chara_n(hff::SigC& sig, hff::CharaInfo& chara, hff::CharaInfo& chara_n, float on_target_clip);
float getMretIntraLag(hff::CharaInfo& chara);
float getMretONLag(hff::CharaInfo& chara_p, hff::CharaInfo& chara_p2);
float getHiloQAI(hff::CharaInfo& chara_p);
float getHiloLagOpen(hff::CharaInfo& chara_p);
float getHiloLagRat(hff::CharaInfo& chara_p);

void get_cum(std::vector<int>& vCum, const std::vector<int>& v);
double get_voluMom(int openMsecs, int msecs, int length, const std::vector<TradeInfo>& trades, float avgVolu, float n1sec);
double get_voluMomPartial(int openMsecs, int msecs, int length, const std::vector<TradeInfo>& trades);
double get_voluMom(int openMsecs, int msecs, int length, const std::vector<TradeInfo>& trades);
double get_vwap_sig(int msecs, int length, const std::vector<TradeInfo>& trades, double minVolu);
double get_logBlockVol(int msecs, int lengh, const std::map<int, float>& m);
double get_volQty( int sec, int length, const std::vector<int>& tradeVolCum );

int get_max_bidSize(int msecs, int length, const std::vector<QuoteInfo>& quotes);
int get_max_askSize(int msecs, int length, const std::vector<QuoteInfo>& quotes);
int get_max_bidSize(int msecs, int length, const std::vector<QuoteDataMicro>& quotes);
int get_max_askSize(int msecs, int length, const std::vector<QuoteDataMicro>& quotes);
bool get_persistent(const QuoteInfo& qFuture, hff::StateInfo& sI);

void calcBookDepthSigsMO(int nBidLevels, const OrderData** bidSide, int nAskLevels, const OrderData** askSide,
		double midPrice, double spreadOff, double dailyVolume, unsigned nSigs, const double* volumeFrac, std::vector<double>& signals);

void calcBookDepthSigsQI(int nBidLevels, const OrderData** bidSide, int nAskLevels, const OrderData** askSide,
		double midPrice, double medSprd, unsigned nSigs, const double* bins,
		std::vector<double>& signals, std::vector<double>& vsignals, double dailyVolume);

void calcBookDepthSigsMOMicro(int nBidLevels, std::vector<const std::pair<const int, PriceLevelData>*>& bidSide, int nAskLevels, std::vector<const std::pair<const int, PriceLevelData>*>& askSide,
		double midPrice, double spreadOff, double dailyVolume, unsigned nSigs, const double* volumeFrac, std::vector<double>& signals);

void calcBookDepthSigsQIMicro(int nBidLevels, std::vector<const std::pair<const int, PriceLevelData>*>& bidSide, int nAskLevels, std::vector<const std::pair<const int, PriceLevelData>*>& askSide,
		double midPrice, double medSprd, unsigned nSigs, const double* bins,
		std::vector<double>& signals, std::vector<double>& vsignals, double dailyVolume);

}; // namespace gtlib

#endif

