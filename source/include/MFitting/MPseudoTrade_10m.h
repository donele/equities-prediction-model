#ifndef __MPseudoTrade_10m__
#define __MPseudoTrade_10m__
#include <MFramework.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <MFitting/MPseudoTradePrep.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MPseudoTrade_10m: public MModuleBase {
public:
	MPseudoTrade_10m(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MPseudoTrade_10m();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	struct DayPnl {
		DayPnl(){}
		DayPnl(float ti, float tcc, float tco, int nb, int ns, float tdv);
		float totIntra;
		float totClcl;
		float totClop;
		int nbuy;
		int nsell;
		float totDV;
	};
	struct Prediction {
		Prediction(const std::string& uid, int time, float pred, float mid, float sprd);
		std::string uid;
		int time;
		float pred;
		float mid;
		float sprd;
	};
	struct PredictionSet {
		PredictionSet();
		PredictionSet(const std::string& model, Prediction& p1, Prediction& p10, Prediction& p40);
		std::string uid;
		int time;
		int bs;
		float totPred;
		float pred1m;
		float pred10m;
		float pred40m;
		float mid;
		float sprd;
		float cost;
	};

	class TradeSim {
	public:
		TradeSim(){}
		~TradeSim(){}
		TradeSim(std::vector<std::string>& targetNames, const std::string& baseDir,
				std::string& model, std::string& sigModel, int udate, int idate,
				double maxPosTicker, const std::vector<int>& ntrees, float thres,
				int iPolicy, float wgtSecondPred, int cutoffTime, float capLevel, bool debug=false);
		int getIndxPred(const std::string& line, int ntrees);
		void select_trades();
		DayPnl calculate_pnl();
		void clear_buffer();
		bool getNextTrade(PredictionSet& p);
		bool isTradable(PredictionSet& p);
		int get_bs(float pred, float cost);
		Prediction getPred(int indxPred, const std::string& lineTxt, const std::string& linePred);
		bool isNewTicker(const PredictionSet& p);
		void flush_buffer();
		void addData(PredictionSet& p);
	private:
		bool debug_;
		int iPolicy_;
		int cutoffTime_;
		float wgtSecondPred_;
		float capLevel_;
		float thres_;
		double maxPosTicker_;
		std::vector<int> vIndxPred_;
		std::string model_;
		std::string txt1mPath_;
		std::string txt10mPath_;
		std::string txt40mPath_;
		std::string pred1mPath_;
		std::string pred10mPath_;
		std::string pred40mPath_;
		std::ifstream ifsTxt1m_;
		std::ifstream ifsTxt10m_;
		std::ifstream ifsTxt40m_;
		std::ifstream ifsPred1m_;
		std::ifstream ifsPred10m_;
		std::ifstream ifsPred40m_;
		std::vector<PredictionSet> buffer_;
		std::unordered_map<std::string, std::vector<PredictionSet>> tradesBySym_;
	};

private:
	bool debug_;
	int iPolicy_;
	int verbose_;
	int maxShrChg_;
	bool weightedPnl_;
	float wgtSecondPred_;
	float capLevel_;
	int cutoffTime_;
	double thres_;
	double maxPos_;
	double maxPosTicker_;
	double currPos_;
	std::string sigModel_;
	std::string dir_;
	std::string market_;
	std::string summPath_;
	std::string detailPath_;
	std::ofstream ofs_;

	std::vector<int> ntrees_;
	std::string fullTargetName_;
	std::vector<std::string> targetNames_;

	Acc accIntra_;
	Acc accClcl_;
	Acc accClop_;
	Acc accNtrd_;
	Acc accDV_;
	Acc accTotal_;

	std::map<std::string, int> mPos_;

	void endDay_new();
	void get_pred_sprd(std::vector<double>& vPred, std::vector<double>& vSprd);
};

#endif
