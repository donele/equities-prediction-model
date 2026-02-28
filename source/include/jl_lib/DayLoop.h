#ifndef __DayLoop__
#define __DayLoop__
#include <string>
#include <set>

class OneDayStat;

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC DayLoop {
public:
	DayLoop();
	virtual ~DayLoop();

	virtual void day_loop(int startDay, int lastDay) = 0;
	void set_valid_symbols( const std::set<std::string>& valid_symbols );
	bool read_binary( const std::string& bin_dir, int idate, OneDayStat* ods, bool longTicker,
		  int nTickerMin = 0, int symbolSizeMax = 12 );
	virtual void finish(){ return; }

protected:
	std::set<std::string> valid_symbols_;
};

#endif
