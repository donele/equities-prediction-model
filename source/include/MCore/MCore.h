#ifndef __MCore__
#define __MCore__
#include <MFramework.h>
#include <jl_lib/HConfig.h>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <list>
#include <boost/thread.hpp>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MCore {
public:
	MCore(const std::string& confDir, const std::string& filename, bool print = false,
			const std::vector<std::string>& lines = std::vector<std::string>());
	MCore(std::istream& is, bool print = false, const std::vector<std::string>& lines = std::vector<std::string>());
	MCore(HConfig& conf);
	void run();

private:
	bool print_;
	HConfig conf_;
	std::multimap<int, MModuleBase*> modules_;
	typedef std::multimap<int, MModuleBase*>::iterator itmod;

	void init(std::vector<std::pair<ModuleInfo, std::multimap<std::string, std::string> > >& conf);
	void runImpl();
	void loop_markets_dates();
	void loop_dates_markets();
	void loop_tickers();
	void loopModule(const std::string& ticker);
	void loop_tickers_thread(int iThread);
	void loop_endDay(int idate);
};

#endif
