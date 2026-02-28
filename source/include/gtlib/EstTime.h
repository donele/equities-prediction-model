#ifndef __gtlib_EstTime__
#define __gtlib_EstTime__
#include <string>
#include <chrono>

namespace gtlib {

class EstTime {
public:
	EstTime(int nTickers);
	~EstTime(){}
	void beginTicker();
	void beginEndDay();
	void endDay();
private:
	int nTickers_;
	int cntTicker_;
	int cntSincePrint_;
	double secBeforeBeginTicker_;
	int minWaitSec_;
	int maxDotInRow_;
	std::chrono::system_clock::time_point start_;
	std::chrono::system_clock::time_point print_;
	std::chrono::system_clock::time_point firstTicker_;

	void printEst();
	std::string getInitialMessage();
	std::string getMessage(double secTotEst);
	std::string getBeginEndMessage();
	std::string getEndMessage();
};

} // gtlib

#endif
