#ifndef __GEX__
#define __GEX__
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

class CLASS_DECLSPEC GEX {
public:
	static GEX* Instance();
	Exchange* get(const std::string& market);

private:
	static GEX* instance_;
	struct Cleaner { ~Cleaner(); };
	friend struct Cleaner;
	GEX();

	std::map<std::string, Exchange*> m_;
};

#endif
