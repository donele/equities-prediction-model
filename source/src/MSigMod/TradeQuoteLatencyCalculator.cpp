#include <MSigMod/TradeQuoteLatencyCalculator.h>
#include <MFramework.h>
#include <gtlib_model/mFtns.h>
#include <gtlib_signal/sigFtns.h>
#include <jl_lib/TickSources.h>
#include <jl_lib/jlutil_tickdata.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <string>
using namespace std;
using namespace hff;
using namespace gtlib;

TradeQuoteLatencyCalculator::TradeQuoteLatencyCalculator()
	:debug_(false),
	pStatusChange_(nullptr)
{
}

TradeQuoteLatencyCalculator::TradeQuoteLatencyCalculator(int idate, const string& uid, const string& ticker,
		Sessions* sessions, int openMsecs, int closeMsecs)
	:debug_(false),
	curr_wgt_usd_(0.),
	curr_wgt_usd_local_(0.),
	pStatusChange_(nullptr),
	idate_(idate),
	uid_(uid),
	ticker_(ticker),
	openMsecs_(openMsecs),
	closeMsecs_(closeMsecs),
	n1sec_((closeMsecs - openMsecs) / 1000 + 1),
	cnt_(-1),
	msecsLastEvent_(0),
	firstQuote_(QuoteInfo()),
	firstTrade_(TradeInfo()),
	lastNbbo_(QuoteInfo()),
	lastTrade_(TradeInfo()),
	lastBookTrade_(TradeInfo()),
	pSessions_(sessions)
{
	curr_wgt_usd_ = mto::currWgtMerc(MEnv::Instance()->market);
	curr_wgt_usd_local_ = 1./mto::currWgtMerc(MEnv::Instance()->markets[0]);
}

TradeQuoteLatencyCalculator::~TradeQuoteLatencyCalculator()
{
}

void TradeQuoteLatencyCalculator::setDebug()
{
	debug_ = true;
}

void TradeQuoteLatencyCalculator::setStatusChange(StatusChangeUS* p)
{
	pStatusChange_ = p;
}

// -------------------------------------------------------------------------
//
// Callback functions.
//
// -------------------------------------------------------------------------

void TradeQuoteLatencyCalculator::NbboCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	//static SampleCounter counter(quoteSampleStep);
	if( !pSessions_->isAfterOpenBeforeClose(msecs) )
		return;
	if( pStatusChange_ != nullptr && pStatusChange_->isHalted(idate_, ticker_, msecs) )
		return;

	// Update last nbbo.
	QuoteInfo this_nbbo = provider->Nbbo();
	lastNbbo_ = this_nbbo;
	reset_level_changing_trade(msecs);

	// Last event time.
	msecsLastEvent_ = msecs;

	// First quote.
	if( msecs > openMsecs_ && (firstQuote_.msecs == 0 || firstQuote_.msecs == lastNbbo_.msecs) )
	{
		if( valid_quote(lastNbbo_) && lastNbbo_.ask >= lastNbbo_.bid )
		{
			firstQuote_ = lastNbbo_;
		}
	}
}

void TradeQuoteLatencyCalculator::TradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	TradeInfo this_trade = provider->LastTrade();
	if( !pSessions_->isAfterOpenBeforeClose(msecs) )
		return;
	if( pStatusChange_ != nullptr && pStatusChange_->isHalted(idate_, ticker_, msecs) )
		return;

	msecsLastEvent_ = msecs;

	if( valid_trade(this_trade, lastNbbo_))
	{
		// Update last trade.
		lastTrade_ = this_trade;
		update_trade(msecs);

		if( firstTrade_.msecs == 0 )
			firstTrade_ = this_trade;
	}
}

void TradeQuoteLatencyCalculator::BookTradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	TradeInfo this_trade = provider->LastBookTrade();
	if( !pSessions_->isAfterOpenBeforeClose(msecs) )
		return;
	if( pStatusChange_ != nullptr && pStatusChange_->isHalted(idate_, ticker_, msecs) )
		return;

	msecsLastEvent_ = msecs;

	if( valid_trade(this_trade, lastNbbo_))
	{
		lastBookTrade_ = this_trade;
	}
}

void TradeQuoteLatencyCalculator::endTicker()
{
}

void TradeQuoteLatencyCalculator::update_trade(int msecs)
{
	if(lastNbbo_.ask > lastNbbo_.bid + ltmb_)
	{
		if(abs(lastTrade_.price - lastNbbo_.bid) < ltmb_ && lastTrade_.qty == lastNbbo_.bidSize)
			bidTakingTrade_ = lastTrade_;
		else if(abs(lastTrade_.price - lastNbbo_.ask) < ltmb_ && lastTrade_.qty == lastNbbo_.askSize)
			askTakingTrade_ = lastTrade_;
	}
}

void TradeQuoteLatencyCalculator::reset_level_changing_trade(int msecs)
{
	if(bidTakingTrade_.msecs > 0)
		;
}
