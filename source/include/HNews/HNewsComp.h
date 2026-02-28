#ifndef __HNewsComp__
#define __HNewsComp__
#include <HLib/HModule.h>
#include <optionlibs/TickData.h>
#include <jl_lib/Sessions.h>
#include <TH1.h>
#include <TH2.h>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HNewsComp: public HModule {
public:
	HNewsComp(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HNewsComp();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	bool debug_;
	int window_;
	Sessions sessions_;
	boost::mutex hist_mutex_;

	TH2F hCorr_;
	TH1F hLag_;
	TH1F hLagZ_;

	TH1F hNovelRP_;
	TH1F hNovelRT_;

	TH1F hLag_novelRP_;
	TH1F hLag_novelRT_;
	TH1F hLag_novelRPZ_;
	TH1F hLag_novelRTZ_;

	void read_news(std::string ticker, int idate, std::vector<int>& vmsecsRP, std::vector<int>& vsentiRP, std::vector<int>& vmsecsRT, std::vector<int>& vsentiRT);
	void compare(std::string market, int idate, std::vector<int>& vmsecsRP, std::vector<int>& vsentiRP, std::vector<int>& vmsecsRT, std::vector<int>& vsentiRT);
	bool findNews(int t0, int& msecs, int& senti, std::vector<int>& vmsecs, std::vector<int>& vsenti);
};

#endif
