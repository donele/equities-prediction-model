#ifndef __gtlib_predana_GsprStatDef__
#define __gtlib_predana_GsprStatDef__

namespace gtlib {

struct BasicStat {
	BasicStat():binAvg(0.), binStdev(0.), npt(0){}
	float binAvg;
	float binStdev;
	int npt;
};

struct PredStat {
	PredStat():mX(0.), mY(0.), dX(0.), dY(0.), corr(0.), lev(0.){}
	float mX, mY, dX, dY;
	float corr, lev;
};

struct TopBottomStat {
	TopBottomStat():frcEv1p(0.), ev1p(0.){}
	float frcEv1p;
	float ev1p;
};

struct TradableStat {
	TradableStat():frcEvT(0.), evT(0.){}
	float frcEvT;
	float evT;
	int relOf;
};

struct GsprStat {
	GsprStat():gsprRat(0.), predGspr(0.), realGspr(0.), plPpt(0.){}
	float gsprRat;
	float predGspr;
	float realGspr;
	float plPpt;
};

struct MaxStat {
	MaxStat():maxPlPpt(0.), maxGsprRat(0.){}
	float maxPlPpt;
	float maxGsprRat;
};

struct RelativeStat {
	RelativeStat():relPlPpt(0), relGsprRat(0){}
	int relPlPpt;
	int relGsprRat;
};

} // namespace gtlib

#endif
