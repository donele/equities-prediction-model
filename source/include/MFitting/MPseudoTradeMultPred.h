#ifndef __MPseudoTradeMultPred__
#define __MPseudoTradeMultPred__
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

class CLASS_DECLSPEC MPseudoTradeMultPred: public MModuleBase {
public:
	MPseudoTradeMultPred(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MPseudoTradeMultPred();

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
		PredictionSet(const std::string& model, std::vector<Prediction>& v, float thres, bool lastPredOptional);
		std::string uid;
		int time;
		int bs;
		float totPred;
		std::vector<float> vPred;
		float mid;
		float sprd;
		float cost;
	};

	class TradeSim {
	public:
		TradeSim(){}
		~TradeSim();
		TradeSim(std::vector<std::string>& targetNames, const std::string& baseDir,
				std::vector<std::string>& predModels, std::string& sigModel, const std::string& fitDesc,
				int udate, int idate, double maxPosTicker,
				const std::vector<int>& ntrees, std::vector<float> wgt,
				int cutoffTime, float capLevel, float thres, bool lastPredOptional, bool debug=false);
		int getIndxPred(const std::string& line, int ntrees);
		void select_trades();
		DayPnl calculate_pnl();
		void clear_buffer();
		bool getNextTrade(PredictionSet& p);
		bool isTradable(PredictionSet& p);
		int get_bs(float pred, float cost);
		Prediction getPred(int indxPred, const std::string& lineTxt, const std::string& linePred, float wgt);
		bool isNewTicker(const PredictionSet& p);
		void flush_buffer();
		void addData(PredictionSet& p);
	private:
		bool debug_;
		bool lastPredOptional_;
		int cutoffTime_;
		float capLevel_;
		float thres_;
		double maxPosTicker_;
		std::vector<int> vIndxPred_;
		std::vector<float> wgt_;
		std::string model_;
		std::vector<std::ifstream*> vIfsTxt_;
		std::vector<std::ifstream*> vIfsPred_;
		std::vector<PredictionSet> buffer_;
		std::unordered_map<std::string, std::vector<PredictionSet>> tradesBySym_;
	};

private:
	bool debug_;
	bool setSecondModelNTreesTmProd_;
	bool lastPredOptional_;
	int verbose_;
	int maxShrChg_;
	bool weightedPnl_;
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
	std::vector<std::string> predModels_;
	std::vector<float> wgt_;

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
