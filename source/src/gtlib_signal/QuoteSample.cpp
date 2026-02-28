#include <gtlib_signal/QuoteSample.h>
#include <gtlib_model/mFtns.h>
#include <jl_lib/jlutil.h>
using namespace std;

namespace gtlib {

QuoteSample::QuoteSample(int openMsecs, int closeMsecs, int interval, bool reqVal, bool fillZeros, bool allowCross)
	:reqVal_(reqVal),
	fillZeros_(fillZeros),
	allowCross_(allowCross),
	interval_(interval),
	openMsecs_(openMsecs),
	closeMsecs_(closeMsecs),
	prevIndex_(-1)
{
	nSample_ = (closeMsecs_ - openMsecs_) / interval_;
	vMid_ = vector<float>(nSample_, 0.);
}

QuoteSample::QuoteSample(int openMsecs, int closeMsecs)
	:QuoteSample(openMsecs, closeMsecs, 1000, true, false, true) // default setting for signal calculation.
{
}

QuoteSample::QuoteSample(const vector<QuoteInfo>& quotes, const Sessions& sessions,
		int openMsecs, int closeMsecs, int interval, bool reqVal, bool fillZeros, bool allowCross)
	:QuoteSample(openMsecs, closeMsecs, interval, reqVal, fillZeros, allowCross)
{
	for( auto q : quotes )
	{
		if( sessions.isAfterOpenBeforeClose(q.msecs) )
			addQuote(q);
	}
	endTicker();
}

QuoteSample::QuoteSample(const vector<QuoteDataMicro>& quotes, const Sessions& sessions,
		int openMsecs, int closeMsecs, int interval, bool reqVal, bool fillZeros, bool allowCross)
	:QuoteSample(openMsecs, closeMsecs, interval, reqVal, fillZeros, allowCross)
{
	for( auto q : quotes )
	{
		if( sessions.isAfterOpenBeforeClose(q.msecs/1000) )// usecs to msecs
			addQuote(q);
	}
	endTicker();
}

void QuoteSample::addQuote(const QuoteInfo& quote)
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

void QuoteSample::addQuote(const QuoteDataMicro& quote)
{
	QuoteInfo q = quote;
	addQuote(q);
}

void QuoteSample::endTicker()
{
	while( prevIndex_ < nSample_ )
	{
		++prevIndex_;
		vMid_[prevIndex_] = (prevIndex_ > 0) ? vMid_[prevIndex_ - 1] : 0.;
	}
}

float QuoteSample::getMid(int msecs) const
{
	int index = (msecs - openMsecs_) / interval_ - 1; // Index for lookup should be one less thatn the index for adding.
	if( index >= 0 && index < nSample_ )
		return vMid_[index];
	else if( index >= nSample_ )
		return vMid_[nSample_ - 1];
	return 0.;
}

} // namespace gtlib
