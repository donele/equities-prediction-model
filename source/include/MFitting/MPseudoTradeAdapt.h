#ifndef __MPseudoTradeAdapt__
#define __MPseudoTradeAdapt__
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

class CLASS_DECLSPEC MPseudoTradeAdapt: public MModuleBase {
public:
	MPseudoTradeAdapt(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MPseudoTradeAdapt();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	//struct TickerPosition {
	//	TickerPosition():position(0){}
	//	int adjpos(int msecs, int hold);
	//	void add(int msecs, int shares);
	//	void beginDay();

	//	std::map<int, int> mMsecsPos;
	//	int position;
	//};

	//struct DaySum {
	//	DaySum():intra(0.), pred(0.), clcl(0.), clop(0.), dv(0.), nbuy(0), nsell(0){}
	//	double intra;
	//	double pred;
	//	double clcl;
	//	double clop;
	//	double dv;
	//	int nbuy;
	//	int nsell;
	//};

	struct TradeMade {
		TradeMade(float m, float p):maxThres(m), pnl(p) {}
		float maxThres;
		float pnl;
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
	double thres_seed_;
	double thres_adapt_;
	double fee_;
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

	double min_medvol_;
	double max_medvol_;
	double max_thres_;
	double thres_step_;
	double learning_rate_seed_;
	double learning_rate_;
	std::vector<TradeMade> trades_;
	std::set<std::string> uids_;

	int ntrdfail_;
	int nMin_;
	double previous_position_;
	std::vector<double> vPositionMin_;
	std::map<std::string, TickerPosition> mPos_;

	void market_close(DaySum& daysum, std::ofstream& ofsDebug, int idate);
	void thres_ana();
	void endDayOld();
};

#endif
