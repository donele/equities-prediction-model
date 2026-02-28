#ifndef __Book__
#define __Book__
#include "optionlibs/TickData.h"
#include <jl_lib/Sessions.h>
#include <map>
#include <vector>
#include <iostream>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC Book {
public:
	friend class Book;
	Book(bool uncross = false);
	Book(const std::string& market, int idate);
	Book(std::map<unsigned, unsigned>& bidvol, std::map<unsigned, unsigned>& askvol, bool uncross = false);
	~Book();

	void remove_crosses( TickSeries<OrderData>& tsOin, TickSeries<OrderData>& tsOout );
	void report_remove_crosses();
	bool negative_spread();
	bool positive_spread();
	int n_ticker_uncrossed();

	void clear();
	bool update( OrderData& order );

	bool get_quote( QuoteInfo& quote, int lot = 1 );
	bool get_quote_lot( QuoteInfo& quote );
	int get_bid_depth();
	int get_ask_depth();
	std::map<unsigned, unsigned> get_bidvol() const;
	std::map<unsigned, unsigned> get_askvol() const;
	void get_auction(double& price, int& size) const;
	void set_exit_on_negative_size( bool tf );
	std::vector<OrderData> get_book_diff( Book& newBook, unsigned msecs = 0 );
	void remove_zeros();
	bool is_inverted() const;
	void set_verbose( bool tf );
	bool existUncrossingOrder();
	std::vector<OrderData> getUncrossingOrders();
	friend std::ostream& operator <<(std::ostream& os, Book& obj);

private:
	bool uncross_;
	bool verbose_;
	bool exit_on_negative_size_;
	bool uncrossedAfterOpen_;
	double scale_;
	unsigned msec_;
	std::map<unsigned, unsigned> bidvol;
	std::map<unsigned, unsigned> askvol;
	Sessions sessions_;
	std::vector<OrderData> uncrossingOrders_;

	// remove_crosses.
	int n_tickers_;
	double n_ticker_uncrossed_;
	double n_input_orders_;
	double n_order_removed_;
	std::vector<int> v_n_order_removed_;
};

FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const Book& obj);
FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const OrderData& obj);

#endif __Book__
