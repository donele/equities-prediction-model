#ifndef __GapFinder__
#define __GapFinder__
#include <string>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC GapFinder {
public:
	GapFinder();
	GapFinder(std::string market, int idate);
	~GapFinder();

private:
};

#endif
