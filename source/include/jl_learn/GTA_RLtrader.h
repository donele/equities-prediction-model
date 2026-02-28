#ifndef __GTA_RLtrader__
#define __GTA_RLtrader__
#include "GTModule.h"
#include "RLmodel.h"
#include "RLfeeder.h"
#include "RLobj.h"
#include <map>
#include <string>
#include <vector>

class __declspec(dllexport) GTA_RLtrader: public GTModule {
public:
	GTA_RLtrader(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~GTA_RLtrader();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void doTicker();
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	struct TickerDaySumm {
		TickerDaySumm():closePrc(0), closePosition(0), pnlClcl(0), pnlIntra(0), dollarvol(0), ntrades(0), pnlIntra2(0) {}
		TickerDaySumm(double cprc, int cpos, double pc, double pi, double dv, int nt, double pi2)
			:closePrc(cprc), closePosition(cpos), pnlClcl(pc), pnlIntra(pi), dollarvol(dv), ntrades(nt), pnlIntra2(pi2) {}

		double closePrc;
		int closePosition;
		double pnlClcl;
		double pnlIntra;
		double dollarvol;
		int ntrades;
		double pnlIntra2;
	};

	struct DaySumm {
		DaySumm():pnlClcl(0), pnlIntra(0), dollarvol(0), ntrades(0), shrpR(0) {}
		DaySumm(double pc, double pi, double dv, int nt, double sr):pnlClcl(pc), pnlIntra(pi), dollarvol(dv), ntrades(nt), shrpR(sr) {}
		double pnlClcl;
		double pnlIntra;
		double dollarvol;
		int ntrades;
		double shrpR;
	};

private:
	RLmodel* model_;
	RLfeeder* feeder_;
	std::map<std::string, std::map<int, TickerDaySumm> > mTickerDay_;
	std::map<int, DaySumm> mDay_;
	std::vector<std::vector<double> > vvWgts_;

	int iModel_;
	int maxPos_;
	double learn_;
	int rewardTime_;
	int interval_;
	int lenSeries_;
	double thres_;
	double thresIncr_;
	double cost_;
	double adap_;
	double maxWgt_;
	int verbose_;
	bool trainOnline_;
	bool writeRoot_;
	std::string timeUnit_;
	std::string opt_;
	std::vector<std::pair<int, int> > sessions_;
};

#endif