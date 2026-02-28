#ifndef __fitanaObjs__
#define __fitanaObjs__
#include <iostream>

namespace gtlib {

struct EvStat {
	EvStat():npts(0), ev(0.), of(0.), malpred(0.){}
	void merge(const EvStat& rhs);
	int npts;
	double ev;
	double of;
	double malpred;
};

struct MevStat {
	MevStat():ntrds(0), mev(0.), bias(0.){}
	void merge(const MevStat& rhs);
	int ntrds;
	double mev;
	double bias;
};

struct AnaDayStat {
	AnaDayStat():npts(0), cbm(0.), ctb(0.){}
	void merge(const AnaDayStat& rhs);
	int npts;
	double cbm;
	double ctb;
	EvStat evBm;
	EvStat evTb;
	MevStat mevBm;
	MevStat mevTb;
};

std::ostream& operator <<(std::ostream& os, const AnaDayStat& rhs);

} // namespace gtlib
#endif
