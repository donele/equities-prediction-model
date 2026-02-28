#ifndef __HConfig__
#define __HConfig__
#include <jl_lib/Arg.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

struct CLASS_DECLSPEC ModuleInfo {
	ModuleInfo(const std::string& cn, const std::string& mn, int eo)
		:className(cn), moduleName(mn), execOrder(eo){}
	std::string className;
	std::string moduleName;
	int execOrder;
};

class CLASS_DECLSPEC HConfig {
public:
	HConfig(){}
	HConfig(std::istream& is, const std::vector<std::string>& lines = std::vector<std::string>(), const std::string& confDir = "");

	void add_module(const std::pair<ModuleInfo, std::multimap<std::string, std::string> >& module);
	std::vector<std::pair<ModuleInfo, std::multimap<std::string, std::string> > > get_block();

private:
	std::vector<std::pair<ModuleInfo, std::multimap<std::string, std::string> > > conf_;

	void parse_conf(std::istream& is);
	void get_modules( std::istream& is, std::vector<ModuleInfo>& modules );
	void get_options( std::istream& is, const std::string& moduleName,
		std::map<std::string, std::multimap<std::string, std::string> >& options );
};

FUNC_DECLSPEC std::ostream& operator <<
(std::ostream& os, const std::vector<std::pair<ModuleInfo, std::multimap<std::string, std::string> > >& obj);

struct CLASS_DECLSPEC ModuleConfig {
	ModuleConfig(const std::string& cn, const std::string& mn, int eo)
		:className(cn), moduleName(mn), execOrder(eo){}
	std::string className;
	std::string moduleName;
	int execOrder;
	Arg arg;
};

class CLASS_DECLSPEC HConfig2 {
public:
	HConfig2(){}
	HConfig2(std::istream& is, const std::vector<std::string>& lines = std::vector<std::string>());

private:
	std::vector<ModuleConfig> conf_;

	void parse_conf(std::istream& is);
	std::vector<ModuleInfo> find_modules(std::istream& is);
	void get_options( std::istream& is, const std::string& moduleName,
		std::map<std::string, std::multimap<std::string, std::string> >& options );
};

#endif
