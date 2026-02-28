#ifndef __HTickReadIndex__
#define __HTickReadIndex__
#include <HLib/HModule.h>
#include "optionlibs/TickData.h"
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

class CLASS_DECLSPEC HTickReadIndex: public HModule {
public:
	HTickReadIndex(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HTickReadIndex();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	int verbose_;
	bool bps_;
	bool fitting_index_;
	std::string return_dir_;
	std::map<std::string, std::string> m_return_dir_;
	std::string format_;
	std::set<std::string> retNames_;
	std::set<std::string> tickersRead_;

	void read_dir(std::string dir, std::string market, int idate);
	void remove_data(int idate);
};

#endif