#ifndef __MPseudoTradeProfile__
#define __MPseudoTradeProfile__
#include <MFramework.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <MFitting/TickerPosition.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MPseudoTradeProfile: public MModuleBase {
public:
	MPseudoTradeProfile(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MPseudoTradeProfile();

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

	struct Performance {
		double intra;
		double pred;
		double clcl;
		double clop;
		double dv;
		int nbuy;
		int nsell;
		Acc accIntra;
		Acc accClcl;
		Acc accClop;
		Acc accNtrd;
		Acc accDV;
		Acc accTotal;
		void beginDay();
		void endDay();
	};

	//struct CharaInfo {
	//	std::vector<float> v;
	//	std::map<std::string, int> mUidIndx;
	//	std::vector<int> vIndx;

	//	void read(std::string market, int idate, std::string condVar, int nP);
	//	int get_indx(std::string uid);
	//};

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
	double thres_;
	double maxPos_;
	double maxPosTicker_;
	double restoreMax_;
	std::string dir_;
	std::string predName_;
	std::string market_;
	std::string filename_;
	std::ofstream ofs_;
	std::string condVar_;

	std::vector<Performance> perf_;
	int ntrdfail_;
	int nMin_;
	double previous_position_;
	std::vector<double> vPositionMin_;
	std::map<std::string, TickerPosition> mPos_;
	CharaProfile charaInfo_;

	void endDayOld();
};

#endif
