#ifndef __SpreadBook__
#define __SpreadBook__
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

class CLASS_DECLSPEC SpreadBook {
public:
	friend class SpreadBook;
	SpreadBook(bool uncross = false);
	SpreadBook(const std::string& market, int idate);
	SpreadBook(const std::map<signed, unsigned>& bidvol, const std::map<signed, unsigned>& askvol, bool uncross = false);
	~SpreadBook();

	void remove_crosses( TickSeries<OrderDataSpread>& tsOin, TickSeries<OrderDataSpread>& tsOout );
	void report_remove_crosses();
	bool negative_spread();
	bool positive_spread();
	int n_ticker_uncrossed();

	void clear();
	bool update( const OrderDataSpread& order );

	bool get_quote( QuoteInfo& quote, int lot = 1 );
	bool get_quote_lot( QuoteInfo& quote );
	int get_bid_depth();
	int get_ask_depth();
	std::map<signed, unsigned> get_bidvol();
	std::map<signed, unsigned> get_askvol();
	void set_exit_on_negative_size( bool tf );
	std::vector<OrderDataSpread> get_book_diff( SpreadBook& newBook, unsigned msecs = 0 );
	void remove_zeros();
	bool is_inverted();
	void set_verbose( bool tf );
	bool existUncrossingOrder();
	std::vector<OrderDataSpread> getUncrossingOrders();
	friend std::ostream& operator <<(std::ostream& os, SpreadBook& obj);

private:
	bool uncross_;
	bool verbose_;
	bool exit_on_negative_size_;
	bool uncrossedAfterOpen_;
	double scale_;
	unsigned msec_;
	std::map<signed, unsigned> bidvol;
	std::map<signed, unsigned> askvol;
	Sessions sessions_;
	std::vector<OrderDataSpread> uncrossingOrders_;

	// remove_crosses.
	int n_tickers_;
	double n_ticker_uncrossed_;
	double n_input_orders_;
	double n_order_removed_;
	std::vector<int> v_n_order_removed_;
};

FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const SpreadBook& obj);
FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const OrderDataSpread& obj);

#endif __SpreadBook__
