#ifndef __TickerPosition__
#define __TickerPosition__
#include <map>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

struct CLASS_DECLSPEC TickerPosition {
	TickerPosition():position(0){}
	int adjpos(int msecs, int hold);
	void add(int msecs, int shares);
	void beginDay();

	std::map<int, int> mMsecsPos;
	int position;
};

struct CLASS_DECLSPEC DaySum {
	DaySum():intra(0.), pred(0.), clcl(0.), clop(0.), dv(0.), nbuy(0), nsell(0){}
	double intra;
	double pred;
	double clcl;
	double clop;
	double dv;
	int nbuy;
	int nsell;
};

#endif
