#ifndef __HCore__
#define __HCore__
#include <HLib.h>
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

class CLASS_DECLSPEC HCore {
public:
	HCore(std::string filename);
	HCore(std::istream& is);
	HCore(HConfig& conf);
	void run();

private:
	HConfig conf_;
	std::multimap<int, HModule*> modules_;
	typedef std::multimap<int, HModule*>::iterator itmod;

	void init(std::vector<std::pair<ModuleInfo, std::multimap<std::string, std::string> > > conf);
	void runImpl();
	void loop_markets_dates();
	void loop_dates_markets();
	void loop_tickers();
	void loopModule(std::string ticker);
	void loop_tickers_thread(int iThread);

	void parse_conf(std::string filename,
		std::vector<std::pair<ModuleInfo, std::multimap<std::string, std::string> > >& conf);
	void get_modules( std::istream& ifs, std::vector<ModuleInfo>& modules );
	void get_options( std::istream& ifs, std::string moduleName,
		std::map<std::string, std::multimap<std::string, std::string> >& options );
};

#endif
