#ifndef __gtlib_tickdump_DumpTick__
#define __gtlib_tickdump_DumpTick__
#include <string>
#include <vector>

namespace gtlib {

class DumpTick {
public:
	DumpTick(){}
	~DumpTick(){}
	virtual void summ() = 0;
	virtual void dump(const std::string& ticker, int msecs = 0) = 0;
};

class DumpOrder: public DumpTick {
public:
	DumpOrder(int idate, const std::vector<std::string>& dirs, int n)
		:d_(idate, dirs, n){}
	virtual void summ(){d_.summ();}
	virtual void dump(const std::string& ticker, int msecs = 0){d_.dump(ticker, msecs);}
private:
	DumpImpl<OrderData> d_;
};

class DumpQuote: public DumpTick {
public:
	DumpQuote(int idate, const std::vector<std::string>& dirs, int n)
		:d_(idate, dirs, n){}
	virtual void summ(){d_.summ();}
	virtual void dump(const std::string& ticker, int msecs = 0){d_.dump(ticker, msecs);}
private:
	DumpImpl<QuoteInfo> d_;
};

class DumpTrade: public DumpTick  {
public:
	DumpTrade(int idate, const std::vector<std::string>& dirs, int n)
		:d_(idate, dirs, n){}
	virtual void summ(){d_.summ();}
	virtual void dump(const std::string& ticker, int msecs = 0){d_.dump(ticker, msecs);}
private:
	DumpImpl<TradeInfo> d_;
};

class DumpReturn: public DumpTick  {
public:
	DumpReturn(int idate, const std::vector<std::string>& dirs, int n)
		:d_(idate, dirs, n){}
	virtual void summ(){d_.summ();}
	virtual void dump(const std::string& ticker, int msecs = 0){d_.dump(ticker, msecs);}
private:
	DumpImpl<ReturnData> d_;
};

} // namespace gtlib

#endif
