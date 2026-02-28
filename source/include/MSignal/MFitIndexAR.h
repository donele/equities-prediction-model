#ifndef __MFitIndexAR__
#define __MFitIndexAR__
#include <MFramework/MModuleBase.h>
#include <MFramework.h>
#include <gtlib_linmod/ARMulti.h>
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

class CLASS_DECLSPEC MFitIndexAR: public MModuleBase {
public:
	MFitIndexAR(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MFitIndexAR();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool single_thread_;
	int nDays_;
	std::map<std::string, gtlib::ARMulti> mar_;

	void add_data(int iFilter);
};

#endif
