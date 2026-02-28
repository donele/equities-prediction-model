#ifndef __hff__
#define __hff__
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <unordered_map>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

namespace hff {
// const variables are used in hff.cpp to allocate the memory, or in sig.cpp and MMakeSample.cpp.
// Also used in MMakeHedge.cpp and MMakePredIndex.cpp.
const int sec_on_ = 900;
const int om_num_basic_sig_ = 16;
const int om_num_sig_ = 64;
const int om_num_err_sig_ = 16;
const int tm_num_basic_sig_ = 12;
const int tm_num_sig_ = 16;
const int max_book_sigs_ = 32;

const float typ_med_sprd_ = 0.0015;
const float min_med_sprd_ = 0.00008;
const float max_med_sprd_ = 0.02;
const float max_ret_ = 100000.;
const int min_quotes_ = 20;
const int min_trades_ = 5;
const float om_hilo_tol_ = 0.0001;
const float tm_volu_mom_lb_1_ = 600;
const float tm_volu_mom_lb_2_ = 3600;
const float min_hiloD_ = 5.;
const int max_qtmax_lag_ = 200;
const int max_qtmax_lag2_ = 1200;
const float min_price_ = 0.01;
const float skip_qt_ = 500.;
const float om_max_ret_ = 500.;
const float om_max_tar_ret_ = 10000.;
const int om_priorqutime_ = 0;
const int om_stale_trd_ = 60;
const int om_target_offset_ = 1;
const int tm_target_offset_ = 61;
const int tm_target_60_ = 3600;
const float om_fill_imb_clip_ = 5.;
const float sec_ret_clip_ = 100.; // used for hedging.
const int max_book_levels_ = 3;
const float min_tob_nbbo_dif_ = 0.005;
const float minSprdOff_ = 0.0001;
const int max_qtwt_lag_ = 100000;
const int om_stale_qut_ = 100000;
const float multi_lin_sprd_clip_ = 50;
const int skip_first_secs_ = 120;
const int om_tree_min_fit_sprd_ = 0;
const int om_tree_max_fit_sprd_ = 200;
const int maxNewsSigs_ = 10;
const double min_tick_default_ = 0.00005;
const int var_fit_days_ = 20;
const int cb_ref_nevt_ = -101;
const int cb_ref_eevt_ = -102;

const int regular_sample_ = 1 << 1;
const int nbbo_event_ = 1 << 2;
const int trade_event_ = 1 << 3;
const int book_trade_event_ = 1 << 4;
const int time_event_ = 1 << 5;
const int news_event_ = 1 << 6;
const int exploratory_event_ = 1 << 7;
const int trade_and_exploratory_event_ = 1 << 8;

struct CLASS_DECLSPEC IndexFilter {
	IndexFilter(const std::string& model, const std::vector<std::string>& names_, int horizon_, int targetLag_, int length_, double clip_index, double clip_fut_index);
	std::string title() const;

	std::string myTitle;
	std::vector<std::string> names;
	int fitDays;
	int length;
	int horizon;
	int targetLag;
	int skip;
	double clip_index_;
	double clip_fut_index_;
	std::vector<int> clip;
	int filtClip(const std::string& name);
};

struct CLASS_DECLSPEC IndexFilters {
	IndexFilters(){}
	IndexFilters(const std::string& model_, double clip_index, double clip_fut_index, bool use_etf_filter = false);
	void addHorizon(int horiz, int lag = 0);

	std::string model;
	bool use_etf_filter;
	std::vector<IndexFilter> filters;
	double clip_index;
	double clip_fut_index;
};

FUNC_DECLSPEC std::vector<std::string> markets(const std::string& model);
FUNC_DECLSPEC int minDataOK(const std::string& model);

FUNC_DECLSPEC int filtClip(const std::string& name);

struct CLASS_DECLSPEC LinearModel {
	LinearModel(){}
	LinearModel(const std::string& model);
	void addHorizon(int horizon, int lag);
	void set_idate(int idate);
	std::vector<std::string> get_ecns(int idate) const;
	int minPts(const std::string& model) const;
	int minPtsTm(const std::string& model) const;
	void setUseMoreCAEcns();

	bool medFromHfuniv;
	bool nfFromHfuniv;
	bool inUnivFromHfuniv;
	bool partialVM;
	bool useMoreCAEcns;
	bool requirePersistChina;
	bool excludeBlockTrade;
	int allowNegSizeTo;
	std::string model;
	std::string model02;
	std::string market;
	std::string mCode;
	bool useShortMrets;
	bool writeCompTicker;
	bool allowCross;
	int nHorizon;
	int sampleDelay;
	std::vector<std::string> markets;
	std::vector<std::pair<int, int> > vHorizonLag;
	bool use_psx;
	std::vector<int> useom;
	std::vector<int> useomErr;
	mutable int openMsecs;
	mutable int closeMsecs;
	mutable int n1sec;
	int exploratoryDelay;
	int minSpreadMMS;
	int maxSpreadMMS;
	int gridInterval;
	mutable int gpts;
	int nSlices;
	int nSig;
	int nErrSig;
	int nTreesOmFit;
	int nTreesOmProd;
	int minNrowsParam;
	int transientMsecs;
	int om_bin_sample_freq;
	int tm_bin_sample_freq;
	int num_time_slices;
	bool om_use_tm_input;
	bool use_pred_index;
	bool use_etf_filter;
	bool use_us_arca_trade_event;
	bool halt_tickers;
	bool primary_only;
	int om_univ_fit_days;
	int om_err_fit_days;
	float clip_pred_index;
	float clip_fut_index;
	float clip_index;
	float clip_ret60;
	float clip_ret300;
	float on_target_clip;// = 800; // target longer than to close.
	float om_target_clip;// = 200;
	float tm_target_clip;// = 600.;
	float tm_target_60_clip;// = 1000.;
	float tm_target_intra_clip;// = 1000000.;
	float scaleTradable;
	float decayTradable;
private:
};

struct CLASS_DECLSPEC NonLinearModel {
	NonLinearModel(){}
	NonLinearModel(const std::string& model, const LinearModel& linMod);
	void addHorizon(int horizon, int lag);

	int nHorizon;
	int maxHorizon;
	int nTreesTmFit;
	int nTreesTmProd;
	int nTrees71ClFit;
	int nTrees71ClProd;
	int nTreesClclFit;
	int nTreesClclProd;
	int nTreesOpclFit;
	int nTreesOpclProd;
	std::vector<std::pair<int, int> > vHorizonLag;
};

struct CLASS_DECLSPEC CharaInfo {
	CharaInfo():medVolume(0.), medVolatility(0.), medmedSprd(0.), medNquotes(0.), medNtrades(0.),
	nffundrst(0.), nffundtrd(0.), nffundbp(0.),
	volatility(0.), close(0.), open(0.), high(0.), low(0.),
	nTrades(0), nQuotes(0), volume(0), adjust(0.), tickValid(0), inUniv(0), positiveMaxPos(0), shareout(0.){}
	float medVolume;
	float medVolatility;
	float medmedSprd;
	float medNquotes;
	float medNtrades;
	float nffundrst;
	float nffundtrd;
	float nffundbp;
	float volatility;
	float close;
	float open;
	float high;
	float low;
	int nTrades;
	int nQuotes;
	int volume;
	float adjust;
	int tickValid;
	int inUniv;
	int positiveMaxPos;
	float shareout;
	std::string ticker;
	std::string market;
	std::string sectype;
};

struct CLASS_DECLSPEC CharaContainer {
	std::unordered_map<std::string, std::unordered_map<int, std::unordered_map<std::string, CharaInfo> > > mMktIdtTkr;
	bool getCharaInfo(const std::string& market, int idate, const std::string& uid, CharaInfo& chara) const;
	void deleteOlder(const std::string& market, int idate);
	bool exists(const std::string& market, int idate);
	std::unordered_map<std::string, hff::CharaInfo>& createAndReturn(const std::string& market, int idate);
};

struct CLASS_DECLSPEC StateInfo {
	StateInfo(){}
	StateInfo(const LinearModel& linMod, const NonLinearModel& nonLinMod);
	void init(const LinearModel& linMod, const NonLinearModel& nonLinMod);
	void init_vars();
	void init_arrays(const LinearModel& linMod, const NonLinearModel& nonLinMod);

	int msso; // msecs from open 
	float mstc; // msecs to close
	float timeFrac;

	float mqut;
	float absSprd;

	float sprd;
	float costBid;
	float costAsk;
	float bid;
	float ask;
	float bsize;   // 20070606  changed int to float for europe
	float asize;
	int bidEx;
	int askEx;
	int maxbsize;	// max in the interval t - MAX_QTMAX_LAG to t
	int maxasize;
	int maxbsize2;	// max in the interval t - MAX_QTMAX_LAG2 to t
	int maxasize2;

	float mretp5;
	float mret1;
	float mret5;
	float mret15;
	float mret30;
	float mret60;
	float mret120;
	float mret300;
	float mret600;
	float mret1200;
	float mret2400;
	float mret4800;

	float mret300L;
	float mret600L;
	float mret1200L;
	float mret2400L;
	float mret4800L;

	//float selfInt60L60;
	//float selfInt60L120;
	//float selfInt60L300;
	//float selfInt60L600;
	//float selfInt60L1200;
	//float selfInt300L300;
	//float selfInt300L600;
	//float selfInt300L1200;
	//float selfInt300L2400;

	//float cwret15;
	float cwret30;
	float cwret60;
	float cwret120;
	float cwret300;
	float cwret600;
	float cwret300L;
	float cwretONLag0;

	//float vwret15;
	float swret30;
	float swret60;
	float swret120;
	float swret300;
	float swret600;
	float swret300L;

	//float voluMom5;
	float voluMom15;
	float voluMom30;
	float voluMom60;
	float voluMom120;
	float voluMom300;
	float voluMom600;
	float voluMom3600;
	float mretOpen; // mretOpenL; // "open" to T - 1200

	// new 20070808
	float quimMax2;
	float intraVoluMom;
	float relativeHiLo;
	//float midCwap;
	//float midVwap;
	//float midCwap600;
	//float midVwap600;

	// new on 20080227
	float mI600, mI3600, mIIntra;
	//float mI5, mI15, mI30, mI60, mI120, mI300;
	float ltrade;
	//float qIm5;

	float hiloLag1;
	float hiloLag2;
	float hilo900;
	float bollinger300;
	float bollinger900;
	float bollinger1800;
	float bollinger3600;
	float logBlockVolUsd60;
	float logBlockVolUsd3600;
	float logBlockVolUsdIntra;

	//float sdTrd;
	//float sdTrdNorm;
	float volSclBsz;
	float volSclAsz;
	float rtrd;
	float rPastTrd;
	float rCurTrd;
	float rtrd60;
	float rtrd120;
	float rtrd3600;
	float rtrd7200;
	float rtrdOverSprd;
	float mrtrd;
	float mrtrd60;
	float mrtrd120;
	float mrtrd3600;
	float mrtrd7200;
	float dnmk;
	float snmk;

	float moP;
	float moA;
	float bidOffA;
	float askOffA;
	float quimP;
	float quimA;

	int tsinceq;    // now in milliseconds 
	int tsincet;
	float isExp;	// if it is an exploratory event.
	float tsincen;
	int tlastq;
	int tlastt;

	std::vector<float> sig1;
	std::vector<float> sig10;
	std::vector<float> sigBook;
	//float fIm1;
	//float fIm2;
	//float fillImbTm10;
	//float fillImbTm5;
	//float fillImbT;
	//float fillImbTp1;
	float eventTakeRatMkt;
	float eventTakeRatTot;
	float indxPredWS1;
	float indxPredWS2;
	float indxPred1Adj;
	float indxPred2Adj;
	float indxPred1Sprd;
	float indxPred2Sprd;
	//float news;
	//float newsSenti;
	//float newsVol;
	//float dswmdsTop;
	//float dswmds;
	//float dswmdp;

	float bidDepth900;
	float askDepth900;
	float bestPostTrade;
	//float sipDiffMid;
	//float sipDiffSide;

	bool isTmSample;
	short validq;
	short validt;
	short valid; // summary of various valid flags; = 1 if validq+validt + dayOK + tobdayOK + inUniv = 5
	short validTm;
	short tobOK;
	short sampleType;
	short askPersists; // default is 1.
	short bidPersists;
	//float persist1;
	//float persist2;
	//float persist5;
	//float persist10;
	//float persist20;
	//float persist50;
	//float persist100;
	//float persist200;
	//float persist500;
	//float persist1000;


	std::vector<float> target;
	std::vector<float> targetUH;
	std::vector<float> targetXsprd;
	std::vector<float> targetXsprdUH;
	float target11Close;
	float target71Close;
	float targetClose;
	float targetNextClose;
	float target11CloseUH;
	float target71CloseUH;
	float targetCloseUH;
	float targetNextCloseUH;

	float predOm;
	float predTm;
	float predOm30s;
	float predOm60s;
	float predOm90s;
	float predOm120s;
	float predOm150s;
	float predOm180s;
	float predOm210s;
	float predOm300s;
	float predOm600s;
	float predTm30s;
	float predTm60s;
	float predTm90s;
	float predTm120s;
	float predTm150s;
	float predTm180s;
	float predTm210s;
	float predTm300s;
	float predTm600s;
	float predTm1200s;
	float predTm2400s;
	float predTm4800s;
	float mret30predOm;
	float mret60predOm;
	float mret120predOm;
	float mret300predOm;
	float mret600predOm;
	float mret120predTm;
	float mret300predTm;
	float mret600predTm;
	float mret1200predTm;
	float mret2400predTm;
	float mret4800predTm;
	float mret120predTm1;
	float mret300predTm1;
	float mret600predTm1;
	float mret1200predTm1;
	float mret2400predTm1;
	float mret4800predTm1;
	float predOmIt1;
	float predOm30sIt1;
	float predOm60sIt1;
	float predOm90sIt1;
	float predOm120sIt1;
	float predOm150sIt1;
	float predOm180sIt1;
	float predOm210sIt1;
	float predOm300sIt1;
	float predOm600sIt1;
	float mret30predOmIt1;
	float mret60predOmIt1;
	float mret120predOmIt1;
	float mret300predOmIt1;
	float mret600predOmIt1;

	float pr1;
	float pr1err;
	float bmpred;
	std::vector<float> pred;
	std::vector<float> predIndex;
	std::vector<float> predErr;
	std::vector<float> predVar;
	std::vector<float> sigIndex1;
	std::vector<float> sigIndex2;
	std::vector<float> sigIndex3;
	std::vector<float> sigIndex4;
	std::vector<float> sigIndex5;
	std::vector<float> sigIndex6;

	int gptOK;
	std::vector<float> om;
	std::vector<float> omErr;
	std::vector<float> targetErr;
	std::vector<float> tm;

	bool persistentChina;
	float relVolat;
	float relSprd;
	float ltrdImb;
	float ltrdPrc;
};

struct CLASS_DECLSPEC SigC {
	SigC();
	SigC(const LinearModel& linMod, const NonLinearModel& nonLinMod);
	~SigC();
	void init(const LinearModel& linMod, const NonLinearModel& nonLinMod);
	void init(const LinearModel& linMod, const NonLinearModel& nonLinMod, int n);
	void init(const LinearModel& linMod, const NonLinearModel& nonLinMod, const std::vector<int>& vMsecs);
	void init_vars();
	void init_si(const LinearModel& linMod, const NonLinearModel& nonLinMod);
	void init_si(const LinearModel& linMod, const NonLinearModel& nonLinMod, int n);
	void init_si(const LinearModel& linMod, const NonLinearModel& nonLinMod, const std::vector<int>& vMsecs);

	std::string ticker;
	float logCap;
	float logShr;
	float avgDlyVolume;
	float avgDlyVolat;
	float adjPrevClose;
	float close;
	float medSprd;
	float medNquotes;
	float medNtrades;
	float medNbooktrades;
	float prevDayVolume;
	float prevDayVolat;
	float mretIntraLag1;
	float mretIntraLag2;
	float mretIntraLag3;
	float mretIntraLag4;
	float mretIntraLag5;
	float mretIntraLag6;
	float mretIntraLag7;
	float mretIntraLag8;
	float mretIntraLag3Sprd;
	float mretIntraLag4Sprd;
	float mretIntraLag5Sprd;
	float hiloQAI;
	float hiloQAI2;
	float hiloQAI3;
	float hiloQAI4;
	float hiloQAI5;
	float hiloQAI6;
	float hiloQAI7;
	float hiloQAI8;
	float hiloQAI3Sprd;
	float hiloQAI4Sprd;
	float hiloQAI5Sprd;
	float hiloLag1Open;
	float hiloLag2Open;
	float hiloLag3Open;
	float hiloLag4Open;
	float hiloLag5Open;
	float hiloLag6Open;
	float hiloLag7Open;
	float hiloLag8Open;
	float hiloLag3OpenSprd;
	float hiloLag4OpenSprd;
	float hiloLag5OpenSprd;
	float hiloLag1Rat;
	float hiloLag2Rat;
	float hiloLag3Rat;
	float hiloLag4Rat;
	float hiloLag5Rat;
	float hiloLag6Rat;
	float hiloLag7Rat;
	float hiloLag8Rat;
	float hiloLag3RatSprd;
	float hiloLag4RatSprd;
	float hiloLag5RatSprd;
	float mretONLag0;
	float mretONLag1;
	float mretONLag2;
	float mretONLag3;
	float mretONLag4;
	float mretONLag5;
	float mretONLag6;
	float mretONLag7;
	float cwretONLag1;
	float cwretIntraLag1;
	float cwretIntraLag2;
	float mretONLag2Sprd;
	float mretONLag3Sprd;
	float mretONLag4Sprd;
	float hiLag1;
	float loLag1;
	float hiLag2;
	float loLag2;
	float closeNextCloseRet;
	int inUnivChara;
	int inUnivFit;
	float firstMidQt;
	int firstQtGpt;
	float sprdOpen;
	float fxRate;
	float logVolu;
	float logPrice;
	float logPriceUsd;
	float logMedSprd;
	float northRST;
	float northTRD;
	float northBP;
	float isSecTypeF;
	float tarMax;
	float tarMaxuh;
	float tarON;
	float tarONuh;
	float tarClcl;
	float tarClcluh;
	float exchVol;
	std::string exchangeChar;
	std::string sectype;
	float exchange;
	std::vector<StateInfo> sI;
};

struct CLASS_DECLSPEC OrderQty {
	OrderQty():msecs(0), signedQty(0){}
	OrderQty(int m, int q):msecs(m), signedQty(q){}
	int msecs;
	int signedQty;
};

class CLASS_DECLSPEC StateInfoH {
public:
	StateInfoH():gptOK(0),target60Intra(0.),target11Close(0.),target71Close(0.),targetClose(0.),targetNextClose(0.){}
	StateInfoH(const StateInfo& sI);
	int gptOK;
	std::vector<float> target;
	float target60Intra;
	float target11Close;
	float target71Close;
	float targetClose;
	float targetNextClose;
};

class CLASS_DECLSPEC SigH {
public:
	SigH():inUnivFit(0),northBP(0.),northTRD(0.),northRST(0.),tarON(0.),tarClcl(0.){}
	SigH(const std::string& ticker, int inUnivFit, int n1sec, int nTargets = 1);
	SigH(const SigC& sig, const std::string& ticker);
	float get_hedged_target(int sec, int len, int lag, bool debug = false) const;
	friend std::ostream& operator <<(std::ostream& os, const SigH& lhs);
	std::string ticker;
	int inUnivFit;
	float northBP;
	float northTRD;
	float northRST;
	float tarON;
	float tarClcl;
	std::vector<StateInfoH> sIH;
};

struct CLASS_DECLSPEC SigHLess: public std::binary_function<SigH, SigH, bool> {
	bool operator()(const SigH& lhs, const SigH& rhs) const;
};

struct CLASS_DECLSPEC SignalLabel {
	SignalLabel():nrows(0), ncols(0){}
	SignalLabel(int nrows_, int ncols_, const std::vector<std::string>& l):nrows(nrows_), ncols(ncols_), labels(l){}
	void print(std::ostream& os);

	int nrows;
	int ncols;
	std::vector<std::string> labels;
};

struct CLASS_DECLSPEC SignalContent {
	SignalContent(int ncols_):ncols(ncols_){}
	SignalContent(std::vector<float> v_):v(v_), ncols(v_.size()){}
	void print(std::ostream& os);

	int ncols;
	std::vector<float> v;
};

struct CLASS_DECLSPEC Prediction {
	Prediction(int ncols_):ncols(ncols_){}
	int ncols;
	std::vector<float> v;
};

} // namespace hff

FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const hff::SignalLabel& obj);
FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const hff::SignalContent& obj);
FUNC_DECLSPEC std::istream& operator >>(std::istream& is, hff::SignalLabel& obj);
FUNC_DECLSPEC std::istream& operator >>(std::istream& is, hff::SignalContent& obj);
FUNC_DECLSPEC std::istream& operator >>(std::istream& is, hff::Prediction& obj);

#endif
