#include <jl_lib/SpreadBook.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include "optionlibs/TickData.h"
#include <iostream>
#include <vector>
#include <functional>
#include <numeric>
#include <algorithm>
using namespace std;

SpreadBook::SpreadBook(bool uncross)
:verbose_(false),
uncross_(uncross),
exit_on_negative_size_(false),
uncrossedAfterOpen_(false),
n_tickers_(0),
n_ticker_uncrossed_(0),
n_input_orders_(0),
n_order_removed_(0),
msec_(0),
scale_(10000.)
{
	sessions_.add_session(make_pair(0, 24*60*60*1000)); // market is unknown.
}

SpreadBook::SpreadBook(const string& market, int idate)
:verbose_(false),
exit_on_negative_size_(false),
uncrossedAfterOpen_(false),
n_tickers_(0),
n_ticker_uncrossed_(0),
n_input_orders_(0),
n_order_removed_(0),
msec_(0),
scale_(10000.)
{
	sessions_ = Sessions(market, idate);
}

SpreadBook::SpreadBook(const map<signed, unsigned>& bidmap, const map<signed, unsigned>& askmap, bool uncross)
:verbose_(false),
uncross_(uncross),
exit_on_negative_size_(false),
uncrossedAfterOpen_(false),
n_tickers_(0),
n_ticker_uncrossed_(0),
n_input_orders_(0),
n_order_removed_(0),
msec_(0),
scale_(10000.)
{
	bidvol = bidmap;
	askvol = askmap;
}

SpreadBook::~SpreadBook()
{}

void SpreadBook::remove_crosses( TickSeries<OrderDataSpread>& tsOin, TickSeries<OrderDataSpread>& tsOout )
{
	clear();
	OrderDataSpread order;
	OrderDataSpread last_order;
	vector<OrderDataSpread> uncrossingOrders;
	bool uncrossedAfterOpen = false;

	int n_order_removed_ticker = 0;
	++n_tickers_;
	int nOticks = tsOin.NTicks();
	n_input_orders_ += nOticks;
	tsOin.StartRead();
	for( int u=0; u<nOticks; ++u )
	{
		// read an order.
		tsOin.Read(&order);

		if( !uncrossingOrders.empty() )
		{
			if( order.msecs == last_order.msecs ) // remove generated orders if it overlaps with a logged order.
			{
				for( vector<OrderDataSpread>::iterator it = uncrossingOrders.begin(); it != uncrossingOrders.end(); )
				{
					if( it->price == order.price && it->size <= order.size && it->type == order.type )
						it = uncrossingOrders.erase(it);
					else
						++it;
				}
			}
			else if( order.msecs > last_order.msecs ) // add the generated orders to the output.
			{
				for( vector<OrderDataSpread>::iterator it = uncrossingOrders.begin(); it != uncrossingOrders.end(); ++it )
				{
					++n_order_removed_;
					++n_order_removed_ticker;
					tsOout.Write(*it);
				}
				uncrossingOrders.clear();
			}
		}

		unsigned bid_ask = (order.type >> 1) & (unsigned)1; // 0:bid, 1:ask
		unsigned add_del = order.type & (unsigned)1; // 0: add, 1:del

		if( 0 == bid_ask ) // bid
		{
			if( 0 == add_del ) // add
			{
				bidvol[order.price] += order.size;

				if( uncrossedAfterOpen && negative_spread() )
				{
					//bool trading = find_if(sessions_.begin(), sessions_.end(), bind2nd(inRange(), order.msecs)) != sessions_.end();
					bool trading = sessions_.isAfterOpenBeforeClose(order.msecs);
					if( trading )
					{
						// Uncross the book.
						while( negative_spread() && !askvol.empty() )
						{
							map<signed, unsigned>::iterator it = askvol.begin();

							OrderDataSpread uOrder;
							uOrder.msecs = order.msecs;
							uOrder.price = it->first;
							uOrder.size = it->second;
							uOrder.type = 3; // ask del
							uncrossingOrders.push_back(uOrder);

							askvol.erase(it);
						}
					}
				}
			}
			else if( 1 == add_del ) // del
			{
				unsigned thevol = bidvol[order.price];
				if( thevol >= order.size )
					bidvol[order.price] = thevol - order.size;
				else
					bidvol[order.price] = 0;
			}
		}
		else if( 1 == bid_ask ) // ask
		{
			if( 0 == add_del ) // add
			{
				askvol[order.price] += order.size;

				if( uncrossedAfterOpen && negative_spread() )
				{
					//bool trading = find_if(sessions_.begin(), sessions_.end(), bind2nd(inRange(), order.msecs)) != sessions_.end();
					bool trading = sessions_.isAfterOpenBeforeClose(order.msecs);
					if( trading )
					{
						// Uncross the book.
						while( negative_spread() && !bidvol.empty() )
						{
							map<signed, unsigned>::reverse_iterator it = bidvol.rbegin();

							OrderDataSpread uOrder;
							uOrder.msecs = order.msecs;
							uOrder.price = it->first;
							uOrder.size = it->second;
							uOrder.type = 1; // bid del
							uncrossingOrders.push_back(uOrder);

							bidvol.erase((++it).base());
						}
					}
				}
			}
			else if( 1 == add_del ) // del
			{
				unsigned thevol = askvol[order.price];
				if( thevol >= order.size )
					askvol[order.price] = thevol - order.size;
				else
					askvol[order.price] = 0;
			}
		}

		if( !uncrossedAfterOpen )
		{
			//bool trading = find_if(sessions_.begin(), sessions_.end(), bind2nd(inRange(), order.msecs)) != sessions_.end();
			bool trading = sessions_.isAfterOpenBeforeClose(order.msecs);
			if( trading )
			{
				if( positive_spread() )
					uncrossedAfterOpen = true;
			}
		}

		tsOout.Write(order);
		last_order = order;
		remove_zeros();
	}

	v_n_order_removed_.push_back(n_order_removed_ticker);
	if( n_order_removed_ticker > 0 )
		++n_ticker_uncrossed_;
	return;
}

void SpreadBook::report_remove_crosses()
{
	double rat = 0;
	if( n_input_orders_ > 0 )
		rat = n_order_removed_ / n_input_orders_;

	char buf[400];
	sprintf(buf, " ticker: %d order: %.1e removed: %d (%.1e)",
		n_tickers_, n_input_orders_, (int)n_order_removed_, rat);
	cout << buf << endl;

	sort(v_n_order_removed_.begin(), v_n_order_removed_.end());

	double mean = accumulate(v_n_order_removed_.begin(), v_n_order_removed_.end(), 0.0)/v_n_order_removed_.size();
	double median = v_n_order_removed_[v_n_order_removed_.size()/2];
	sprintf(buf, "   all tickers, removed orders mean %5.1f median %5.1f",
		mean, median);
	cout << buf << endl;

	vector<int>::iterator itNonzero = lower_bound(v_n_order_removed_.begin(), v_n_order_removed_.end(), 1);
	double n_ticker_removed = distance(itNonzero, v_n_order_removed_.end());
	double mean2 = 0;
	double median2 = 0;
	if( n_ticker_removed > 0 )
	{
		mean2 = accumulate(itNonzero, v_n_order_removed_.end(), 0.0)/n_ticker_removed;
		median2 = *(itNonzero + n_ticker_removed / 2);
	}
	sprintf(buf, " %5d tickers, removed orders mean %5.1f median %5.1f",
		(int)n_ticker_removed,  mean2, median2);
	cout << buf << endl;

	return;
}

int SpreadBook::n_ticker_uncrossed()
{
	return n_ticker_uncrossed_;
}

//void Book::order2quote( const TickSeries<OrderData>& tsOin, TickSeries<QuoteInfo>& tsQ )
//{
//	return;
//}

void SpreadBook::clear()
{
	bidvol.clear();
	askvol.clear();
	msec_ = 0;
	return;
}

void SpreadBook::set_verbose( bool tf )
{
	verbose_ = tf;
	return;
}

void SpreadBook::set_exit_on_negative_size( bool tf )
{
	exit_on_negative_size_ = tf;
	return;
}

bool SpreadBook::update( const OrderDataSpread& order )
{
	bool ret = true;
	msec_ = order.msecs;

	signed price = order.price;
	unsigned size = order.size;
	unsigned bid_ask = (order.type >> 1) & (unsigned)1; // 0:bid, 1:ask
	unsigned add_del = order.type & (unsigned)1; // 0: add, 1:del

	if( 0 == bid_ask ) // bid
	{
		if( 0 == add_del ) // add
		{
			bidvol[price] += size;

			if( uncross_ && uncrossedAfterOpen_ && is_inverted() )
			{
				//bool trading = find_if(sessions_.begin(), sessions_.end(), bind2nd(inRange(), order.msecs)) != sessions_.end();
				bool trading = sessions_.isAfterOpenBeforeClose(order.msecs);
				if( trading )
				{
					while( is_inverted() && !askvol.empty() )
					{
						map<signed, unsigned>::iterator it = askvol.begin();
						askvol.erase(it);
					}
				}
			}
		}
		else if( 1 == add_del ) // del
		{
			unsigned thevol = bidvol[price];
			if( size > thevol )
			{
				if( verbose_ )
					cerr << " Book::update() volume at price " << price
						<< " becomes negative at " << msec_ << endl;
				if( exit_on_negative_size_ )
					exit(4);
				else
					ret = false;
			}
			if( thevol >= size )
				bidvol[price] = thevol - size;
			else
				bidvol[price] = 0;
		}
	}
	else if( 1 == bid_ask ) // ask
	{
		if( 0 == add_del ) // add
		{
			askvol[price] += size;

			if( uncross_ && uncrossedAfterOpen_ && is_inverted() )
			{
				//bool trading = find_if(sessions_.begin(), sessions_.end(), bind2nd(inRange(), order.msecs)) != sessions_.end();
				bool trading = sessions_.isAfterOpenBeforeClose(order.msecs);
				if( trading )
				{
					while( is_inverted() && !bidvol.empty() )
					{
						map<signed, unsigned>::reverse_iterator it = bidvol.rbegin();
						bidvol.erase((++it).base());
					}
				}
			}
		}
		else if( 1 == add_del ) // del
		{
			unsigned thevol = askvol[price];
			if( size > thevol )
			{
				if( verbose_ )
					cerr << " Book::update() volume at price " << price
						<< " becomes negative at " << msec_ << endl;
				if( exit_on_negative_size_ )
					exit(4);
				else
					ret = false;
			}
			if( thevol >= size )
				askvol[price] = thevol - size;
			else
				askvol[price] = 0;
		}
	}

	remove_zeros();

	if( !uncrossedAfterOpen_ )
	{
		//bool trading = find_if(sessions_.begin(), sessions_.end(), bind2nd(inRange(), order.msecs)) != sessions_.end();
		bool trading = sessions_.isAfterOpenBeforeClose(order.msecs);
		if( trading )
		{
			if( !is_inverted() )
				uncrossedAfterOpen_ = true;
		}
	}

	if( verbose_ && is_inverted() )
	{
		int ask = askvol.begin()->first;
		int bid = bidvol.rbegin()->first;
		cerr << " SpreadBook::update() " << msec_ << "\tbid "
			<< bid << " (" << bidvol[bid] << ") > ask "
			<< ask << " (" << askvol[ask] << ") " << endl;
	}

	return ret;
}

bool SpreadBook::negative_spread()
{
	if( !askvol.empty() && !bidvol.empty() )
	{
		int ask = askvol.begin()->first;
		int bid = bidvol.rbegin()->first;
		if( ask < bid )
			return true;
	}
	return false;
}

bool SpreadBook::positive_spread()
{
	if( !askvol.empty() && !bidvol.empty() )
	{
		int ask = askvol.begin()->first;
		int bid = bidvol.rbegin()->first;
		if( ask > bid )
			return true;
	}
	return false;
}

bool SpreadBook::is_inverted()
{
	bool inverted = false;
	if( !askvol.empty() && !bidvol.empty() )
	{
		int ask = askvol.begin()->first;
		int bid = bidvol.rbegin()->first;
		if( ask <= bid )
			inverted = true;
	}
	return inverted;
}

void SpreadBook::remove_zeros()
{
	// remove zero's
	for( map<signed, unsigned>::iterator it = bidvol.begin(); it != bidvol.end(); )
		if( it->second == 0 )
			bidvol.erase(it++);
		else
			++it;
	for( map<signed, unsigned>::iterator it = askvol.begin(); it != askvol.end(); )
		if( it->second == 0 )
			askvol.erase(it++);
		else
			++it;

	return;
}

bool SpreadBook::get_quote( QuoteInfo& quote, int lot )
{
	static QuoteInfo last_quote;

	quote.msecs = msec_;
	quote.quflags = 0;

	quote.ask = UINT_MAX;
	quote.askSize = 0;
	int accAskSize = 0;
	for( map<signed, unsigned>::iterator it = askvol.begin(); it != askvol.end(); ++it )
	{
		accAskSize += it->second;
		if( accAskSize >= lot )
		{
			float orderPrice = it->first;
			float realPrice = orderPrice / scale_;
			quote.ask = realPrice;
			//quote.ask = (double)it->first / scale_;
			quote.askSize = accAskSize / lot;
			break;
		}
	}

	quote.bid = 0;
	quote.bidSize = 0;
	int accBidSize = 0;
	for( map<signed, unsigned>::reverse_iterator it = bidvol.rbegin(); it != bidvol.rend(); ++it )
	{
		accBidSize += it->second;
		if( accBidSize >= lot )
		{
			float orderPrice = it->first;
			float realPrice = orderPrice / scale_;
			quote.bid = realPrice;
			//quote.bid = (double)it->first / scale_;
			quote.bidSize = accBidSize / lot;
			break;
		}
	}

	if( 0 == quote.askSize )
		quote.ask = UINT_MAX;
	if( 0 == quote.ask )
		quote.ask = UINT_MAX;
	if( 0 == quote.bidSize )
		quote.bid = 0;

	if( fabs(quote.ask - last_quote.ask) > ltmb_ || quote.askSize != last_quote.askSize ||
		fabs(quote.bid - last_quote.bid) > ltmb_ || quote.bidSize != last_quote.bidSize )
	{
		last_quote = quote;
		return true;
	}

	return false;
}

bool SpreadBook::get_quote_lot( QuoteInfo& quote )
{
	static QuoteInfo last_quote;

	quote.msecs = msec_;
	quote.quflags = 0;

	quote.ask = UINT_MAX;
	quote.askSize = 0;
	int accAskSize = 0;
	for( map<signed, unsigned>::iterator it = askvol.begin(); it != askvol.end(); ++it )
	{
		accAskSize += it->second;
		if( accAskSize >= 100 )
		{
			quote.ask = it->first / scale_;
			quote.askSize = accAskSize / 100;
			break;
		}
	}

	quote.bid = 0;
	quote.bidSize = 0;
	int accBidSize = 0;
	for( map<signed, unsigned>::reverse_iterator it = bidvol.rbegin(); it != bidvol.rend(); ++it )
	{
		accBidSize += it->second;
		if( accBidSize >= 100 )
		{
			quote.bid = it->first / scale_;
			quote.bidSize = accBidSize / 100;
			break;
		}
	}

	if( 0 == quote.askSize )
		quote.ask = UINT_MAX;
	if( 0 == quote.ask )
		quote.ask = UINT_MAX;
	if( 0 == quote.bidSize )
		quote.bid = 0;

	if( fabs(quote.ask - last_quote.ask) > ltmb_ || quote.askSize != last_quote.askSize ||
		fabs(quote.bid - last_quote.bid) > ltmb_ || quote.bidSize != last_quote.bidSize )
	{
		last_quote = quote;
		return true;
	}

	return false;
}

int SpreadBook::get_bid_depth()
{
	return bidvol.size();
}

int SpreadBook::get_ask_depth()
{
	return askvol.size();
}

map<signed, unsigned> SpreadBook::get_bidvol()
{
	return bidvol;
}

map<signed, unsigned> SpreadBook::get_askvol()
{
	return askvol;
}

/*
void SpreadBook::get_auction(double& price, int& size)
{
	price = 0.;
	size = 0;
	if( is_inverted() )
	{
		set<signed> allPrice;

		map<signed, unsigned> cumBidvol;
		unsigned bidsum = 0;
		for( map<signed, unsigned>::reverse_iterator it = bidvol.rbegin(); it != bidvol.rend(); ++it )
		{
			bidsum += it->second;
			cumBidvol[it->first] = bidsum;
			allPrice.insert(it->first);
		}

		map<signed, unsigned> cumAskvol;
		unsigned asksum = 0;
		for( map<signed, unsigned>::iterator it = askvol.begin(); it != askvol.end(); ++it )
		{
			asksum += it->second;
			cumAskvol[it->first] = asksum;
			allPrice.insert(it->first);
		}

		signed auction_price = 0;
		unsigned auction_size = 0;
		signed surplus = 0;
		//for( map<unsigned, unsigned>::iterator it = cumAskvol.begin(); it != cumAskvol.end(); ++it )
		for( set<signed>::iterator it = allPrice.begin(); it != allPrice.end(); ++it )
		{
			signed this_price = *it;

			signed tradable_ask = 0;
			map<signed, unsigned>::iterator ita = cumAskvol.begin();
			map<signed, unsigned>::iterator itaEnd = cumAskvol.end();
			for( ; ita != itaEnd; ++ita )
			{
				if( ita->first <= this_price )
					tradable_ask = ita->second;
				else
					break;
			}

			map<signed, unsigned>::iterator itb = cumBidvol.lower_bound(this_price);
			signed tradable_bid = 0;
			if( itb != cumBidvol.end() )
				tradable_bid = itb->second;

			signed this_tradable = min(tradable_ask, tradable_bid);
			signed this_surplus = max(tradable_ask, tradable_bid) - this_tradable;
			if( this_tradable > auction_size || (this_tradable == auction_size && this_surplus < surplus) )
			{
				auction_price = this_price;
				auction_size = this_tradable;
				surplus = this_surplus;
			}
			else if( this_tradable < auction_size || (this_tradable != 0 && this_tradable == auction_size && this_surplus > surplus) )
				break;
		}
		price = auction_price / 10000.;
		size = auction_size;
	}
}
*/

vector<OrderDataSpread> SpreadBook::get_book_diff( SpreadBook& newBook, unsigned msecs )
{
	remove_zeros();
	vector<OrderDataSpread> vod;

	for( map<signed, unsigned>::reverse_iterator it = bidvol.rbegin(); it != bidvol.rend(); ++it )
	{
		signed bid = it->first;
		unsigned bidSize = it->second;
		unsigned newBidSize = newBook.bidvol[bid];
		if( bidSize > newBidSize )
		{
			OrderDataSpread ord;
			ord.type = 1; // bid, del
			ord.msecs = msecs;
			ord.size = bidSize - newBidSize;
			ord.price = bid;
			vod.push_back(ord);
		}
		else if( bidSize < newBidSize )
		{
			OrderDataSpread ord;
			ord.type = 0; // bid, add
			ord.msecs = msecs;
			ord.size = newBidSize - bidSize;
			ord.price = bid;
			vod.push_back(ord);
		}
	}

	for( map<signed, unsigned>::iterator it = askvol.begin(); it != askvol.end(); ++it )
	{
		signed ask = it->first;
		unsigned askSize = it->second;
		unsigned newAskSize = newBook.askvol[ask];
		if( askSize > newAskSize )
		{
			OrderDataSpread ord;
			ord.type = 3; // ask, del
			ord.msecs = msecs;
			ord.size = askSize - newAskSize;
			ord.price = ask;
			vod.push_back(ord);
		}
		else if( askSize < newAskSize )
		{
			OrderDataSpread ord;
			ord.type = 2; // ask, add
			ord.msecs = msecs;
			ord.size = newAskSize - askSize;
			ord.price = ask;
			vod.push_back(ord);
		}
	}

	for( map<signed, unsigned>::reverse_iterator it = newBook.bidvol.rbegin();
		it != newBook.bidvol.rend(); ++it )
	{
		signed newBid = it->first;
		unsigned newBidSize = it->second;
		unsigned bidSize = bidvol[newBid];
		if( bidSize == 0 )
		{
			OrderDataSpread ord;
			ord.type = 0; // bid, add
			ord.msecs = msecs;
			ord.size = newBidSize;
			ord.price = newBid;
			vod.push_back(ord);
		}
	}

	for( map<signed, unsigned>::reverse_iterator it = newBook.askvol.rbegin();
		it != newBook.askvol.rend(); ++it )
	{
		signed newAsk = it->first;
		unsigned newAskSize = it->second;
		unsigned askSize = askvol[newAsk];
		if( askSize == 0 )
		{
			OrderDataSpread ord;
			ord.type = 2; // ask, add
			ord.msecs = msecs;
			ord.size = newAskSize;
			ord.price = newAsk;
			vod.push_back(ord);
		}
	}

	remove_zeros();
	newBook.remove_zeros();
	return vod;
}

bool SpreadBook::existUncrossingOrder()
{
	return !uncrossingOrders_.empty();
}

vector<OrderDataSpread> SpreadBook::getUncrossingOrders()
{
	vector<OrderDataSpread> ret = uncrossingOrders_;
	uncrossingOrders_.clear();
	return ret;
}

ostream& operator <<(ostream& os, SpreadBook& obj)
{
	if( obj.is_inverted() )
		os << "***";
	else
		os << "---";
	os << " bid ---" << endl;
	for( map<signed, unsigned>::const_iterator it = obj.bidvol.begin(); it != obj.bidvol.end(); ++it )
		os << it->first << ":" << it->second << "\t";
	os << endl;

	os << "--- ask ---" << endl;
	for( map<signed, unsigned>::const_iterator it = obj.askvol.begin(); it != obj.askvol.end(); ++it )
		os << it->first << ":" << it->second << "\t";
	os << endl;

	return os;
}

ostream& operator <<(ostream& os, const OrderDataSpread& obj)
{
	os << obj.msecs << " " << obj.price << " " << obj.size << " " << obj.type;
	return os;
}
