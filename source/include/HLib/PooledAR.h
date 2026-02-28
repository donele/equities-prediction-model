#ifndef __PooledAR__
#define __PooledAR__
#include <HLib/AR.h>
#include <vector>
#include <map>
#include <string>
#include <jl_lib/jlutil.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC PooledAR {
public:
	PooledAR(){}
	PooledAR(int filterLen, bool scale);

	void add(std::string ticker, double v);
	void add(int lag, double v0, double v1);
	std::vector<double> getCoeffs();
	void beginTicker(const std::string& ticker);
	void endTicker(const std::string& ticker);
	void calculate_mean();
	double pred(std::string ticker, std::vector<double>& v);

private:
	int filterLen_;
	double tickerMean_;
	double tickerStdev_;
	bool scale_;
	AR ar_;	
	std::map<std::string, Acc> mTickerAcc_;
	std::map<std::string, double> mTickerMean_;
	std::map<std::string, double> mTickerStdev_;
	Acc* acc_;

	friend FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const PooledAR& obj);
	friend FUNC_DECLSPEC std::istream& operator >>(std::istream& is, PooledAR& obj);
};


#endif
