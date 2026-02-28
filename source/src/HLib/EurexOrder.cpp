#include <HOrders.h>

EurexOrder::EurexOrder()
{}

EurexOrder::EurexOrder(double p, int q, char s, int o, int e)
:price(p),
qty(q),
side(s),
orderMsecs(o),
eventMsecs(e),
tradeMsecs(0),
quoteMsecs(0),
priceA(0),
qtyA(0),
orderMsecsA(0),
eventMsecsA(0),
tradeMsecsA(0),
quoteMsecsA(0),
rpnl(0.)
{}

bool EurexOrder::match() const
{
	return false;
}