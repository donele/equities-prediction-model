#include <jl_lib/MercatorOrder.h>

MercatorOrder::MercatorOrder()
{}

MercatorOrder::MercatorOrder(double p, int q, int qe, char s, int f, int o, int e, int bx, int bs, double b, double a, int as, int ax,
							 char et, int st, int ft, double pr1, double pr10)
:price(p),
qty(q),
qtyExec(qe),
side(s),
feedMsecs(f),
orderMsecs(o),
eventMsecs(e),
quoteMsecs(0),
detMsecs(0),
matMsecs(0),
trdMsecs(0),
insertMsecs(0),
executeMsecs(0),
bidEx(bx),
bidsize(bs),
bid(b),
ask(a),
asksize(as),
askEx(ax),
quoteMatch(1),
execType(et),
orderSchedType(st),
fillType(ft),
gain(0),
ret1(0.),
ret10(0.),
retR(0.),
retON(0.),
pred1(pr1),
pred10(pr10)
{}

bool MercatorOrder::isFilledFull() const
{
	return qty == qtyExec;
}

bool MercatorOrder::isFilledIncl() const
{
	return qtyExec > 0;
}

