#ifndef __HReadChara__
#define __HReadChara__
#include <HLib/HModule.h>
#include <HLib.h>
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

class CLASS_DECLSPEC HReadChara: public HModule {
public:
	HReadChara(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HReadChara();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void endMarket();
	virtual void endJob();

private:
	bool debug_;
	hff::CharaContainer chara_;
	//std::map<std::string, std::set<std::string> > mMktUids_;
	std::set<std::string> uids_;

	void set_uid_list();
	void read_chara(std::string market, int idate);
};

#endif