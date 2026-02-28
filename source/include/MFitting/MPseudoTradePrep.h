#ifndef __MPseudoTradePrep__
#define __MPseudoTradePrep__
#include <MFramework.h>
#include <jl_lib/TickSources.h>
#include <jl_lib/Book.h>
#include <gtlib_fastsim/fastsimObjs.h>
#include <map>
#include <string>
#include <vector>
#include <fstream>
//#include <MFitting/HFTrees.h>
#include <boost/thread.hpp>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MPseudoTradePrep: public MModuleBase {
public:
	MPseudoTradePrep(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MPseudoTradePrep();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

	struct Trade {
		Trade(){}
		Trade(std::string u, int t, double p, double m, double c, int bs_, double f = 0., double mo = 0., double mt = 0.)
			:uid(u), msecs(t), price(p), mid(m), cost(c), bs(bs_), pred(f), mmom(mo), mmtm(mt){};
		Trade(std::string u, int t, double p, double m, double c, int bs_, double f, int bsz, int asz)
			:uid(u), msecs(t), price(p), mid(m), cost(c), bs(bs_), pred(f), bidSize(bsz), askSize(asz){};
		Trade(std::string u, int t, double p, double m, double f, double c, double b, double a, int bsz, int asz)
			:uid(u), msecs(t), price(p), mid(m), pred(f), cost(c), bid(b), ask(a), bidSize(bsz), askSize(asz){};
		std::string uid;
		int msecs;
		double price;
		double mid;
		double cost;
		int bs;
		double pred;
		double mmom;
		double mmtm;
		double bid;
		double ask;
		int bidSize;
		int askSize;
	};

	struct CLASS_DECLSPEC TradeLess:
		public std::binary_function<Trade, Trade, bool> {
			bool operator()(const Trade& lhs, const Trade& rhs) const;
		};

	struct AuctionInfo {
		AuctionInfo():CA_price(0.), CA_size(0), OA_price(0.), OA_size(0){}
		AuctionInfo(double cp, int cs, double op, int os):CA_price(cp), CA_size(cs), OA_price(op), OA_size(os){}
		float CA_price;
		int CA_size;
		float OA_price;
		int OA_size;
	};

private:
	bool debug_;
	bool do_auction_;
	std::string market_;
	std::string selMarket_;
	boost::mutex priv_mutex_;
	TickSources tSources_;

	std::vector<std::string> bookDirs_;
	std::map<std::string, std::string> mTicker2Uid_;
	std::map<std::string, std::string> mUid2Ticker_;
	std::map<std::string, gtlib::CloseInfo> mClose_;
	std::map<std::string, AuctionInfo> mAuc_;
	std::map<std::string, std::map<int, QuoteInfo> > mMsecQuote_;
	std::map<std::string, std::vector<double> > mMid_;
	std::map<std::string, Book> mBookClose_;
	std::map<std::string, Book> mBookOpen_;
	std::map<std::string, int> mCAmsecs_;
	std::map<std::string, int> mOAmsecs_;

	bool book_complete(const Book& book);
	void get_close_info(std::map<std::string, gtlib::CloseInfo>& mClose, const std::vector<std::string>& markets, int idate);
	void get_auction_info(std::map<std::string, AuctionInfo>& mAuc, const std::vector<std::string>& markets, int idate);
	void get_book_close(std::map<std::string, Book>& mBook, const std::vector<std::string>& markets, int idate);
	void get_book_nextopen(std::map<std::string, Book>& mBook, const std::vector<std::string>& markets, int idate);
	void get_trades(std::vector<TradeInfo>& trades, TickAccessMulti<TradeInfo>& ta, const std::string& ticker, int idate);
	void get_orders(std::vector<OrderData>& orders, TickAccessMulti<OrderData>& ta, const std::string& ticker, int idate, int maxMsecs);
};

#endif
