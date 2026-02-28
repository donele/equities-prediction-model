#ifndef __fastsimObjs__
#define __fastsimObjs__
#include <jl_lib/jlutil.h>

namespace gtlib {

struct CloseInfo {
	CloseInfo():close(0.), retclcl(0.), retclop(0.), adjust(1.){}
	CloseInfo(double c, double ro, double rc, double ad = 1.):close(c), retclop(ro), retclcl(rc), adjust(ad){}
	double close; // today's close.
	double retclop; // from today's close to the next day's open.
	double retclcl;
	double adjust;
};

struct DailySimStat {
	DailySimStat();
	double retIntra;
	double retClcl;
	double retClop;
	double intra;
	double clcl;
	double clop;
	double dv;
	double dvBuy;
	int n;
	double absDolPos;
	double sgnDolPos;
	double turnover;
	void print(const std::string& name);
};

class SampleData {
public:
	SampleData(float s, float c, float pr, float pc, int bs, int as, const std::string& t, int m);
	std::string ticker;
	float sprd;
	float cost;
	float pred;
	float price;
	int bidSize;
	int askSize;
	double msecs;
	int getDPos(double restore, int pos, int maxShrTrade, double maxDolPos,
			double dolPosNet, double maxDolPosNet, double thres, int openMsecs, int closeMsecs);
	double getTradePrice(int dPos);
private:
	int getSide(double totPred, double thres);
	int getShrTrd(int side, int maxShrTrd, int pos, double maxDolPos, double dolPosNet, double maxDolPosNet);
	int getShrTrd(int side, int maxShrTrd, double dolPos, double maxDolPos);
};
bool operator<(SampleData const& a, SampleData const& b);

struct TickerData {
	TickerData():close(0.), pos(0), lastPrice(0.), lastSide(0), dvDay(0.){}
	float close;
	int pos;
	float lastPrice;
	int lastSide;
	double dvDay;
	//double dvBuyDay;
};

} // namespace gtlib
#endif
