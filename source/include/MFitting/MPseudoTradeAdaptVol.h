#ifndef __MPseudoTradeAdaptVol__
#define __MPseudoTradeAdaptVol__
#include <MFramework.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <MFitting/MPseudoTradePrep.h>
#include <MFitting/TickerPosition.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MPseudoTradeAdaptVol: public MModuleBase {
public:
	MPseudoTradeAdaptVol(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MPseudoTradeAdaptVol();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	struct TradeMade {
		TradeMade(float m, float p):maxThres(m), pnl(p) {}
		float maxThres;
		float pnl;
	};

	struct ThresInfo {
		ThresInfo(double tsd, double lrs):thres(tsd), learning_rate(lrs){}
		void beginDay(){trades.clear();}
		double thres;
		double learning_rate;
		std::vector<TradeMade> trades;
	};

private:
	bool debug_;
	int verbose_;
	int maxShrChg_;
	bool addON_;
	int ONmult_;
	int minMsecs_;
	int maxMsecs_;
	int minMsecsON_;
	int maxMsecsON_;
	int hold_;
	int nP_;
	double thres_seed_;
	double maxPos_;
	double maxPosTicker_;
	double restoreMax_;
	std::string dir_;
	std::string predName_;
	std::string market_;
	std::string filename_;
	std::ofstream ofs_;

	Acc accIntra_;
	Acc accClcl_;
	Acc accClop_;
	Acc accNtrd_;
	Acc accDV_;
	Acc accTotal_;

	double max_thres_;
	double thres_step_;
	double learning_rate_seed_;

	std::vector<ThresInfo> thresInfo_;
	int ntrdfail_;
	int nMin_;
	double previous_position_;
	std::vector<double> vPositionMin_;
	std::map<std::string, TickerPosition> mPos_;
	CharaProfile charaInfo_;

	void market_close(DaySum& daysum, std::ofstream& ofsDebug, int idate);
	void thres_ana();
	void endDayOld();
};

#endif
