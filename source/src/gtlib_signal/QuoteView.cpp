#include <gtlib_signal/QuoteView.h>
#include <gtlib_model/mFtns.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/jlutil_tickdata.h>
#include <numeric>
using namespace std;

namespace gtlib {

QuoteView::QuoteView(const vector<QuoteInfo>* quotes, const vector<TradeInfo>* trades, const Sessions& sessions,
		int openMsecs, int closeMsecs, int interval, bool reqVal, bool fillZeros, bool allowCross)
	:reqVal_(reqVal),
	fillZeros_(fillZeros),
	allowCross_(allowCross),
	interval_(interval),
	openMsecs_(openMsecs),
	closeMsecs_(closeMsecs),
	prevIndex_(-1),
	quotes_(quotes),
	trades_(trades),
	nTrades_(trades->size()),
	tIndx_(0),
	curMsecs_(-1),
	timeFrac_(0.),
	high_(0.),
	low_(0.),
	dv_(0.),
	share_(0.)
{
	nSample_ = (closeMsecs_ - openMsecs_) / interval_;
	vMid_ = vector<float>(nSample_, 0.);
	vShare_ = vector<int>(nSample_, 0);
	if(trades_ != nullptr) {
		for( auto t : (*trades_) )
		{
			if( sessions.isAfterOpenBeforeClose(t.msecs) )
				addTrade(t);
		}
	}
	if(quotes_ != nullptr) {
		for( auto q : (*quotes_) )
		{
			if( sessions.isAfterOpenBeforeClose(q.msecs) )
				addQuote(q);
		}
	}
	endTicker();
}

void QuoteView::addQuote(const QuoteInfo& quote)
{
	int index = (quote.msecs - openMsecs_) / interval_;
	if( index >= 0 && index < nSample_ )
	{
		while( prevIndex_ < index )
		{
			++prevIndex_;
			vMid_[prevIndex_] = (prevIndex_ > 0) ? vMid_[prevIndex_ - 1] : 0.;
		}
		bool okCross = allowCross_ || quote.ask > quote.bid;
		if( okCross && (!reqVal_ || valid_quote(quote)) )
			vMid_[index] = get_mid(quote);
		else
		{
			if( index > 0 && fillZeros_ )
				vMid_[index] = vMid_[index - 1];
			else
				vMid_[index] = 0.;
		}
	}
}

void QuoteView::addTrade(const TradeInfo& trade)
{
	int index = (trade.msecs - openMsecs_) / interval_;
	if( index >= 0 && index < nSample_ )
		vShare_[index] += trade.qty;
}

void QuoteView::endTicker()
{
	while( prevIndex_ < nSample_ )
	{
		++prevIndex_;
		vMid_[prevIndex_] = (prevIndex_ > 0) ? vMid_[prevIndex_ - 1] : 0.;
	}
}

float QuoteView::getMid(int msecs) const
{
	int index = (msecs - openMsecs_) / interval_ - 1; // Index for lookup should be one less thatn the index for adding.
	if( index >= 0 && index < nSample_ )
		return vMid_[index];
	else if( index >= nSample_ )
		return vMid_[nSample_ - 1];
	return 0.;
}

float QuoteView::getMidExact(int msecs) const
{
	if(quotes_ == nullptr)
		return 0.;

	QuoteInfo qtemp;
	qtemp.msecs = msecs;
	auto it = upper_bound(quotes_->begin(), quotes_->end(), qtemp,
			[](const QuoteInfo& l, const QuoteInfo& r){return l.msecs < r.msecs;});

	if(it != quotes_->begin())
		advance(it, -1);
	return get_mid(*it);
}

float QuoteView::getHiLo(int msecs)
{
	advanceTrade(msecs);
	double logHiLo = (low_ > 0. && high_ > low_) ? log(high_ / low_) : 0.;
	return sqrt(timeFrac_) * logHiLo * logHiLo;
}

float QuoteView::getDv(int msecs)
{
	advanceTrade(msecs);
	return timeFrac_ * dv_;
}

float QuoteView::getShare(int msecs)
{
	advanceTrade(msecs);
	return timeFrac_ * share_;
}

void QuoteView::advanceTrade(int msecs)
{
	if(curMsecs_ > 0 && msecs >= curMsecs_)
	{
		tIndx_ = 0;
		curMsecs_ = -1;
		timeFrac_ = 0.;
		high_ = 0.;
		low_ = 0.;
		dv_ = 0.;
		share_ = 0.;
	}
	while(tIndx_ < nTrades_ - 1)
	{
		const TradeInfo& trade = (*trades_)[tIndx_];
		int tmsecs = trade.msecs;
		if(tmsecs < msecs)
		{
			timeFrac_ = (msecs > openMsecs_) ? ((double)closeMsecs_ - openMsecs_) / (msecs - openMsecs_) : 0.;
			high_ = max(high_, trade.price);
			low_ = low_ == 0. ? trade.price : min(low_, trade.price);
			dv_ += trade.qty * trade.price;
			share_ += trade.qty;
			++tIndx_;
		}
		else
			break;
	}
}

int QuoteView::getShare(int msecs, int lagSec)
{
	int index = (msecs - openMsecs_) / interval_;
	int index0 = max(0, index - lagSec * 1000 / interval_);
	int sumShare = accumulate(vShare_.begin() + index0, vShare_.begin() + index, 0);
	return sumShare;
}

} // namespace gtlib
