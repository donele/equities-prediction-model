#ifndef __GCurr__
#define __GCurr__
#include "optionlibs/TickData.h"
#include <string>
#include <vector>
#include <map>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC GCurr {
public:
	static GCurr* Instance();
	double convert(const std::string& marketTo, const std::string& marketFrom, double price);
	double exchrat(const std::string& marketTo, const std::string& marketFrom);
	void set_idate(int idate);

private:
	static GCurr* instance_;
	GCurr();

	int idate_;
	std::map<std::string, double> m_;
};

#endif
