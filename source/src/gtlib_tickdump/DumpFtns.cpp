#include <gtlib_tickdump/DumpFtns.h>
using namespace std;

namespace gtlib {

string getDumpMeta(int idate, const string& ticker, int nticks)
{
	char buf[200];
	sprintf(buf, "idate: %d  ticker: %s  nticks: %d",
			idate, ticker.c_str(), nticks);
	return buf;
}

template <>
std::string getDumpHeader<OrderData>()
{
	return "msecs\tprice\tsize\ttype";
}

template <>
std::string getDumpHeader<QuoteInfo>()
{
	return "msecs\tbidEx\tbidSize\tbid\task\taskSize\taskEx";
}

template <>
std::string getDumpHeader<TradeInfo>()
{
	return "msecs\tprice\tqty\tflag";
}

template <>
std::string getDumpHeader<ReturnData>()
{
	return "msecs\tprice\tsize\ttype";
}

template <>
std::string getDumpContent<OrderData>(const OrderData& order)
{
	char buf[200];
	sprintf(buf, "%d\t%d\t%d\t%d", order.msecs, order.price, order.size, order.type);
	return buf;
}

template <>
std::string getDumpContent<QuoteInfo>(const QuoteInfo& quote)
{
	char buf[200];
	sprintf(buf, "%d\t%d\t%d\t%.2f\t%.2f\t%d\t%d",
			quote.msecs, quote.bidEx, quote.bidSize, quote.bid,
			quote.ask, quote.askSize, quote.askEx);
	return buf;
}

template <>
std::string getDumpContent<TradeInfo>(const TradeInfo& trade)
{
	char buf[200];
	sprintf(buf, "%d\t%.2f\t%d\t%d", trade.msecs, trade.price, trade.qty, trade.flags);
	return buf;
}

template <>
std::string getDumpContent<ReturnData>(const ReturnData& ret)
{
	char buf[200];
	sprintf(buf, "%d\t%.8f", ret.msecs, ret.ret);
	return buf;
}
} // namespace gtlib
