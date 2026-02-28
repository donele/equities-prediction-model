#include <MSigMod/SignalCalculatorMicroTest.h>
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

const bool debug_first_quote = false; // If true, get the first quote from nbbo tickdata. Use to compare to reg sample code.

SignalCalculatorMicroTest::SignalCalculatorMicroTest()
	:debug_(false),
	pStatusChange_(nullptr)
{
}

SignalCalculatorMicroTest::SignalCalculatorMicroTest(int idate, const string& uid, const string& ticker, SigC& sig,
		Sessions* sessions, int openMsecs, int closeMsecs, const string& auctionMkts,
		const vector<hff::OrderQty>* tradeQty)
: TCS_book_micro(ticker,UsecsTime(openMsecs),UsecsTime(closeMsecs),auctionMkts),
	debug_(false),
	discardOutliers_(true),
	quoteSampleStep_(0),
	pStatusChange_(nullptr),
	idate_(idate),
	tradeSampleStep_(0),
	booktradeSampleStep_(0),
	uid_(uid),
	ticker_(ticker),
	openMsecs_(openMsecs),
	closeMsecs_(closeMsecs),
	n1sec_((closeMsecs - openMsecs) / 1000 + 1),
	cnt_(-1),
	openDelay_(0),
	msecsLastEvent_(0),
	validt_(0),
	lastFillImb_(0),
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

SignalCalculatorMicroTest::~SignalCalculatorMicroTest()
{
}

void SignalCalculatorMicroTest::setDebug()
{
	debug_ = true;
}

void SignalCalculatorMicroTest::setDiscardOutliers(bool tf)
{
	discardOutliers_ = tf;
}

void SignalCalculatorMicroTest::setStatusChange(StatusChangeUS* p)
{
	pStatusChange_ = p;
}

void SignalCalculatorMicroTest::setOpenDelay(int delay)
{
	openDelay_ = delay;
}

void SignalCalculatorMicroTest::setQuoteSampleStep(int step)
{
	quoteSampleStep_ = step;
	nbboCounter_ = SampleCounter(quoteSampleStep_);
}

void SignalCalculatorMicroTest::setTradeSampleStep(int step)
{
	tradeSampleStep_ = step;
	tradeCounter_ = SampleCounter(tradeSampleStep_);
}

void SignalCalculatorMicroTest::setBooktradeSampleStep(int step)
{
	booktradeSampleStep_ = step;
	booktradeCounter_ = SampleCounter(booktradeSampleStep_);
}

const vector<QuoteInfo>& SignalCalculatorMicroTest::getQuotes() const
{
	return quotes_;
}

const TradeInfo& SignalCalculatorMicroTest::getFirstTrade() const
{
	return firstTrade_;
}

const QuoteInfo& SignalCalculatorMicroTest::getFirstQuote() const
{
	return firstQuote_;
}

void SignalCalculatorMicroTest::endTicker()
{
	qSample_.endTicker();
}

void SignalCalculatorMicroTest::addQuote(const QuoteInfo& quote)
{
	quotes_.push_back(quote);
	qSample_.addQuote(quote);
}

void SignalCalculatorMicroTest::addTrade(const TradeInfo& trade)
{
	trades_.push_back(trade);

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

void SignalCalculatorMicroTest::addBookTrade(const TradeInfo& trade)
{
	// Update sd(trade price)
	accTrd_.add(trade.price);

	// Update last book trade.
	nbboBeforeLastBookTrade_ = lastNbbo_;
	nbboBeforeLastBookTradeWithLessTime_ = lastNbboWithLessTime_;
}

void SignalCalculatorMicroTest::setTmSampleMsecs(const vector<int>& sampleMsecs, const int scale)
{
	tmSampleMsecs_.clear();
	int N = sampleMsecs.size();
	for( int i = 0; i < N; ++i )
	{
		if( (i + 1) % scale == 0 ) // To select 300, 600, etc.
			tmSampleMsecs_.insert(sampleMsecs[i]);
	}
}

// -------------------------------------------------------------------------
//
// Callback functions.
//
// -------------------------------------------------------------------------

void SignalCalculatorMicroTest::NbboCB(UsecsTime ti,const QuoteDataMicro& qdm)
{
	TCS_book_micro::NbboCB(ti,qdm);//this must be called somewhwere inside NbboCB, usually right at the beginning
	int msecs = ti.Msecs();

	//static SampleCounter counter(quoteSampleStep);
	nbboCounter_.update();
	if( !pSessions_->isAfterOpenBeforeClose(msecs) )
		return;
	if( pStatusChange_ != nullptr && pStatusChange_->isHalted(idate_, ticker_, msecs) )
		return;

	// Update last nbbo.
	QuoteInfo this_nbbo = Nbbo();
	if(this_nbbo.msecs > lastNbbo_.msecs)
		lastNbboWithLessTime_ = lastNbbo_;
	lastNbbo_ = this_nbbo;
	addQuote(this_nbbo);

	// Last event time.
	msecsLastEvent_ = ti.Msecs();

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
		NewSig(msecs, nbbo_event_);
}

void SignalCalculatorMicroTest::TradeCB(UsecsTime ti,const TradeDataMicro& trade)
{
	TCS_book_micro::TradeCB(ti,trade);//this must be called somewhwere inside TradeCB, usually right at the beginning
	int msecs = ti.Msecs();

	//static SampleCounter counter(tradeSampleStep);
	tradeCounter_.update();
	if( !pSessions_->isAfterOpenBeforeClose(ti.Msecs()) )
		return;
	if( pStatusChange_ != nullptr && pStatusChange_->isHalted(idate_, ticker_, msecs) )
		return;

	msecsLastEvent_ = msecs;
	TradeInfo this_trade = LastTrade();

	if( valid_trade(this_trade, lastNbbo_))
	{
		// Update last trade.
		lastTrade_ = this_trade;

		addTrade(this_trade);
		if( firstTrade_.msecs == 0 )
			firstTrade_ = this_trade;

		// Seen a trade after valid quote.
		if( !validt_ && firstQuote_.msecs > 0 )
			validt_ = 1;

		// Trade within bid and ask
		bool valid_quote_trade = lastTrade_.price + 1e-6 >= lastNbbo_.bid && lastTrade_.price - 1e-6 <= lastNbbo_.ask;
		if( valid_quote_trade && tradeCounter_.doSample() )
			NewSig(msecs, trade_event_);
	}
}

void SignalCalculatorMicroTest::BookTradeCB(UsecsTime ti,const TradeDataMicro& trade)
{
	TCS_book_micro::BookTradeCB(ti,trade);//this must be called somewhwere inside BookTradeCB, usually right at the beginning
	int msecs = ti.Msecs();

	//static SampleCounter counter(booktradeSampleStep);
	booktradeCounter_.update();
	if( !pSessions_->isAfterOpenBeforeClose(msecs) )
		return;
	if( pStatusChange_ != nullptr && pStatusChange_->isHalted(idate_, ticker_, msecs) )
		return;

	// Last event time.
	msecsLastEvent_ = msecs;

	TradeInfo this_trade = LastBookTrade();

	if( valid_trade(this_trade, lastNbbo_))
	{
		lastBookTrade_ = this_trade;
		addBookTrade(this_trade);
		lastFillImb_ = get_fillImb();

		// Seen a trade after valid quote.
		if( !validt_ && firstQuote_.msecs > 0 )
			validt_ = 1;

		bool newsig_ok = false;
		if( booktradeCounter_.doSample() )
			newsig_ok = NewSig(msecs, book_trade_event_);
	}
}

void SignalCalculatorMicroTest::RegularCB(UsecsTime ti)//fundamentally the same
{
	int msecs = ti.Msecs();
	if( !pSessions_->isAfterOpenBeforeClose(msecs) )
		return;
	if( pStatusChange_ != nullptr && pStatusChange_->isHalted(idate_, ticker_, msecs) )
		return;

	NewSig(msecs, regular_sample_);
}

void SignalCalculatorMicroTest::setPersistence(int iSig)
{
	StateInfo& ti = psig_->sI[iSig];

	// Check transient.
	{
		QuoteInfo nbbo = Nbbo();

		if ( nbbo.bid < ti.bid - 0.0001 )
			ti.bidPersists = 0;

		if ( nbbo.ask > ti.ask + 0.0001 )
			ti.askPersists = 0;
	}
}

// -------------------------------------------------------------------------
//
// Signal calculation.
//
// -------------------------------------------------------------------------

bool SignalCalculatorMicroTest::NewSig(int msecs, int sampleType)
{
	if( !validt_ || firstQuote_.msecs <= 0 || !valid_quote(lastNbbo_) || msecs <= firstQuote_.msecs || !pSessions_->inSessionStrictWait(msecs, openDelay_) )
		return false;
	if( pStatusChange_ != nullptr && pStatusChange_->isHalted(idate_, ticker_, msecs) )
		return false;
 
	// check crossed.
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	if( !linMod.allowCross && lastNbbo_.ask - lastNbbo_.bid < min_tick_default_ )
		return false;

	hff::SigC& sig = *psig_;
	StateInfo sI(linMod, nonLinMod);

	int msso = msecs - openMsecs_;
	sI.msso = msso;
	sI.sampleType = sampleType;
	sI.mqut = get_mid(lastNbbo_);
	sI.bid = lastNbbo_.bid;
	sI.ask = lastNbbo_.ask;

	calculate_trade_signals(sig, sI, msecs);
	calculate_quote_signals(sig, sI, msecs);
	calculate_fillimb_signals(sI, msecs, sampleType);
	calculate_tob_signals(sI);
	calculate_book_signals(sig, sI);
	calculate_bookcnt_signals(sig, sI);
	calculate_mi_signals(sig, sI);
	calculate_signals(sig, sI, msecs);

	if( discardOutliers_ && fabs(sI.mret60) > om_max_ret_ )
		return false;
	if( sampleType & book_trade_event_ )
	{
		if( !linMod.use_us_arca_trade_event && string(1, lastBookTrade_.flags) == "P" )
			return false;
	}

	// Update linear model and write binary signal files if gptOK.
	sI.gptOK = 1;
	if( tmSampleMsecs_.count(msecs) )
		sI.isTmSample = true;

	sig.sI.push_back(sI);

	int iSig = sig.sI.size() - 1;
	TaskPersistence task(this, iSig);
	_provider->SetTask(UsecsTime(msecs + linMod.transientMsecs), &task);
	return true;
}

void SignalCalculatorMicroTest::calculate_trade_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs)
{
	int sec = sI.msso / 1000;

	//sI.sdTrd = accTrd_.stdev();
	//float scaleFac = sqrt((float)(n1sec_ - 1) / sec);
	//sI.sdTrdNorm = sI.sdTrd * scaleFac / sig.avgDlyVolat;
	if(lastBookTrade_.price > 0.)
	{
		sI.rtrd = sI.mqut / lastBookTrade_.price - 1.;
		sI.rtrdOverSprd = sI.rtrd / sig.medSprd;
	}
	QuoteInfo& priorQuote = (lastBookTrade_.msecs > nbboBeforeLastBookTrade_.msecs) ? nbboBeforeLastBookTrade_: nbboBeforeLastBookTradeWithLessTime_;
	if(valid_quote(priorQuote))
	{
		float priorMid = get_mid(priorQuote);
		if(priorMid > 0.)
			sI.mrtrd = sI.mqut / priorMid - 1.;
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
	//sI.voluMom5 = get_voluMom(openMsecs_, msecs, 5, trades_, sig.avgDlyVolume, n1sec_);
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
}

double SignalCalculatorMicroTest::get_mret_upto1s(int msecs,
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

void SignalCalculatorMicroTest::calculate_quote_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs)
{
	float firstMidQt = get_mid(firstQuote_);
	int sec = sI.msso / 1000;

	sI.volSclBsz = basis_pts_ * lastNbbo_.bidSize / sig.avgDlyVolume;
	sI.volSclAsz = basis_pts_ * lastNbbo_.askSize / sig.avgDlyVolume;

	sI.sprd = basis_pts_ * get_sprd(lastNbbo_.bid, lastNbbo_.ask);

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

	//sI.selfInt60L60 = sI.mret60 * mret60L;
	//sI.selfInt60L120 = sI.mret60 * mret120L;
	//sI.selfInt60L300 = sI.mret60 * sI.mret300L;
	//sI.selfInt60L600 = sI.mret60 * sI.mret600L;
	//sI.selfInt60L1200 = sI.mret60 * sI.mret1200L;
	//sI.selfInt300L300 = sI.mret300 * sI.mret300L;
	//sI.selfInt300L600 = sI.mret300 * sI.mret600L;
	//sI.selfInt300L1200 = sI.mret300 * sI.mret1200L;
	//sI.selfInt300L2400 = sI.mret300 * sI.mret2400L;

	//sI.cwret15 = sI.mret15 * sig.logCap;
	sI.cwret30 = sI.mret30 * sig.logCap;
	sI.cwret60 = sI.mret60 * sig.logCap;
	sI.cwret120 = sI.mret120 * sig.logCap;
	sI.cwret300 = sI.mret300 * sig.logCap;
	sI.cwret600 = sI.mret600 * sig.logCap;
	sI.cwret300L = sI.mret300L * sig.logCap;

	//sI.vwret15 = sI.mret15 * sI.voluMom15;
	//sI.vwret30 = sI.mret30 * sI.voluMom30;
	//sI.vwret60 = sI.mret60 * sI.voluMom60;
	//sI.vwret120 = sI.mret120 * sI.voluMom120;
	//sI.vwret300 = sI.mret300 * sI.voluMom300;
	//sI.vwret600 = sI.mret600 * sI.voluMom600;

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
}

void SignalCalculatorMicroTest::calculate_fillimb_signals(hff::StateInfo& sI, int msecs, int sampleType)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	string model = MEnv::Instance()->model;

	QuoteInfo quote = Bbo( (unsigned char)lastBookTrade_.flags );
	string market = string(1, lastBookTrade_.flags);

	// Fill imbalance.
	sI.sig1[1] = lastFillImb_;

	//int vsize = vfi_.size();
	//if( vsize > 0 )
		//sI.fIm1 = vfi_[vsize - 1];
	//if( vsize > 1 )
		//sI.fIm2 = vfi_[vsize - 2];

	vfi_.push_back(lastFillImb_);
}

float SignalCalculatorMicroTest::get_fillImb(const QuoteInfo& quote, const TradeInfo& trade)
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

float SignalCalculatorMicroTest::get_fillImb()
{
	QuoteInfo quote = Bbo( (unsigned char)lastBookTrade_.flags );
	string market = string(1, lastBookTrade_.flags);
	if( valid_quote(quote) && quote.ask > quote.bid )
		return get_fillImb(quote, lastBookTrade_);
	return 0.;
}

void SignalCalculatorMicroTest::calculate_targets(hff::SigC& sig, const vector<QuoteInfo>& quotes, bool smoothTarget)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;

	vector<double> vMid1d(linMod.n1sec);
	if(smoothTarget)
		get_mid_series_smooth(vMid1d, &quotes, linMod.openMsecs, linMod.closeMsecs, 0, false);
	else
		get_mid_series(vMid1d, &quotes, linMod.openMsecs, linMod.closeMsecs, 0, false);
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
					}
				}
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

void SignalCalculatorMicroTest::calculate_tob_signals(hff::StateInfo& sI)
{
	TobSnapshot<QuoteDataMicro>* tob = Tob();
	vector<const QuoteDataMicro*> bidSide(max_book_levels_);
	vector<const QuoteDataMicro*> askSide(max_book_levels_);
	vector<double> vqI(max_book_levels_);
	vector<double> voff(max_book_levels_);
	int nTobMarkets;
	tob->BidSide(&bidSide[0], &nTobMarkets, max_book_levels_);
	tob->AskSide(&askSide[0], &nTobMarkets, max_book_levels_);
	if(nTobMarkets >= 2)
	{
		if( fabs( askSide[0]->RealAsk() - lastNbbo_.ask) < min_tob_nbbo_dif_
				&& fabs( bidSide[0]->RealBid() - lastNbbo_.bid) < min_tob_nbbo_dif_ )
		{
			double mid_0 = get_mid(bidSide[0]->RealBid(), askSide[0]->RealAsk());
			double sprd_0 = askSide[0]->RealAsk() - bidSide[0]->RealBid();
			double sprdOff_0 = (sprd_0 > minSprdOff_) ? sprd_0 : minSprdOff_;

			for( int x = 0; x < nTobMarkets; ++x )
			{
				if( askSide[x]->askSize > 0 && bidSide[x]->bidSize > 0 )
				{
					double mid_x = get_mid(bidSide[x]->RealBid(), askSide[x]->RealAsk());
					double sprd_x = askSide[x]->RealAsk() - bidSide[x]->RealBid();
					double sprdOff_x = (sprd_x > minSprdOff_) ? sprd_x : minSprdOff_;
					double sprdRel_x = basis_pts_ * sprdOff_x / mid_0;

					if( mid_x > min_price_ && sprdRel_x <= skip_qt_ )
					{
						double totSize_x = askSide[x]->askSize + bidSide[x]->bidSize;
						double qimb_x = ((double)bidSide[x]->bidSize - (double)askSide[x]->askSize) / totSize_x;
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

void SignalCalculatorMicroTest::calculate_book_signals(hff::SigC& sig, hff::StateInfo& sI)
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

		vector<const pair<const int, PriceLevelData>*> bidSide(maxLevels);
		vector<const pair<const int, PriceLevelData>*> askSide(maxLevels);
		int nBidSide = 0;
		int nAskSide = 0;
		CompBk()->GetBidSideBook(maxLevels, &bidSide[0], &nBidSide);
		CompBk()->GetAskSideBook(maxLevels, &askSide[0], &nAskSide);

		if( nBidSide >= 2 && nAskSide >= 2
				&& fabs(askSide[0]->first*OrderDataMicro::tickSize - lastNbbo_.ask) <= min_tob_nbbo_dif_
				&& fabs(bidSide[0]->first*OrderDataMicro::tickSize - lastNbbo_.bid) <= min_tob_nbbo_dif_
				&& bidSide[0]->second.totsize > 0 && askSide[0]->second.totsize > 0 ) 
		{
			int nLevels = min(nBidSide, nAskSide);
			nLevels = min(nLevels, maxLevels);

			double mid_0 = 0.5 * (askSide[0]->first*OrderDataMicro::tickSize + bidSide[0]->first*OrderDataMicro::tickSize);
			double sprd_0 = askSide[0]->first*OrderDataMicro::tickSize - bidSide[0]->first*OrderDataMicro::tickSize;
			double sprdOff_0 = (sprd_0 > minSprdOff_) ? sprd_0 : minSprdOff_;
			double sprdRel_0 = basis_pts_ * sprdOff_0 / mid_0;

			if( fabs(sprdRel_0) < skip_qt_ && mid_0 > min_price_ )
			{
				vector<double> inputs(hff::max_book_sigs_);
				vector<double> sprdi(max_book_levels_);
				double sizeratMax = 20;

				float pmedmedSprd = sig.medSprd * mid_0;
				calcBookDepthSigsMOMicro(nBidSide, bidSide, nAskSide, askSide, mid_0, sprdOff_0,
						sig.avgDlyVolume, nVolFrac, volumeFrac, bookDepthMO);
				calcBookDepthSigsQIMicro(nBidSide, bidSide, nAskSide, askSide, mid_0, pmedmedSprd,
						nSprdBins, sprdBins, bookDepthQI, vBookDepthQI, sig.avgDlyVolume);

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
				double totSize_0 = askSide[0]->second.totsize + bidSide[0]->second.totsize;
				for(int x = 0; x < nLevelsUsed; ++x)
				{	
					double totSize_x = askSide[x]->second.totsize + bidSide[x]->second.totsize;
					double mid_x = 0.5 * (askSide[x]->first*OrderDataMicro::tickSize + bidSide[x]->first*OrderDataMicro::tickSize);
					double sprd_x = askSide[x]->first*OrderDataMicro::tickSize - bidSide[x]->first*OrderDataMicro::tickSize;
					double sprdOff_x = (sprd_x > minSprdOff_) ? sprd_x : minSprdOff_;

					double sprdRel_x = basis_pts_ * sprdOff_x / mid_0;
					if( sprdRel_x < skip_qt_ && mid_x > min_price_
							&& askSide[x]->second.totsize > 0 && bidSide[x]->second.totsize > 0)
					{	
						double qimb_x = ((double)bidSide[x]->second.totsize - (double)askSide[x]->second.totsize) / totSize_x;
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

void SignalCalculatorMicroTest::calculate_bookcnt_signals(hff::SigC& sig, hff::StateInfo& sI)
{
	if( sI.tobOK == 1 )
	{
		float tickSize = .01f;
		QuoteInfo nbbo = Nbbo();

		float adjbid = nbbo.bid - .5f * tickSize;
		float adjask = nbbo.ask + .5f * tickSize;

		const int maxNQuotes = 15;
		int nQuotes = 0;
		QuoteDataMicro tobQuotes[maxNQuotes];
		const TobSnapshot<QuoteDataMicro> *tob = Tob();
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

void SignalCalculatorMicroTest::calculate_mi_signals(hff::SigC& sig, hff::StateInfo& sI)
{
	if( tradeQty_ == 0 || tradeQty_->empty() )
		return;

	int sec = sI.msso / 1000;
	float scaleFac = (float)(n1sec_ - 1) / sec;

	sI.mI600 = f600_ * get_volQty(sec, 600, miTradeVolCum_) / miNormFac_;
	sI.mI3600 = f3600_ * get_volQty(sec, 3600, miTradeVolCum_) / miNormFac_;
	sI.mIIntra = scaleFac * get_volQty(sec, 0, miTradeVolCum_) / miNormFac_;
}

void SignalCalculatorMicroTest::calculate_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	float firstMidQt = get_mid(firstQuote_);

	sig.logVolu = log10(sig.avgDlyVolume);
	sig.logPrice = log10(sig.adjPrevClose);
	sig.logMedSprd = log10(sig.medSprd);

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

void SignalCalculatorMicroTest::get_top_comp_book(QuoteInfo& compBkTop)
{
	const int maxBookLevels = 100;
	int nBidLevels, nAskLevels;
	const OrderDataMicro* bidSide[maxBookLevels];
	const OrderDataMicro* askSide[maxBookLevels];

	OrderBkMicro<OrderDataMicro, QuoteDataMicro>* compBk = CompBk(); // get composite book.
	compBk->GetBidSideBook();
	compBk->GetAskSideBook();

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

SignalCalculatorMicroTest::TaskPersistence::TaskPersistence(SignalCalculatorMicroTest* sigcal, int iSig)
	:iSig_(iSig),
	sigcal_(sigcal)
{
}
SignalCalculatorMicroTest::TaskPersistence* SignalCalculatorMicroTest::TaskPersistence::MakeCopy(void)
{
	return new TaskPersistence(sigcal_, iSig_);
}

void SignalCalculatorMicroTest::TaskPersistence::Execute(UsecsTime ti)
{
	sigcal_->setPersistence(iSig_);
}

