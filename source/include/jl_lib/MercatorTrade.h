#ifndef __MercatorTrade__
#define __MercatorTrade__
#include <vector>
#include <gtlib_signal/QuoteView.h>
#include <gtlib_signal/QuoteSample.h>
#include <optionlibs/TickData.h>

struct CondVars {
	CondVars();
	float dayVolat;
	float dayVolatSurprise;
	float dayVolume;
	float dayVolumeSurprise;
	float volat;
	float volatSurprise;
	float dv;
	float dvSurprise;
	float share;
	float shareSurprise;
	float mercatorTrade;
	float mercatorTradeSurprise;
	float share1s;
	float share60s;
	float share3600s;
};

class MercatorTrade {
public:
	MercatorTrade(){}
	MercatorTrade(double ms, int sn, double pr, int qt, double p1, double p10,
			double md, char ex, int sch, int oqt, double op);

	int msecs;
	int sign;
	float price;
	int oqty;
	int qty;
	float mid;
	float cost;
	char execType;
	int schedType;

	float pred;
	float pred1;
	float pred10;
	float gain1;
	float gain61;
	float gainC;

	CondVars cv;
private:
};

#endif
