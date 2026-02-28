#ifndef __RLobj__
#define __RLobj__
#include <vector>
#include <string>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

struct CLASS_DECLSPEC RLtrade {
	RLtrade(int indx_, int shr_, double F_):indx(indx_), shr(shr_), F(F_){}
	int indx;
	int shr;
	double F;
};

struct CLASS_DECLSPEC RLticker {
	RLticker():indxT(-1), indxLastTrade(0), indxLastTradeUnrewarded(0), position(0), dollarvol(0.){}
	void add(double mid);
	double pnl(double cost);
	double pnl2(double cost);

	int indxT;
	int indxLastTrade;
	int indxLastTradeUnrewarded;
	int position;
	double dollarvol;
	std::vector<double> vmid;
	std::vector<double> vrtn;
	std::vector<RLtrade> trades;
};

struct CLASS_DECLSPEC RLinput {
	RLinput(){}
	RLinput(std::string ticker_, double mid_):ticker(ticker_), mid(mid_){}
	std::string ticker;
	double mid;
};

#endif