#include <gtlib/EstTime.h>
#include <jl_lib/jlutil.h>
#include <iostream>
using namespace std;

namespace gtlib {

EstTime::EstTime(int nTickers)
	:nTickers_(nTickers),
	cntTicker_(0),
	cntSincePrint_(0),
	secBeforeBeginTicker_(0.),
	minWaitSec_(30),
	maxDotInRow_(60)
{
	start_ = std::chrono::system_clock::now();
	print_ = start_;
}

void EstTime::beginTicker()
{
	if( cntTicker_ == 0 )
	{
		auto end = std::chrono::system_clock::now();
		secBeforeBeginTicker_ = std::chrono::duration_cast<std::chrono::seconds>(end - start_).count();
		firstTicker_ = end;
		print_ = end;
		string message = getInitialMessage();
		cout << message << endl;
	}
	if( isPowerOfTwo(cntTicker_) )
		printEst();
	++cntTicker_;
}

void EstTime::printEst()
{
	auto end = std::chrono::system_clock::now();
	double secSincePrint = std::chrono::duration_cast<std::chrono::seconds>(end - print_).count();
	if( secSincePrint > minWaitSec_ )
	{
		double secSinceFirstTicker = std::chrono::duration_cast<std::chrono::seconds>(end - firstTicker_).count();
		double secTotEst = secBeforeBeginTicker_ + secSinceFirstTicker * (double)nTickers_ / cntTicker_;

		string message = getMessage(secTotEst);
		cout << message << endl;

		print_ = end;
		cntSincePrint_ = 0;
	}
	cout.flush();
}

string EstTime::getInitialMessage()
{
	char buf[200];
	if( secBeforeBeginTicker_ < 60 )
		sprintf(buf, "Ticker #1/%d. So Far: %.1f sec", nTickers_, secBeforeBeginTicker_);
	else
		sprintf(buf, "Ticker #1/%d. So Far: %.1f min", nTickers_, secBeforeBeginTicker_/60.);
	return buf;
}

string EstTime::getMessage(double secTotEst)
{
	char buf[200];
	if( secTotEst < 60. )
		sprintf(buf, "Ticker #%d/%d. Tot(Est.): %.1f sec %s",
			cntTicker_, nTickers_, secTotEst, getMemoryInfoSimple().c_str());
	else
		sprintf(buf, "Ticker #%d/%d. Tot(Est.): %.1f min %s",
			cntTicker_, nTickers_, secTotEst/60., getMemoryInfoSimple().c_str());
	return buf;
}

void EstTime::beginEndDay()
{
	string message = getBeginEndMessage();
	cout << message << endl;
}

string EstTime::getBeginEndMessage()
{
	auto end = std::chrono::system_clock::now();
	double secTot = std::chrono::duration_cast<std::chrono::seconds>(end - start_).count();
	char buf[200];
	if( secTot < 60 )
		sprintf(buf, "EstTime::beginEndDay() So Far: %.0f seconds.", secTot);
	else if( secTot > 60 )
		sprintf(buf, "EstTime::beginEndDay() So Far: %.1f minutes.", secTot/60.);
	return buf;
}

void EstTime::endDay()
{
	string message = getEndMessage();
	cout << message << endl;
}

string EstTime::getEndMessage()
{
	auto end = std::chrono::system_clock::now();
	double secTot = std::chrono::duration_cast<std::chrono::seconds>(end - start_).count();
	char buf[200];
	if( secTot < 60 )
		sprintf(buf, "EstTime::endDay() Tot: %.0f seconds.", secTot);
	else if( secTot > 60 )
		sprintf(buf, "EstTime::endDay() Tot: %.1f minutes.", secTot/60.);
	return buf;
}

} // namespace gtlib
