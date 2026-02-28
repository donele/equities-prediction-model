#ifndef __MPseudoTradeRestore__
#define __MPseudoTradeRestore__
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

class CLASS_DECLSPEC MPseudoTradeRestore: public MModuleBase {
public:
	MPseudoTradeRestore(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MPseudoTradeRestore();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	int verbose_;
	int maxShrChg_;
	bool addON_;
	bool exper_univ_;
	int ONmult_;
	int minMsecs_;
	int maxMsecs_;
	int minMsecsON_;
	int maxMsecsON_;
	int hold_;
	double thres_;
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

	int ntrdfail_;
	int nMin_;
	double previous_position_;
	std::vector<double> vPositionMin_;
	std::map<std::string, TickerPosition> mPos_;
	std::set<std::string> uids_exper_;

	void market_close(DaySum& daysum, std::ofstream& ofsDebug, int idate);
	void endDayOld();
	void get_uids_exper_univ(const std::string& model2, int idate);
	void get_sticker(const std::string& market, char* cmd, std::set<std::string>& sticker);
};

#endif
