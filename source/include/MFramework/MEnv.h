#ifndef __MEnv__
#define __MEnv__
#include <string>
#include <vector>
#include <MFramework.h>
#include <gtlib_model/FutSigPar.h>
#include <gtlib_model/hff.h>
#include <boost/thread.hpp>

class CLASS_DECLSPEC MEnv {
public:
	static MEnv* Instance();
	void initialize();
	bool multiThread() const;
	bool multiThreadModule;
	bool multiThreadTicker;
	int nMaxThreadTicker;
	int udate;
	int debugFlag;
	std::string loopingOrder;
	std::string sourceFlag;
	std::string fitDesc;
	std::string sigType;
	std::string fullTargetName;
	std::vector<std::string> markets;
	std::string baseDir;
	std::string confDir;
	std::vector<int> idates;
	std::vector<int> allIdates;
	std::map<std::string, std::vector<int> > midates;
	std::vector<std::string> tickersDebug;
	std::vector<std::string> tickers;
	std::string market;
	std::string model;
	int nTickerMax;
	int idate;
	int d1;
	int d2;

	hff::IndexFilters indexFilters;
	hff::LinearModel linearModel;
	hff::NonLinearModel nonLinearModel;
	FutSigPar futSigPar;

	boost::mutex io_mutex;
	boost::mutex event_mutex;

private:
	static MEnv* instance_;
	struct Cleaner { ~Cleaner(); };
	friend struct Cleaner;
	MEnv();
	~MEnv();
	MEnv( const MEnv& );
	const MEnv& operator=( const MEnv& );
};

#endif
