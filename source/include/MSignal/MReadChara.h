#ifndef __MReadChara__
#define __MReadChara__
#include <MFramework/MModuleBase.h>
#include <MFramework.h>
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

class CLASS_DECLSPEC MReadChara: public MModuleBase {
public:
	MReadChara(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MReadChara();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	//bool readHfuniverse_;
	std::set<std::string> uids_;
	hff::CharaContainer chara_;
	std::unordered_map<std::string, std::string> mTickerUid_;

	void read_chara(const std::string& market, int idate);
};

#endif
