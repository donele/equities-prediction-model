#include <MSigMod/SignalCalculatorTP.h>
#include <MFramework.h>
#include <gtlib_model/mFtns.h>
#include <gtlib_signal/sigFtns.h>
#include <jl_lib/GFee.h>
#include <jl_lib/TickSources.h>
#include <jl_lib/jlutil_tickdata.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <string>
using namespace std;
using namespace hff;
using namespace gtlib;

const bool debug_first_quote = false; // If true, get the first quote from nbbo tickdata. Use to compare to reg sample code.

SignalCalculatorTP::SignalCalculatorTP()
	:debug_(false),
	pStatusChange_(nullptr)
{
}

SignalCalculatorTP::SignalCalculatorTP(int idate, const string& uid, const string& ticker, SigC& sig,
		Sessions* sessions, int openMsecs, int closeMsecs, const vector<hff::OrderQty>* tradeQty, int exploratoryDelay)
	:debug_(false),
	discardOutliers_(true),
	dropBadTarget_(false),
	sample_block_trade_(false),
	invalid_trade_appeared_(false),
	persistAna_(false),
	quoteUpdatePending_(false),
	doSamplePendingUpdate_(true),
	requireTradeAfterFirstQuote_(true),
	rtrd_simple_def_(false),
	debugNBookTrade_(0),
	debugNAllTaken_(0),
	curr_wgt_usd_(0.),
	curr_wgt_usd_local_(0.),
	quoteSampleStep_(0),
	pStatusChange_(nullptr),
	idate_(idate),
	tradeSampleStep_(0),
	booktradeSampleStep_(0),
	doExploratory(false),
	uid_(uid),
	ticker_(ticker),
	openMsecs_(openMsecs),
	closeMsecs_(closeMsecs),
	exploratoryDelay_(exploratoryDelay),
	n1sec_((closeMsecs - openMsecs) / 1000 + 1),
	cnt_(-1),
	openDelay_(0),
	msecsLastEvent_(0),
	validt_(0),
	lastFillImb_(0),
	lastFillImbOld_(0),
	high_(0.),
	low_(max_float_),
	high900_(0.),
	low900_(max_float_),
	f600_(0.),
	f3600_(0.),
	miNormFac_(0.),
	ret_on_(0.),
	adjPrevClose_(sig.adjPrevClose),
	tradeQty_(tradeQty),
	qSample_(openMsecs, closeMsecs),
	firstQuote_(QuoteInfo()),
	firstTrade_(TradeInfo()),
	lastNbbo_(QuoteInfo()),
	lastTrade_(TradeInfo()),
	lastBookTrade_(TradeInfo()),
	psig_(&sig),
	psigh_(0),
	pSessions_(sessions)
{
	f600_ = float(n1sec_ - 1) / 600;
	f3600_ = float(n1sec_ - 1) / 3600;
	curr_wgt_usd_ = mto::currWgtMerc(MEnv::Instance()->market);
	curr_wgt_usd_local_ = 1./mto::currWgtMerc(MEnv::Instance()->markets[0]);

	vector<int> tradeVol(n1sec_, 0);
	miTradeVolCum_ = vector<int>(n1sec_, 0);
	if( tradeQty_ != nullptr && !tradeQty_->empty() )
	{
		for( vector<hff::OrderQty>::const_iterator it = tradeQty_->begin(); it != tradeQty_->end(); ++it )
		{
			int bin = (it->msecs - openMsecs - 1) / 1000 + 1;
			if( bin >= 0 && bin < n1sec_ )
				tradeVol[bin] += it->signedQty;
		}
		get_cum(miTradeVolCum_, tradeVol);
	}
	miNormFac_ = (sig.avgDlyVolume > 0) ? sig.avgDlyVolume : 1.0;
}

SignalCalculatorTP::~SignalCalculatorTP()
{
}

void SignalCalculatorTP::setDebug()
{
	debug_ = true;
}

void SignalCalculatorTP::setDiscardOutliers(bool tf)
{
	discardOutliers_ = tf;
}

void SignalCalculatorTP::setDropBadTarget(bool tf)
{
	dropBadTarget_ = tf;
}

void SignalCalculatorTP::setRtrdSimpleDef(bool tf)
{
	rtrd_simple_def_ = tf;
}

void SignalCalculatorTP::setStatusChange(StatusChangeUS* p)
{
	pStatusChange_ = p;
}

void SignalCalculatorTP::setOpenDelay(int delay)
{
	openDelay_ = delay;
}

void SignalCalculatorTP::setQuoteSampleStep(int step)
{
	quoteSampleStep_ = step;
	nbboCounter_ = SampleCounter(quoteSampleStep_);
}

void SignalCalculatorTP::setTradeSampleStep(int step)
{
	tradeSampleStep_ = step;
	tradeCounter_ = SampleCounter(tradeSampleStep_);
}

void SignalCalculatorTP::setBooktradeSampleStep(int step)
{
	booktradeSampleStep_ = step;
	booktradeCounter_ = SampleCounter(booktradeSampleStep_);
}

void SignalCalculatorTP::sampleBlockTrade()
{
	sample_block_trade_ = true;
}

void SignalCalculatorTP::excludeBlockTrade()
{
	exclude_block_trade_ = true;
}

const vector<QuoteInfo>& SignalCalculatorTP::getQuotes() const
{
	return quotes_;
}

const TradeInfo& SignalCalculatorTP::getFirstTrade() const
{
	return firstTrade_;
}

const QuoteInfo& SignalCalculatorTP::getFirstQuote() const
{
	return firstQuote_;
}

void SignalCalculatorTP::endTicker()
{
	qSample_.endTicker();
}

void SignalCalculatorTP::doPersistAna()
{
	persistAna_ = true;
}

void SignalCalculatorTP::samplePendingUpdate(bool tf)
{
	doSamplePendingUpdate_ = tf;
}

void SignalCalculatorTP::requireTradeAfterFirstQuote(bool tf)
{
	requireTradeAfterFirstQuote_ = tf;
}

void SignalCalculatorTP::addQuote(const QuoteInfo& quote)
{
	quotes_.push_back(quote);
	qSample_.addQuote(quote);
	lastBookTradeBeforeNbbo_ = lastBookTrade_;
}

void SignalCalculatorTP::addTrade(const TradeInfo& trade)
{
	trades_.push_back(trade);

	lastFillImbOld_ = 2. * (trade.price - get_mid(lastNbbo_)) / (lastNbbo_.ask - lastNbbo_.bid);

	// Update high and low.
	if( trade.price > high_ )
		high_ = trade.price;
	if( trade.price < low_ )
		low_ = trade.price;

	int msecs_15min = 900000 + openMsecs_;
	if( trade.msecs < msecs_15min )
	{
		if( trade.price > high900_ )
			high900_ = trade.price;
		if( trade.price < low900_ )
			low900_ = trade.price;
	}
}

void SignalCalculatorTP::addBookTrade(const TradeInfo& trade)
{
	// Update sd(trade price)
	accTrd_.add(trade.price);

	// Update last book trade.
	//if(lastNbbo_.msecs > nbboBeforeLastBookTrade_.msecs)
		//nbboBeforeLastBookTradeWithLessTime_ = nbboBeforeLastBookTrade_;
	nbboBeforeLastBookTrade_ = lastNbbo_;
	nbboBeforeLastBookTradeWithLessTime_ = lastNbboWithLessTime_;
}

void SignalCalculatorTP::setTmSampleMsecs(const vector<int>& sampleMsecs, const int scale)
{
	tmSampleMsecs_.clear();
	int N = sampleMsecs.size();
	for( int i = 0; i < N; ++i )
	{
		if( scale < 1 || (i + 1) % scale == 0 ) // To select 300, 600, etc.
			tmSampleMsecs_.insert(sampleMsecs[i]);
	}
}

// -------------------------------------------------------------------------
//
// Callback functions.
//
// -------------------------------------------------------------------------

void SignalCalculatorTP::NbboCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	quoteUpdatePending_ = false;

	//static SampleCounter counter(quoteSampleStep);
	nbboCounter_.update();
	if( !pSessions_->isAfterOpenBeforeClose(msecs) )
		return;
	if( pStatusChange_ != nullptr && pStatusChange_->isHalted(idate_, ticker_, msecs) )
		return;

	// Update last nbbo.
	QuoteInfo this_nbbo = provider->Nbbo();
	if(debug_)
		printf("%d %8d %10.4f %10.4f%8d \n", msecs, this_nbbo.bidSize, this_nbbo.bid, this_nbbo.ask, this_nbbo.askSize);
	if(this_nbbo.msecs > lastNbbo_.msecs)
		lastNbboWithLessTime_ = lastNbbo_;
	lastNbbo_ = this_nbbo;
	addQuote(this_nbbo);

	// Last event time.
	msecsLastEvent_ = msecs;

	// First quote.
	if( msecs > openMsecs_ && (firstQuote_.msecs == 0 || firstQuote_.msecs == lastNbbo_.msecs) )
	{
		if( valid_quote(lastNbbo_) && lastNbbo_.ask >= lastNbbo_.bid )
		{
			firstQuote_ = lastNbbo_;
			psig_->sprdOpen = get_sprd(firstQuote_.bid, firstQuote_.ask) * basis_pts_;
		}
	}

	if( adjPrevClose_ > 0. && msecs < sec_on_ * 1000 + openMsecs_ )
	{
		if( valid_quote(lastNbbo_) )
		{
			float mid = get_mid(lastNbbo_);
			if(mid > 0.)
				ret_on_ = basis_pts_ * (mid / adjPrevClose_ - 1.);
		}
		else
			ret_on_ = 0.;
	}

	if( nbboCounter_.doSample() )
		NewSig(msecs, provider, nbbo_event_);
}

void SignalCalculatorTP::TradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	TradeInfo this_trade = provider->LastTrade();
	if( !pSessions_->isAfterOpenBeforeClose(msecs) )
		return;
	if( pStatusChange_ != nullptr && pStatusChange_->isHalted(idate_, ticker_, msecs) )
		return;

	// EU trade test
	if(isBlockTrade(this_trade, true))
	{
		invalid_trade_appeared_ = true;
		if(exclude_block_trade_)
			return;
	}

	tradeCounter_.update();
	msecsLastEvent_ = msecs;

	if( valid_trade(this_trade, lastNbbo_))
	{
		// Update last trade.
		lastTrade_ = this_trade;

		addTrade(this_trade);
		if( firstTrade_.msecs == 0 )
			firstTrade_ = this_trade;

		// Seen a trade after valid quote
		if(!validt_ && firstQuote_.msecs > 0)
			validt_ = 1;

		// KR trade quote latency test
		if(fabs(lastTrade_.price - lastNbbo_.bid) < ltmb_ && lastTrade_.qty == lastNbbo_.bidSize)
			quoteUpdatePending_ = true;
		else if(fabs(lastTrade_.price - lastNbbo_.ask) < ltmb_ && lastTrade_.qty == lastNbbo_.askSize)
			quoteUpdatePending_ = true;

		// Trade within bid and ask
		bool valid_quote_trade = lastTrade_.price + 1e-6 >= lastNbbo_.bid && lastTrade_.price - 1e-6 <= lastNbbo_.ask;
		if( valid_quote_trade && tradeCounter_.doSample() )
			NewSig(msecs, provider, trade_event_);
	}
}

bool SignalCalculatorTP::isBlockTrade(const TradeInfo& trade, bool record)
{
	bool is_block_trade = false;
	float notional = curr_wgt_usd_ * trade.price * trade.qty * curr_wgt_usd_local_;
	if(notional > 500000)
		is_block_trade = true;
	if(is_block_trade && record)
		mBlockTrade_[trade.msecs] += notional;
	return is_block_trade;
}

void SignalCalculatorTP::BookTradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	TradeInfo this_trade = provider->LastBookTrade();
	if( !pSessions_->isAfterOpenBeforeClose(msecs) )
		return;
	if( pStatusChange_ != nullptr && pStatusChange_->isHalted(idate_, ticker_, msecs) )
		return;

	//// EU trade test
	if(isBlockTrade(this_trade, false))
	{
		invalid_trade_appeared_ = true;
		if(exclude_block_trade_)
			return;
	}

	booktradeCounter_.update();
	msecsLastEvent_ = msecs;

	if( valid_trade(this_trade, lastNbbo_))
	{
		lastBookTrade_ = this_trade;
		addBookTrade(this_trade);
		lastFillImb_ = get_fillImb(provider);

		// Seen a trade after valid quote.
		if( !validt_ && firstQuote_.msecs > 0 )
			validt_ = 1;

		bool newsig_ok = false;
		if( booktradeCounter_.doSample() )
			newsig_ok = NewSig(msecs, provider, book_trade_event_);

		if( doExploratory && newsig_ok && mTrades_.find(msecs) == mTrades_.end() )
		{
			this_trade.flags = psig_->sI.rbegin()->sig1[1]; // Embed fillImb in TradeInfo.flags.

			// Record of trades to be used by exploratory signal calculation.
			mTrades_.insert(make_pair(msecs, this_trade));

			// Record of composite book.
			QuoteInfo compBkTop;
			get_top_comp_book(compBkTop, provider);
			mCompBkTop_.insert(make_pair(msecs, compBkTop));

			// Set time for exploratory events.
			provider->SetTimeCB( msecs + exploratoryDelay_, (void*)cb_ref_eevt_ );
		}
	}
}

void SignalCalculatorTP::RegularCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	if( !pSessions_->isAfterOpenBeforeClose(msecs) )
		return;
	if( pStatusChange_ != nullptr && pStatusChange_->isHalted(idate_, ticker_, msecs) )
		return;

	NewSig(msecs, provider, regular_sample_);
}

void SignalCalculatorTP::TimeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, void* ref)
{
	long long iRef0 = reinterpret_cast<long long>(ref);
	int iRef = static_cast<int>(iRef0);

	if( iRef >= 0 )
	{
		StateInfo& ti = psig_->sI[iRef];

		// Check transient.
		{
			QuoteInfo nbbo = provider->Nbbo();

			if ( nbbo.bid < ti.bid - 0.0001 )
				ti.bidPersists = 0;

			if ( nbbo.ask > ti.ask + 0.0001 )
				ti.askPersists = 0;
		}
	}
	else if( iRef == cb_ref_nevt_ )
	{
		if( !pSessions_->isAfterOpenBeforeClose(msecs) )
			return;

		NewSig(msecs, provider, news_event_);
	}
	else if( iRef == cb_ref_eevt_ ) // exploratory event.
	{
		if( !pSessions_->isAfterOpenBeforeClose(msecs) )
			return;

		NewSig(msecs, provider, exploratory_event_);
	}
}

// -------------------------------------------------------------------------
//
// Signal calculation.
//
// -------------------------------------------------------------------------

bool SignalCalculatorTP::NewSig(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, int sampleType)
{
	bool allOK = true;
	if(!doSamplePendingUpdate_ && quoteUpdatePending_)
		//return false;
		allOK = false;
	if(requireTradeAfterFirstQuote_ && !validt_)
		//return false;
		allOK = false;
	if(firstQuote_.msecs <= 0 || !valid_quote(lastNbbo_) || msecs <= firstQuote_.msecs || !pSessions_->inSessionStrictWait(msecs, openDelay_))
		//return false;
		allOK = false;
	if(pStatusChange_ != nullptr && pStatusChange_->isHalted(idate_, ticker_, msecs))
		//return false;
		allOK = false;
	if(sample_block_trade_ && !invalid_trade_appeared_)
		//return false;
		allOK = false;
 
	// check crossed.
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	if( !linMod.allowCross && lastNbbo_.ask - lastNbbo_.bid < min_tick_default_ )
		//return false;
		allOK = false;

	hff::SigC& sig = *psig_;
	StateInfo sI(linMod, nonLinMod);

	int msso = msecs - openMsecs_;
	sI.msso = msso;
	sI.sampleType = sampleType;
	if( sampleType & book_trade_event_ || sampleType & exploratory_event_ )
		sI.sampleType = sampleType + trade_and_exploratory_event_;
	sI.mqut = get_mid(lastNbbo_);
	sI.bid = lastNbbo_.bid;
	sI.ask = lastNbbo_.ask;

	calculate_trade_signals(sig, sI, msecs);
	calculate_quote_signals(sig, sI, msecs, provider);
	calculate_fillimb_signals(sI, msecs, provider, sampleType);
	calculate_tob_signals(sI, provider);
	calculate_book_signals(sig, sI, provider);
	calculate_atf_signals(sI, provider);
	calculate_bookcnt_signals(sig, sI, provider);
	calculate_mi_signals(sig, sI, provider);
	calculate_signals(sig, sI, msecs);

	if( discardOutliers_ && fabs(sI.mret60) > om_max_ret_ )
		//return false;
		allOK = false;
	if( sampleType & book_trade_event_ || sampleType & exploratory_event_ )
	{
		if( !linMod.use_us_arca_trade_event && string(1, lastBookTrade_.flags) == "P" )
			//return false;
			allOK = false;
	}

	// Update linear model and write binary signal files if gptOK.
	sI.gptOK = allOK ? 1 : 0;
	if( tmSampleMsecs_.count(msecs) )
		sI.isTmSample = true;

	sig.sI.push_back(sI);

	if(linMod.transientMsecs > 0)
		provider->SetTimeCB( msecs + linMod.transientMsecs, (void*)(sig.sI.size() - 1) );
	if(persistAna_)
	{
		provider->SetTimeCB( msecs + 1, (void*)(sig.sI.size() - 1) );
		provider->SetTimeCB( msecs + 2, (void*)(sig.sI.size() - 1) );
		provider->SetTimeCB( msecs + 5, (void*)(sig.sI.size() - 1) );
		provider->SetTimeCB( msecs + 10, (void*)(sig.sI.size() - 1) );
		provider->SetTimeCB( msecs + 20, (void*)(sig.sI.size() - 1) );
		provider->SetTimeCB( msecs + 50, (void*)(sig.sI.size() - 1) );
		provider->SetTimeCB( msecs + 100, (void*)(sig.sI.size() - 1) );
		provider->SetTimeCB( msecs + 200, (void*)(sig.sI.size() - 1) );
		provider->SetTimeCB( msecs + 500, (void*)(sig.sI.size() - 1) );
		provider->SetTimeCB( msecs + 1000, (void*)(sig.sI.size() - 1) );
	}
	return true;
}

void SignalCalculatorTP::calculate_trade_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs)
{
	int sec = sI.msso / 1000;

	if(lastBookTrade_.price > 0.)
	{
		double rtrd_denom = lastBookTradeBeforeNbbo_.price;
		double rtrd = (rtrd_denom > ltmb_) ? sI.mqut / rtrd_denom - 1. : 0.;
		double rCurTrd_denom = lastBookTrade_.price;
		double rCurTrd = (rCurTrd_denom > ltmb_) ? sI.mqut / rCurTrd_denom - 1. : 0.;

		sI.rPastTrd = rtrd;
		if(lastBookTrade_.msecs > lastBookTradeBeforeNbbo_.msecs)
			sI.rCurTrd = rCurTrd;

		if(rtrd_simple_def_)
			sI.rtrd = rCurTrd;
		else
			sI.rtrd = rtrd;

		sI.rtrdOverSprd = rtrd / sig.medSprd;
		if(lastBookTrade_.msecs > msecs - 60*1000)
			sI.rtrd60 = rtrd;
		if(lastBookTrade_.msecs > msecs - 120*1000)
			sI.rtrd120 = rtrd;
		if(lastBookTrade_.msecs > msecs - 3600*1000)
			sI.rtrd3600 = rtrd;
		if(lastBookTrade_.msecs > msecs - 7200*1000)
			sI.rtrd7200 = rtrd;
	}
	QuoteInfo& priorQuote = (lastBookTrade_.msecs > nbboBeforeLastBookTrade_.msecs) ? nbboBeforeLastBookTrade_: nbboBeforeLastBookTradeWithLessTime_;
	if(valid_quote(priorQuote))
	{
		float priorMid = get_mid(priorQuote);
		double mrtrd = (priorMid > ltmb_) ? sI.mqut / priorMid - 1. : 0.;
		if(priorMid > 0.)
			sI.mrtrd = mrtrd;
		if(lastBookTrade_.msecs > msecs - 60*1000)
			sI.mrtrd60 = mrtrd;
		if(lastBookTrade_.msecs > msecs - 120*1000)
			sI.mrtrd120 = mrtrd;
		if(lastBookTrade_.msecs > msecs - 3600*1000)
			sI.mrtrd3600 = mrtrd;
		if(lastBookTrade_.msecs > msecs - 7200*1000)
			sI.mrtrd7200 = mrtrd;
	}

	// realativeHiLo.
	if( low_ > 0. && high_ > 0. && low_ <= high_ )
	{
		double hiLoD = basis_pts_ * (high_ - low_) / (high_ + low_);
		if( hiLoD > min_hiloD_ )
			sI.relativeHiLo = (2. * lastTrade_.price - high_ - low_) / (high_ - low_);
	}

	// hiloLag1.
	if( sig.loLag1 > 0. && sig.hiLag1 > 0. && sig.hiLag1 >= sig.loLag1 )
	{
		double hiLoDL1 = basis_pts_ * (sig.hiLag1 - sig.loLag1) / (sig.hiLag1 + sig.loLag1);
		if( hiLoDL1 > min_hiloD_ )
			sI.hiloLag1 = (2. * lastTrade_.price - sig.hiLag1 - sig.loLag1) / (sig.hiLag1 - sig.loLag1);
	}

	// hiloLag2.
	if( sig.loLag2 > 0. && sig.hiLag2 > 0. && sig.hiLag1 >= sig.loLag1 )
	{
		double hiLoDL2 = basis_pts_ * (sig.hiLag2 - sig.loLag2) / (sig.hiLag2 + sig.loLag2);
		if( hiLoDL2 > min_hiloD_ )
			sI.hiloLag2 = (2. * lastTrade_.price - sig.hiLag2 - sig.loLag2) / (sig.hiLag2 - sig.loLag2);
	}

	// hilo900.
	if( sec >= 900 && low900_ > 0. && high900_ > 0. && high900_ >= low900_ )
	{
		double hiLoD900 = basis_pts_ * (high900_ - low900_) / (high900_ + low900_);
		if( hiLoD900 > min_hiloD_ )
			sI.hilo900 = (2. * lastTrade_.price - high900_ - low900_) / (high900_ - low900_);
	}

	// vm and vwap.
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	sI.voluMom15 = get_voluMom(openMsecs_, msecs, 15, trades_, sig.avgDlyVolume, n1sec_);
	sI.voluMom30 = get_voluMom(openMsecs_, msecs, 30, trades_, sig.avgDlyVolume, n1sec_);
	sI.voluMom60 = get_voluMom(openMsecs_, msecs, 60, trades_, sig.avgDlyVolume, n1sec_);
	sI.voluMom120 = get_voluMom(openMsecs_, msecs, 120, trades_, sig.avgDlyVolume, n1sec_);
	sI.voluMom300 = get_voluMom(openMsecs_, msecs, 300, trades_, sig.avgDlyVolume, n1sec_);
	sI.voluMom600 = get_voluMom(openMsecs_, msecs, 600, trades_, sig.avgDlyVolume, n1sec_);
	sI.voluMom3600 = get_voluMom(openMsecs_, msecs, 3600, trades_, sig.avgDlyVolume, n1sec_);
	sI.intraVoluMom = get_voluMom(openMsecs_, msecs, 0, trades_, sig.avgDlyVolume, n1sec_);

	sI.bollinger300 = get_vwap_sig(msecs, 300, trades_, 9);
	sI.bollinger900 = get_vwap_sig(msecs, 900, trades_, 9);
	sI.bollinger1800 = get_vwap_sig(msecs, 1800, trades_, 9);
	sI.bollinger3600 = get_vwap_sig(msecs, 3600, trades_, 9);

	sI.logBlockVolUsd60 = get_logBlockVol(msecs, 60, mBlockTrade_);
	sI.logBlockVolUsd3600 = get_logBlockVol(msecs, 3600, mBlockTrade_);
	sI.logBlockVolUsdIntra = get_logBlockVol(msecs, 0, mBlockTrade_);
}

double SignalCalculatorTP::get_mret_upto1s(int msecs,
		int length_msec, int firstQtMsec, double firstMidQt, double mqut, int lag)
{
	double mret = 0.;
	if( msecs > firstQtMsec )
	{
		int msecsFrom = msecs - length_msec - lag;
		double midFrom = 0.;
		if( msecsFrom <= firstQtMsec )
			midFrom = firstMidQt;
		else
			midFrom = qSample_.getMid(msecsFrom);

		double midTo = mqut;
		if( lag > 0 )
		{
			int msecsTo = msecs - lag;
			if( msecsTo <= firstQtMsec )
				midTo = firstMidQt;
			else
				midTo = qSample_.getMid(msecsTo);
		}

		if( midFrom > 0. && midTo > 0. )
			mret = basis_pts_ * (midTo / midFrom - 1.);
	}
	return mret;
}

double SignalCalculatorTP::get_mret_upto1s(int msecs,
		int length_msec, int firstQtMsec, double firstMidQt, double mqut,
		TickProvider<TradeInfo, QuoteInfo, OrderData>* provider, int lag)
{
	double mret = 0.;
	if( msecs > firstQtMsec )
	{
		double midFrom = 0.;
		int msecsFrom = msecs - length_msec - lag;
		if( msecsFrom <= firstQtMsec )
			midFrom = firstMidQt;
		else
		{
			QuoteInfo quoteFrom = provider->NbboAt(msecsFrom);
			if( valid_quote(quoteFrom) )
				midFrom = get_mid(quoteFrom);
		}

		double midTo = mqut;
		if( lag > 0 )
		{
			int msecsTo = msecs - lag;
			if( msecsTo <= firstQtMsec )
				midTo = firstMidQt;
			else
			{
				QuoteInfo quoteTo = provider->NbboAt(msecsTo);
				midTo = get_mid(quoteTo);
			}
		}

		if( midFrom > 0. && midTo > 0. )
			mret = basis_pts_ * (midTo / midFrom - 1.);
	}
	return mret;

}

void SignalCalculatorTP::calculate_quote_signals(hff::SigC& sig, hff::StateInfo& sI,
		int msecs, TickProvider<TradeInfo, QuoteInfo, OrderData>* provider)
{
	float firstMidQt = get_mid(firstQuote_);
	int sec = sI.msso / 1000;

	sI.volSclBsz = basis_pts_ * lastNbbo_.bidSize / sig.avgDlyVolume;
	sI.volSclAsz = basis_pts_ * lastNbbo_.askSize / sig.avgDlyVolume;

	sI.sprd = basis_pts_ * get_sprd(lastNbbo_.bid, lastNbbo_.ask);
	auto fee_bpt_bid_ask = GFee::Instance().feeBptBidAsk(MEnv::Instance()->model, ExecFeesPrimex(MEnv::Instance()->model, ticker_), lastNbbo_);
	sI.costBid = sI.sprd / 2. + fee_bpt_bid_ask[0];
	sI.costAsk = sI.sprd / 2. + fee_bpt_bid_ask[1];

	sI.mret1 = get_mret_upto1s(msecs, 1000, firstQuote_.msecs, firstMidQt, sI.mqut);
	sI.mret5 = get_mret_upto1s(msecs, 5000, firstQuote_.msecs, firstMidQt, sI.mqut);
	sI.mret15 = get_mret_upto1s(msecs, 15000, firstQuote_.msecs, firstMidQt, sI.mqut);
	sI.mret30 = get_mret_upto1s(msecs, 30000, firstQuote_.msecs, firstMidQt, sI.mqut);
	sI.mret60 = get_mret_upto1s(msecs, 60000, firstQuote_.msecs, firstMidQt, sI.mqut);

	sI.mret120 = get_mret_upto1s(msecs, 120000, firstQuote_.msecs, firstMidQt, sI.mqut);
	sI.mret300 = get_mret_upto1s(msecs, 300000, firstQuote_.msecs, firstMidQt, sI.mqut);
	sI.mret600 = get_mret_upto1s(msecs, 600000, firstQuote_.msecs, firstMidQt, sI.mqut);
	sI.mret1200 = get_mret_upto1s(msecs, 1200000, firstQuote_.msecs, firstMidQt, sI.mqut);
	sI.mret2400 = get_mret_upto1s(msecs, 2400000, firstQuote_.msecs, firstMidQt, sI.mqut);
	sI.mret4800 = get_mret_upto1s(msecs, 4800000, firstQuote_.msecs, firstMidQt, sI.mqut);
	sI.mret300L = get_mret_upto1s(msecs, 300000, firstQuote_.msecs, firstMidQt, sI.mqut, 300000);
	sI.mret600L = get_mret_upto1s(msecs, 600000, firstQuote_.msecs, firstMidQt, sI.mqut, 600000);
	sI.mret1200L = get_mret_upto1s(msecs, 1200000, firstQuote_.msecs, firstMidQt, sI.mqut, 1200000);
	sI.mret2400L = get_mret_upto1s(msecs, 2400000, firstQuote_.msecs, firstMidQt, sI.mqut, 2400000);
	sI.mret4800L = get_mret_upto1s(msecs, 4800000, firstQuote_.msecs, firstMidQt, sI.mqut, 4800000);

	float mret60L = get_mret_upto1s(msecs, 60000, firstQuote_.msecs, firstMidQt, sI.mqut, 60000);
	float mret120L = get_mret_upto1s(msecs, 120000, firstQuote_.msecs, firstMidQt, sI.mqut, 120000);

	sI.cwret30 = sI.mret30 * sig.logCap;
	sI.cwret60 = sI.mret60 * sig.logCap;
	sI.cwret120 = sI.mret120 * sig.logCap;
	sI.cwret300 = sI.mret300 * sig.logCap;
	sI.cwret600 = sI.mret600 * sig.logCap;
	sI.cwret300L = sI.mret300L * sig.logCap;

	sI.swret30 = sI.mret30 * sig.logShr;
	sI.swret60 = sI.mret60 * sig.logShr;
	sI.swret120 = sI.mret120 * sig.logShr;
	sI.swret300 = sI.mret300 * sig.logShr;
	sI.swret600 = sI.mret600 * sig.logShr;
	sI.swret300L = sI.mret300L * sig.logShr;

	// overnight return.
	sI.sig10[7] = ret_on_;
	if( fabs(sI.sig10[7]) > max_ret_)
		sI.sig10[7] = max_ret_ * sI.sig10[7] / fabs(sI.sig10[7]);

	// max sizes.
	sI.maxbsize = get_max_bidSize(msecs, 200, quotes_);
	sI.maxasize = get_max_askSize(msecs, 200, quotes_);
	sI.maxbsize2 = get_max_bidSize(msecs, 1200, quotes_);
	sI.maxasize2 = get_max_askSize(msecs, 1200, quotes_);

	sI.cwretONLag0 = ret_on_ * sig.logCap;
	sig.cwretONLag1 = sig.mretONLag1 * sig.logCap;
	sig.cwretIntraLag1 = sig.mretIntraLag1 * sig.logCap;
	sig.cwretIntraLag2 = sig.mretIntraLag2* sig.logCap;
	// sprd adjust.
	sig.mretIntraLag3Sprd = sig.mretIntraLag3 * sI.sprd;
	sig.mretIntraLag4Sprd = sig.mretIntraLag4 * sI.sprd;
	sig.mretIntraLag5Sprd = sig.mretIntraLag5 * sI.sprd;
	sig.hiloQAI3Sprd = sig.hiloQAI3 * sI.sprd;
	sig.hiloQAI4Sprd = sig.hiloQAI4 * sI.sprd;
	sig.hiloQAI5Sprd = sig.hiloQAI5 * sI.sprd;
	sig.hiloLag3OpenSprd = sig.hiloLag3Open * sI.sprd;
	sig.hiloLag4OpenSprd = sig.hiloLag4Open * sI.sprd;
	sig.hiloLag5OpenSprd = sig.hiloLag5Open * sI.sprd;
	sig.hiloLag3RatSprd = sig.hiloLag3Rat * sI.sprd;
	sig.hiloLag4RatSprd = sig.hiloLag4Rat * sI.sprd;
	sig.hiloLag5RatSprd = sig.hiloLag5Rat * sI.sprd;
	sig.mretONLag2Sprd = sig.mretONLag2 * sI.sprd;
	sig.mretONLag3Sprd = sig.mretONLag3 * sI.sprd;
	sig.mretONLag4Sprd = sig.mretONLag4 * sI.sprd;
}

void SignalCalculatorTP::calculate_fillimb_signals(hff::StateInfo& sI, int msecs,
		TickProvider<TradeInfo, QuoteInfo, OrderData>* provider, int sampleType)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	string model = MEnv::Instance()->model;

	QuoteInfo quote = provider->Bbo( (unsigned char)lastBookTrade_.flags );
	string market = string(1, lastBookTrade_.flags);

	// Fill imbalance.
	sI.sig1[1] = lastFillImb_;
	int vsize = vfi_.size();
	vfi_.push_back(lastFillImb_);
}

void SignalCalculatorTP::calculate_exploratory_signals(hff::StateInfo& sI, int msecs,
		TickProvider<TradeInfo, QuoteInfo, OrderData>* provider, int sampleType)
{
	if( sampleType & exploratory_event_ )
	{
		sI.isExp = 1.;

		map<int, TradeInfo>::iterator itt = mTrades_.find(msecs - exploratoryDelay_);
		map<int, QuoteInfo>::iterator itq = mCompBkTop_.find(msecs - exploratoryDelay_);
		if( itt != mTrades_.end() && itq != mCompBkTop_.end() )
		{
			TradeInfo& trade = itt->second;
			QuoteInfo& quoteFrom = itq->second;

			sI.sig1[1] = trade.flags; // Override fillImb.

			QuoteInfo quoteTo;
			get_top_comp_book(quoteTo, provider);

			if( valid_quote(quoteFrom) && valid_quote(quoteTo) )
			{
				double Pa = quoteFrom.ask;
				double Pb = quoteFrom.bid;
				double Sa = quoteFrom.askSize;
				double Sb = quoteFrom.bidSize;
				double dPa = quoteTo.ask - quoteFrom.ask;
				double dPb = quoteTo.bid - quoteFrom.bid;
				double dSa = quoteTo.askSize - quoteFrom.askSize;
				double dSb = quoteTo.bidSize - quoteTo.bidSize;
				double SaOL = 0.;
				double SbOL = 0.;
				get_dsOL(provider, quoteFrom, SaOL, SbOL);
				double Pmid = (Pa + Pb) / 2.;
				double dSaOL = SaOL - quoteFrom.askSize;
				double dSbOL = SbOL - quoteFrom.bidSize;
			}

			mTrades_.erase(itt);
			mCompBkTop_.erase(itq);
		}
	}
}

float SignalCalculatorTP::get_fillImb(const QuoteInfo& quote, const TradeInfo& trade)
{
	float ret = 0.;
	bool trade_at_ask = fabs(quote.ask - trade.price) < ltmb_;
	bool trade_at_bid = fabs(quote.bid - trade.price) < ltmb_;
	if( trade_at_ask && !trade_at_bid )
		ret = 1.;
	else if( !trade_at_ask && trade_at_bid )
		ret = -1.;
	return ret;
}

float SignalCalculatorTP::get_fillImb(TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	QuoteInfo quote = provider->Bbo( (unsigned char)lastBookTrade_.flags );
	string market = string(1, lastBookTrade_.flags);
	if( valid_quote(quote) && quote.ask > quote.bid )
		return get_fillImb(quote, lastBookTrade_);
	return 0.;
}

bool SignalCalculatorTP::allTaken(TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	QuoteInfo quote = provider->Bbo( (unsigned char)lastBookTrade_.flags );
	string market = string(1, lastBookTrade_.flags);
	if( valid_quote(quote) && quote.ask > quote.bid )
	{
		float rat = 0.;
		bool trade_at_ask = fabs(quote.ask - lastBookTrade_.price) < ltmb_;
		bool trade_at_bid = fabs(quote.bid - lastBookTrade_.price) < ltmb_;
		if( trade_at_ask && !trade_at_bid )
			return quote.askSize == lastBookTrade_.qty;
		else if( !trade_at_ask && trade_at_bid )
			return quote.bidSize == lastBookTrade_.qty;
	}
	return false;
}

void SignalCalculatorTP::koreanTevtFilter(hff::SigC& sig, bool allowEarlierQuote)
{
	const int dtLarge = 1000000;
	int dMin = 2000;
	int dMax = 3000;
	int oIdx = -1;
	int qIdx = -1;
	int nQuotes = quotes_.size();
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	int Npts = sig.sI.size();
	if(Npts > 0)
	{
		const vector<OrderData>* porders = static_cast<const vector<OrderData>*>(MEvent::Instance()->get(ticker_, "orders"));
		int nOrders = (porders != nullptr) ? porders->size() : 0;
		for( int k = 0; k < Npts; ++k )
		{
			hff::StateInfo& sI = sig.sI[k];
			if(sI.sampleType & trade_event_)
			{
				int msecs = sI.msso + linMod.openMsecs;

				int minAbsDt = dtLarge;
				int minDt = dtLarge;
				int matchOrderIdx = -1;
				while(oIdx > 0 && (*porders)[oIdx].msecs > msecs - dMin)
					--oIdx;
				while(oIdx < nOrders - 1 && (*porders)[oIdx].msecs < msecs + dMax)
				{
					const OrderData& od = (*porders)[oIdx];
					if(od.msecs > msecs - dMin)
					{
						if(od.type & od.addDeleteIndicator && od.price == ROUND(sI.ltrdPrc * 10000))
						{
							int dt = od.msecs - (sI.msso + linMod.openMsecs);
							if(abs(dt) < minAbsDt)
							{
								minAbsDt = abs(dt);
								minDt = dt;
								matchOrderIdx = oIdx;
							}
							else
								break;
						}
					}
					++oIdx;
				}
				//if(minDt < 0) // trade comes after quote
				//	sI.gptOK = 0;
				//else if(matchOrderIdx >= 0)
				if(!allowEarlierQuote && minDt < 0)
					sI.gptOK = 0;
				if(matchOrderIdx >= 0)
				{
					while(qIdx < nQuotes - 1 && quotes_[qIdx].msecs < (*porders)[matchOrderIdx].msecs && quotes_[qIdx+1].msecs < (*porders)[matchOrderIdx].msecs + linMod.transientMsecs)
						++qIdx;
					QuoteInfo& quote = quotes_[qIdx];
					if(abs(quote.bid - sI.bid) > ltmb_ || abs(quote.ask - sI.ask) > ltmb_)
						sI.gptOK = 0;
				}
			}
		}
	}
}

void SignalCalculatorTP::calculate_targets(hff::SigC& sig, const vector<QuoteInfo>& quotes, bool smoothTarget)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;

	vector<double> vMid1d(linMod.n1sec);
	if(smoothTarget)
		get_mid_series_smooth(vMid1d, &quotes, linMod.openMsecs, linMod.closeMsecs, 0, false, true);
	else
		get_mid_series(vMid1d, &quotes, linMod.openMsecs, linMod.closeMsecs, 0, false, true);
	vector<int> vQindx1s = get_index_1s(&quotes, linMod.openMsecs, linMod.closeMsecs);

	const vector<pair<int, int> >& vHorizonLag = nonLinMod.vHorizonLag;
	int nT = vHorizonLag.size();
	int Npts = sig.sI.size();
	for( int k = 0; k < Npts; ++k )
	{
		hff::StateInfo& sI = sig.sI[k];
		int sec = sI.msso / 1000;
		int secNearFut = sI.msso / 1000 + 1;
		if( sI.gptOK == 1 )
		{
			int secPersistChina = min(sec + 4, linMod.n1sec - 1);
			QuoteInfo qFuture = quotes[vQindx1s[secPersistChina]];
			sI.persistentChina = get_persistent(qFuture, sI);

			// Same day target
			for( int iT = 0; iT < nT; ++iT )
			{
				int horizon = vHorizonLag[iT].first;
				int lag = vHorizonLag[iT].second;

				float midFrom = 0.;
				if( lag == 0 )
					midFrom = sI.mqut;
				else if( lag > 0 )
				{
					int secFrom = min(secNearFut + lag, linMod.n1sec - 1);
					midFrom = vMid1d[secFrom];
				}

				int secTo = min(secNearFut + lag + horizon, linMod.n1sec - 1);
				float midTo = vMid1d[secTo];

				if( midFrom > 0. && midTo > 0. )
				{
					sI.target[iT] = basis_pts_ * (midTo / midFrom - 1.);

					// clip.
					if( horizon <= 60 )
						clip(sI.target[iT], linMod.om_target_clip);
					else if( horizon <= 600 )
						clip(sI.target[iT], linMod.tm_target_clip);
					else if( horizon <= 3600 )
						clip(sI.target[iT], linMod.tm_target_60_clip);
					else if( horizon > 3600 )
						clip(sI.target[iT], linMod.tm_target_intra_clip);

					// Copy. Originals will be hedged.
					sI.targetUH[iT] = sI.target[iT];
				}
				else if (dropBadTarget_)
					sI.gptOK = 0;
			}

			// to close.
			{
				float midTo = sig.close;
				{
					float midFrom = sI.mqut;
					if( midFrom > 0. && midTo > 0. )
						sI.targetClose = basis_pts_ * (midTo / midFrom - 1.);
				}
				{
					int secFrom = secNearFut + 11*60;
					if(secFrom < linMod.n1sec - 1)
					{
						float midFrom = vMid1d[secFrom];
						if( midFrom > 0. && midTo > 0. )
							sI.target11Close = basis_pts_ * (midTo / midFrom - 1.);
					}
				}
				{
					int secFrom = secNearFut + 71*60;
					if(secFrom < linMod.n1sec - 1)
					{
						float midFrom = vMid1d[secFrom];
						if( midFrom > 0. && midTo > 0. )
							sI.target71Close = basis_pts_ * (midTo / midFrom - 1.);
						else if (dropBadTarget_)
							sI.gptOK = 0;
					}
				}
			}

			// Next day target
			{
			}

			// clip.
			clip(sI.target11Close, linMod.tm_target_intra_clip);
			clip(sI.target71Close, linMod.tm_target_intra_clip);
			//clip(sI.targetClose, linMod.tm_target_intra_clip);

			// Copy. Originals will be hedged.
			sI.target11CloseUH = sI.target11Close;
			sI.target71CloseUH = sI.target71Close;
			sI.targetCloseUH = sI.targetClose;
		}
	}
}

void SignalCalculatorTP::calculate_targets_next(hff::SigC& sig, const vector<QuoteInfo>& quotesNext)
{
	//sig.tarNext15m45m;
	//sig.tarNext15m75m;
	//sig.tarNext15mClose;
}

void SignalCalculatorTP::calculate_exchange_vol(hff::SigC& sig)
{
	string model = MEnv::Instance()->model;
	if(model[0] == 'E')
	{
		switch (sig.exchangeChar[0])
		{
			case 'I': sig.exchVol = 0.; break;
			case 'O': sig.exchVol = 1.; break;
			case 'Y': sig.exchVol = 2.; break;
			case 'B': sig.exchVol = 3.; break;
			case 'Z': sig.exchVol = 4.; break;
			case 'C': sig.exchVol = 5.; break;
			case 'D': sig.exchVol = 6.; break;
			case 'W': sig.exchVol = 7.; break;
			case 'A': sig.exchVol = 8.; break;
			case 'M': sig.exchVol = 9.; break;
			case 'X': sig.exchVol = 10.; break;
			case 'P': sig.exchVol = 11.; break;
			case 'L': sig.exchVol = 12.; break;
			case 'F': sig.exchVol = 13.; break;
			default: cout << "calculate_exchange_vol() " << sig.exchangeChar << endl;
		}
	}
}

void SignalCalculatorTP::calculate_atf_signals(hff::StateInfo& sI,
		TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	int idate = MEnv::Instance()->idate;
	unsigned char mktP = ticker_[0];
	QuoteInfo quoteP = provider->Bbo(mktP);

	QuoteInfo quoteA;
	auto ecns = linMod.get_ecns(idate);
	for(auto& ecn : ecns)
	{
		unsigned char mkt = ecn[1];
		if(mkt != mktP)
		{
			QuoteInfo quote = provider->Bbo(mkt);
			if(quoteA.bidSize == 0 || quote.bid > quoteA.bid + ltmb_ || fabs(quote.bid - quoteA.bid) < ltmb_ && quote.bidSize > quoteA.bidSize)
			{
				quoteA.bid = quote.bid;
				quoteA.bidSize = quote.bidSize;
			}
			if(quoteA.askSize == 0 || quote.ask < quoteA.ask - ltmb_ || fabs(quote.ask - quoteA.ask) < ltmb_ && quote.askSize > quoteA.askSize)
			{
				quoteA.ask = quote.ask;
				quoteA.askSize = quote.askSize;
			}
		}
	}

	double midP = get_mid(quoteP.bid, quoteP.ask);
	double midA = get_mid(quoteA.bid, quoteA.ask);
	double sprdP = quoteP.ask - quoteP.bid;
	double sprdA = quoteA.ask - quoteA.bid;
	double sprdNbbo = sI.ask - sI.bid;

	if(sprdP > ltmb_ && sprdNbbo > ltmb_)
		sI.moP = sprdNbbo / sprdP * (midP / sI.mqut - 1.);
	if(sprdA > ltmb_ && sprdNbbo > ltmb_)
		sI.moA = sprdNbbo / sprdA * (midA / sI.mqut - 1.);

	sI.bidOffA = quoteA.bid - sI.bid;
	sI.askOffA = quoteA.ask - sI.ask;

	if(quoteP.askSize + quoteP.bidSize > 0)
		sI.quimP = float(quoteP.bidSize - quoteP.askSize) / (quoteP.askSize + quoteP.bidSize);
	if(quoteA.askSize + quoteA.bidSize > 0)
		sI.quimA = float(quoteA.bidSize - quoteA.askSize) / (quoteA.askSize + quoteA.bidSize);
}

void SignalCalculatorTP::calculate_tob_signals(hff::StateInfo& sI,
		TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	TobSnapshot<QuoteInfo>* tob = provider->Tob();
	vector<const QuoteInfo*> bidSide(max_book_levels_);
	vector<const QuoteInfo*> askSide(max_book_levels_);
	vector<double> vqI(max_book_levels_);
	vector<double> voff(max_book_levels_);
	int nTobMarkets;
	tob->BidSide(&bidSide[0], &nTobMarkets, max_book_levels_);
	tob->AskSide(&askSide[0], &nTobMarkets, max_book_levels_);
	if(nTobMarkets >= 2)
	{
		if( fabs( askSide[0]->ask - lastNbbo_.ask) < min_tob_nbbo_dif_
				&& fabs( bidSide[0]->bid - lastNbbo_.bid) < min_tob_nbbo_dif_ )
		{
			double mid_0 = get_mid(bidSide[0]->bid, askSide[0]->ask);
			double sprd_0 = askSide[0]->ask - bidSide[0]->bid;
			double sprdOff_0 = (sprd_0 > minSprdOff_) ? sprd_0 : minSprdOff_;

			for( int x = 0; x < nTobMarkets; ++x )
			{
				if( askSide[x]->askSize > 0 && bidSide[x]->bidSize > 0 )
				{
					double mid_x = get_mid(bidSide[x]->bid, askSide[x]->ask);
					double sprd_x = askSide[x]->ask - bidSide[x]->bid;
					double sprdOff_x = (sprd_x > minSprdOff_) ? sprd_x : minSprdOff_;
					double sprdRel_x = basis_pts_ * sprdOff_x / mid_0;

					if( mid_x > min_price_ && sprdRel_x <= skip_qt_ )
					{
						double totSize_x = askSide[x]->askSize + bidSide[x]->bidSize;
						double qimb_x = (bidSide[x]->bidSize - askSide[x]->askSize) / totSize_x;
						double off_x = (sprd_x > 0.) ? basis_pts_ * (mid_x / mid_0 - 1.) * (sprdOff_0 / sprdOff_x) : 0.;
						vqI[x] = qimb_x;
						voff[x] = off_x;
					}
				}
			}
		}
		sI.sig1[8] = vqI[1];
		sI.sig1[9] = vqI[2];
		sI.sig1[10] = voff[1];
		sI.sig1[11] = voff[2];
		sI.sig10[8]  = sI.sig1[8];
		sI.sig10[9]  = sI.sig1[9];
		sI.sig10[10] = sI.sig1[10];
		sI.sig10[11] = sI.sig1[11];
	}
	sI.tobOK = 1;
}

void SignalCalculatorTP::calculate_book_signals(hff::SigC& sig, hff::StateInfo& sI,
		TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	if( sI.tobOK == 1 )
	{
		const int maxLevels = 1000;
		const unsigned nVolFrac = 6;
		const unsigned nSprdBins = 7;
		const double volumeFrac[nVolFrac] = {0.01, 0.02, 0.04, 0.08, 0.16, 0.32};
		const double sprdBins[nSprdBins] = {0.25, 0.5, 1., 2., 4., 8., 16.};
		vector<double> bookDepthMO(nVolFrac);
		vector<double> bookDepthQI(nSprdBins);
		vector<double> vBookDepthQI(nSprdBins);

		OrderBk<OrderData>* compBk = provider->CompBk();
		vector<const OrderData*> bidSide(maxLevels);
		vector<const OrderData*> askSide(maxLevels);
		int nBidSide = 0;
		int nAskSide = 0;
		compBk->GetBidSideBook(maxLevels, &bidSide[0], &nBidSide);
		compBk->GetAskSideBook(maxLevels, &askSide[0], &nAskSide);

		if( nBidSide >= 2 && nAskSide >= 2
				&& fabs(askSide[0]->RealPrice() - lastNbbo_.ask) <= min_tob_nbbo_dif_
				&& fabs(bidSide[0]->RealPrice() - lastNbbo_.bid) <= min_tob_nbbo_dif_
				&& bidSide[0]->size > 0 && askSide[0]->size > 0 ) 
		{
			int nLevels = min(nBidSide, nAskSide);
			nLevels = min(nLevels, maxLevels);

			double mid_0 = 0.5 * (askSide[0]->RealPrice() + bidSide[0]->RealPrice());
			double sprd_0 = askSide[0]->RealPrice() - bidSide[0]->RealPrice();
			double sprdOff_0 = (sprd_0 > minSprdOff_) ? sprd_0 : minSprdOff_;
			double sprdRel_0 = basis_pts_ * sprdOff_0 / mid_0;

			if( fabs(sprdRel_0) < skip_qt_ && mid_0 > min_price_ )
			{
				vector<double> inputs(hff::max_book_sigs_);
				vector<double> sprdi(max_book_levels_);
				double sizeratMax = 20;

				float pmedmedSprd = sig.medSprd * mid_0;
				//calcBookDepthSigsMO(nLevels, &bidSide[0], nLevels, &askSide[0], // 20181016
				calcBookDepthSigsMO(nBidSide, &bidSide[0], nAskSide, &askSide[0], // 20181016
						mid_0, sprdOff_0, sig.avgDlyVolume, nVolFrac, volumeFrac, bookDepthMO);
				//calcBookDepthSigsQI(nLevels, &bidSide[0], nLevels, &askSide[0], // 20181016
				calcBookDepthSigsQI(nBidSide, &bidSide[0], nAskSide, &askSide[0], // 20181016
						mid_0, pmedmedSprd, nSprdBins, sprdBins, bookDepthQI, vBookDepthQI, sig.avgDlyVolume);

				sI.sigBook[8] = bookDepthMO[0];
				sI.sigBook[14] = bookDepthMO[1];
				sI.sigBook[15] = bookDepthMO[2];
				sI.sigBook[16] = bookDepthMO[3];
				sI.sigBook[17] = bookDepthMO[4];
				sI.sigBook[18] = bookDepthMO[5];

				sI.sigBook[19] = bookDepthQI[0];
				sI.sigBook[20] = bookDepthQI[1];
				sI.sigBook[21] = bookDepthQI[2];
				sI.sigBook[22] = bookDepthQI[3];
				sI.sigBook[23] = bookDepthQI[4];
				sI.sigBook[24] = bookDepthQI[5];
				sI.sigBook[25] = bookDepthQI[6];

				sI.sigBook[9] = vBookDepthQI[2];
				sI.sigBook[10] = vBookDepthQI[3];
				sI.sigBook[11] = vBookDepthQI[4];
				sI.sigBook[12] = vBookDepthQI[5];
				sI.sigBook[13] = vBookDepthQI[6];
				sI.sigBook[26] = vBookDepthQI[0];
				sI.sigBook[27] = vBookDepthQI[1];

				// loop over levels.
				int nLevelsUsed = min(nLevels, max_book_levels_);
				double totSize_0 = askSide[0]->size + bidSide[0]->size;
				for(int x = 0; x < nLevelsUsed; ++x)
				{	
					double totSize_x = askSide[x]->size + bidSide[x]->size;
					double mid_x = 0.5 * (askSide[x]->RealPrice() + bidSide[x]->RealPrice());
					double sprd_x = askSide[x]->RealPrice() - bidSide[x]->RealPrice();
					double sprdOff_x = (sprd_x > minSprdOff_) ? sprd_x : minSprdOff_;

					double sprdRel_x = basis_pts_ * sprdOff_x / mid_0;
					if( sprdRel_x < skip_qt_ && mid_x > min_price_
							&& askSide[x]->size > 0 && bidSide[x]->size > 0)
					{	
						double qimb_x = ((double)bidSide[x]->size - (double)askSide[x]->size) / totSize_x;
						inputs[4 * x] = qimb_x; //quote imbalance
						if(x > 0 && sprd_x > 0.)
						{
							inputs[4 * x - 3] = basis_pts_ * (mid_x / mid_0 - 1.) * (sprdOff_0 / sprdOff_x); // offset
							inputs[4 * x - 2] = totSize_x / totSize_0;   // qutRat
							if(sizeratMax > 0.0 && inputs[4 * x - 2] >  sizeratMax) inputs[4 * x - 2] = sizeratMax;
							inputs[4 * x - 1] = sprdOff_0 / sprd_x;
						}
					}
				}

				sI.sigBook[0] = inputs[2]; // qutRat
				sI.sigBook[1] = inputs[6]; // qutRat
				sI.sigBook[2] = inputs[3]; // sprdRat
				sI.sigBook[3] = inputs[7]; // sprdRat
				sI.sigBook[4] = inputs[4]; // qutImb   
				sI.sigBook[5] = inputs[8]; // qutImb
				sI.sigBook[6] = inputs[1]; // offset
				sI.sigBook[7] = inputs[5]; // offset
			}
		}
	}
}

void SignalCalculatorTP::calculate_bookcnt_signals(hff::SigC& sig, hff::StateInfo& sI,
		TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	if( sI.tobOK == 1 )
	{
		float tickSize = .01f;
		//QuoteInfo nbbo = TickProviderClassic::Nbbo();
		QuoteInfo nbbo = provider->Nbbo();

		float adjbid = nbbo.bid - .5f * tickSize;
		float adjask = nbbo.ask + .5f * tickSize;

		const int maxNQuotes = 15;
		int nQuotes = 0;
		QuoteInfo tobQuotes[maxNQuotes];
		//const TobSnapshot<QuoteInfo> *tob = TickProviderClassic::Tob();
		const TobSnapshot<QuoteInfo> *tob = provider->Tob();
		tob->GetMarkets(tobQuotes, &nQuotes, maxNQuotes);

		unsigned nquBid = 0, nquAsk = 0;
		for(unsigned u = 0; u < nQuotes; ++u)
		{
			const QuoteInfo& qu = tobQuotes[u];
			if(qu.bid >= adjbid)
				++nquBid;
			if(qu.ask <= adjask)
				++nquAsk;
		}

		double sumBidQty = 0, sumAskQty = 0, sumBidQty2 = 0, sumAskQty2 = 0;
		for(unsigned u = 0; u < maxNQuotes; ++u)
		{
			const QuoteInfo& qu = tobQuotes[u];
			if(qu.bid >= adjbid)
			{
				sumBidQty += qu.bidSize;
				sumBidQty2 += SQR(qu.bidSize);
			}
			if(qu.ask <= adjask)
			{
				sumAskQty += qu.askSize;
				sumAskQty2 += SQR(qu.askSize);
			}
		}

		double nquBidWt = sumBidQty2 > 0 ? SQR(sumBidQty) / sumBidQty2 : 0.;
		double nquAskWt = sumAskQty2 > 0 ? SQR(sumAskQty) / sumAskQty2 : 0.;

		sI.dnmk = nquBidWt - nquAskWt;
		sI.snmk = nquBidWt + nquAskWt;
	}
}

void SignalCalculatorTP::calculate_mi_signals(hff::SigC& sig, hff::StateInfo& sI,
		TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	if( tradeQty_ == 0 || tradeQty_->empty() )
		return;

	int sec = sI.msso / 1000;
	float scaleFac = (float)(n1sec_ - 1) / sec;

	sI.mI600 = f600_ * get_volQty(sec, 600, miTradeVolCum_) / miNormFac_;
	sI.mI3600 = f3600_ * get_volQty(sec, 3600, miTradeVolCum_) / miNormFac_;
	sI.mIIntra = scaleFac * get_volQty(sec, 0, miTradeVolCum_) / miNormFac_;
}

void SignalCalculatorTP::calculate_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	float firstMidQt = get_mid(firstQuote_);

	sig.logVolu = log10(sig.avgDlyVolume);
	sig.logPrice = log10(sig.adjPrevClose);
	sig.logPriceUsd = log10(curr_wgt_usd_ * sig.adjPrevClose);
	sig.logMedSprd = log10(sig.medSprd);

	sI.ltrdPrc = lastTrade_.price;
	sI.relSprd = 0.;
	sI.ltrdImb = 0.;
	if(sig.medSprd > 0)
	{
		sI.relSprd = (sI.sprd / basis_pts_) / sig.medSprd;
		sI.ltrdImb = (lastBookTrade_.price - sI.mqut) / (.5 * sig.medSprd * sI.mqut);
	}

	// intra-day volatility estimated from high and low
	sI.relVolat = 0.;
	if(high_ >= low_ && low_ > 0. && sI.msso > 0)
	{
		float scaleFac = sqrt((float)(n1sec_ - 1)*1000 / sI.msso);
		sI.relVolat = scaleFac * basis_pts_ * pow(log(high_/low_), 2) / sig.avgDlyVolat;
	}

	sI.bsize = lastNbbo_.bidSize / 100.;
	sI.asize = lastNbbo_.askSize / 100.;
	sI.bidEx = lastNbbo_.bidEx;
	sI.askEx = lastNbbo_.askEx;

	// mret300 clip
	sI.sig10[2] = sI.mret300;
	clip(sI.sig10[2], linMod.clip_ret300);

	sI.sig1[14] = sI.sig10[2];

	// mret300L 
	sI.sig10[3] = sI.mret300L;
	clip(sI.sig10[3], linMod.clip_ret300);

	sI.sig1[15] = sI.sig10[3];			

	// mret 600 lagged 600 
	sI.sig10[5] = sI.mret600L; // ok since, unlike, sig[2], no nonlinear transform
	clip(sI.sig10[5], max_ret_);

	// mret 1200 lagged 1200
	sI.sig10[6] = sI.mret1200L; 
	clip(sI.sig10[6], max_ret_);

	// mret 2400 lagged 2400
	clip(sI.mret2400L, max_ret_);

	// mret 4800 lagged 4800
	clip(sI.mret4800L, max_ret_);

	// mretOpen
	//if(firstMidQt > 0.)
	sI.mretOpen = basis_pts_ * (sI.mqut / firstMidQt - 1.);
	clip(sI.mretOpen, max_ret_);

	// qutImbWt
	if(msecs - lastNbbo_.msecs < 1000 * max_qtwt_lag_)
	{
		sI.sig10[0] = ((double)(sI.bsize - sI.asize)) / (sI.bsize + sI.asize);
		sI.sig10[0] *= sqrt(max(sI.bsize, sI.asize) / sig.avgDlyVolume);
	}

	// qutImbMax
	if(msecs - lastNbbo_.msecs < 1000 * max_qtmax_lag_ && sI.maxbsize > 0 && sI.maxasize > 0)
		sI.sig10[1] = ((double)(sI.maxbsize - sI.maxasize)) / (sI.maxbsize + sI.maxasize);

	// qutImbMax2
	if(msecs - lastNbbo_.msecs < 1000 * max_qtmax_lag2_ && sI.maxbsize2 > 0 && sI.maxasize2 > 0)
		sI.quimMax2 = ((double)(sI.maxbsize2 - sI.maxasize2)) / (sI.maxbsize2 + sI.maxasize2);

	//scale hilo by vol: this should not be used anywhere 
	sI.sig10[4] = sI.relativeHiLo;

	// one minute signals 
	if(msecs - lastNbbo_.msecs < 1000 * om_stale_qut_)
		sI.sig1[0] = ((float)(sI.bsize - sI.asize)) / (sI.bsize + sI.asize);

	// fillImb (sig1[1]) and hilo (sig1[3]) done earlier
	// signals mret300 (sig1[14]) and mret300Lag (sig1[15]) are calculated above
	sI.sig1[3] = sI.relativeHiLo;

	sI.sig1[2] = sI.mret60;
	clip(sI.sig1[2], linMod.clip_ret60);

	sI.absSprd = fabs(sI.sprd);
	clip(sI.absSprd, multi_lin_sprd_clip_);
}

void SignalCalculatorTP::get_top_comp_book(QuoteInfo& compBkTop, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	const int maxBookLevels = 100;
	int nBidLevels, nAskLevels;
	const OrderData* bidSide[maxBookLevels];
	const OrderData* askSide[maxBookLevels];

	OrderBk<OrderData>* compBk = provider->CompBk(); // get composite book.
	compBk->GetBidSideBook(maxBookLevels, bidSide, &nBidLevels); // read bid side of composite book.
	compBk->GetAskSideBook(maxBookLevels, askSide, &nAskLevels); // read ask side of composite book.

	if( nBidLevels > 0 && nAskLevels > 0 )
	{
		compBkTop.bidSize = bidSide[0]->size;
		compBkTop.bid = bidSide[0]->RealPrice();
		compBkTop.ask = askSide[0]->RealPrice();
		compBkTop.askSize = askSide[0]->size;
		for( int i = 1; i < nBidLevels; ++i )
		{
			if( fabs(bidSide[i]->RealPrice() - compBkTop.bid) < ltmb_ )
				compBkTop.bidSize += bidSide[i]->size;
			else if( bidSide[i]->RealPrice() < compBkTop.bid - ltmb_ )
				break;
		}
		for( int i = 1; i < nAskLevels; ++i )
		{
			if( fabs(askSide[i]->RealPrice() - compBkTop.ask) < ltmb_ )
				compBkTop.askSize += askSide[i]->size;
			else if( askSide[i]->RealPrice() > compBkTop.ask + ltmb_ )
				break;
		}
	}
}

void SignalCalculatorTP::get_dsOL(TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, QuoteInfo& quoteFrom, double& SaOL, double& SbOL)
{
	const int maxBookLevels = 1000;
	int nBidLevels, nAskLevels;
	const OrderData* bidSide[maxBookLevels];
	const OrderData* askSide[maxBookLevels];

	OrderBk<OrderData>* compBk = provider->CompBk(); // get composite book.
	compBk->GetBidSideBook(maxBookLevels, bidSide, &nBidLevels); // read bid side of composite book.
	compBk->GetAskSideBook(maxBookLevels, askSide, &nAskLevels); // read ask side of composite book.

	if( nBidLevels > 0 && nAskLevels > 0 )
	{
		for( int i = 0; i < nBidLevels; ++i )
		{
			if( fabs(bidSide[i]->RealPrice() - quoteFrom.bid) < ltmb_ ) // same price level as old quote.
				SbOL += bidSide[i]->size;
			else if( bidSide[i]->RealPrice() < quoteFrom.bid - ltmb_ )
				break;
		}
		for( int i = 0; i < nAskLevels; ++i )
		{
			if( fabs(askSide[i]->RealPrice() - quoteFrom.ask) < ltmb_ )
				SaOL += askSide[i]->size;
			else if( askSide[i]->RealPrice() > quoteFrom.ask + ltmb_ )
				break;
		}
	}
}
