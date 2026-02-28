#ifndef __MInitFut__
#define __MInitFut__
#include <MFramework/MModuleBase.h>
#include <gtlib_model/FutSigPar.h>
#include <optionlibs/TickData.h>
#include <boost/thread.hpp>
#include <boost/timer.hpp>
#include <vector>
#include <string>
#include <map>
#include <set>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MInitFut: public MModuleBase {
public:
	MInitFut(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MInitFut();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	int verbose_;
	bool multiThreadModule_;
	bool multiThreadTicker_;
	int nMaxThreadTicker_;
	int udate_;
	int d1_;
	int d2_;
	int nfitdates_;
	int noosdates_;
	int elapsed_prev_;
	int mem_prev_;
	std::string market_;
	std::string projName_;
	std::vector<std::string> products_;
	std::string baseDir_;
	boost::timer timer_;

	void set_idate_list();
	void set_ticker_list(int idate);
};

#endif
