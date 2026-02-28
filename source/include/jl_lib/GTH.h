#ifndef __GTH__
#define __GTH__
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

class CLASS_DECLSPEC GTH {
public:
	static GTH* Instance();

	void init(const std::string& market);
	TickerHistory* get(const std::string& market);
	void renew(const std::string& market);

private:
	static GTH* instance_;
	struct Cleaner { ~Cleaner(); };
	friend struct Cleaner;
	GTH();

	std::map<std::string, TickerHistory*> m_;
};

#endif
