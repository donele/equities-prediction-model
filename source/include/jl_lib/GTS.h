#ifndef __GTS__
#define __GTS__
#include <jl_lib/TickerSector.h>
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

class CLASS_DECLSPEC GTS {
public:
	static GTS* Instance();

	TickerSector* get(const std::string& market, int idate);

private:
	static GTS* instance_;
	struct Cleaner { ~Cleaner(); };
	friend struct Cleaner;
	GTS();

	std::map<std::string, TickerSector*> m_;
};

#endif
