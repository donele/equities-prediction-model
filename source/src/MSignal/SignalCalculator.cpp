#include <MSignal/SignalCalculator.h>
#include <gtlib_model/mFtns.h>
#include <MFramework.h>
#include <jl_lib/TickSources.h>
#include <MSignal/sig.h>
#include <MSignal/sig_at.h>
#include <jl_lib/jlutil_tickdata.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <string>
using namespace std;
using namespace sig;
using namespace hff;
using namespace gtlib;

const bool debug_first_quote = false; // If true, get the first quote from nbbo tickdata. Use to compare to reg sample code.

SignalCalculator::SignalCalculator()
	:debug_fillImbBook_(false)
{
}

SignalCalculator::SignalCalculator(const string& ticker, SigC& sig, const vector<QuoteInfo>* quotes, const vector<TradeInfo>* trades,
		Sessions* sessions, int openMsecs, int closeMsecs, int exploratoryDelay, const vector<QuoteInfo>* sip)
	:debug_fillImbBook_(false),
	debug_booktrade_prob_(false),
	regularSampleProbability(0),
	quoteSampleProbability(0),
	tradeSampleProbability(0),
	booktradeSampleProbability(0),
	ticker_(ticker),
	openMsecs_(openMsecs),
	closeMsecs_(closeMsecs),
	exploratoryDelay_(exploratoryDelay),
	n1sec_((closeMsecs - openMsecs) / 1000 + 1),
	cnt_(-1),
	msecsLastEvent_(0),
	validt_(0),
	lastFillImb_(0),
	lastFillImbOld_(0),
	high_(0.),
	low_(max_float_),
	high900_(0.),
	low900_(max_float_),
	fVolM600_(0.),
	fVolM3600_(0.),
	ret_on_(0.),
	firstQuote_(QuoteInfo()),
	lastNbbo_(QuoteInfo()),
	lastTrade_(TradeInfo()),
	lastBookTrade_(TradeInfo()),
	psig_(&sig),
	pSessions_(sessions),
	quotes_(quotes),
	trades_(trades),
	news_(nullptr),
	sip_(sip),
	psigh_(nullptr)
{
	//
	// Trade.
	//
	// high900 and low900.
	int firstMsecs = 0;
	for( vector<QuoteInfo>::const_iterator it = quotes_->begin(); it != quotes_->end(); ++it )
	{
		if( it->msecs > openMsecs_ && valid_quote(*it) && it->ask >= it->bid )
		{
			firstMsecs = it->msecs;

			// debug.
			if( debug_first_quote )
				firstQuote_ = *it;

			break;
		}
	}
	int msecs_15min = 900000 + openMsecs_;
	for( vector<TradeInfo>::const_iterator it = trades_->begin(); it != trades_->end(); ++it )
	{
		if( it->msecs >= msecs_15min )
			break;
		//else if( it->msecs >= firstMsecs )
		else
		{
			if( it->price > high900_ )
				high900_ = it->price;
			if( it->price < low900_ )
				low900_ = it->price;
		}
	}

	// 1 second series.
	//get_trade_index_1s(vTindx1s_, trades_);
	//get_quote_index_1s(vQindx1s_, quotes_);
	//get_quote_index_1s(vSindx1s_, sip_);
	vTindx1s_ = get_index_1s(trades_, openMsecs, closeMsecs);
	vQindx1s_ = get_index_1s(quotes_, openMsecs, closeMsecs);
	vSindx1s_ = get_index_1s(sip_, openMsecs, closeMsecs);

	// Trade volume. In 1 second interval.
	vector<double> trdVolu1s(n1sec_);
	vector<double> sumTrdPrc1s(n1sec_);
	vector<double> nTrds1s(n1sec_);
	for( vector<TradeInfo>::const_iterator it = trades_->begin(); it != trades_->end(); ++it )
	{
		if( it->msecs <= openMsecs_ )
			continue;
		else if( it->msecs >= closeMsecs_ )
			break;
		else
		{
			int secIndx = (it->msecs - openMsecs_) / 1000 + 1;
			trdVolu1s[secIndx] += it->qty;
			++nTrds1s[secIndx];
			sumTrdPrc1s[secIndx] += it->price;
		}
	}
	get_cum(trdVolu1sCum_, trdVolu1s);
	get_cum(sumTrdPrc1sCum_, sumTrdPrc1s);
	get_cum(nTrds1sCum_, nTrds1s);

	fVolM600_ = float(n1sec_ - 1) / tm_volu_mom_lb_1_;
	fVolM3600_ = float(n1sec_ - 1) / tm_volu_mom_lb_2_;

	//
	// Quote.
	//
	// Mid quotes.
	get_mid_series(vMid1s_, quotes_, openMsecs, closeMsecs, 0, false);

	// overnight return.
	if(sig.adjPrevClose > 0.)
		ret_on_ = basis_pts_ * (vMid1s_[sec_on_] / sig.adjPrevClose - 1.);

	// Max size.
	vMaxBidSize1s_ = vector<int>(n1sec_);
	vMaxAskSize1s_ = vector<int>(n1sec_);

	//
	// Hedge object.
	//
	const vector<hff::SigH>* pvSigH_ = static_cast<const vector<hff::SigH>*>(MEvent::Instance()->get("", "vSigH"));
	if( pvSigH_ == 0 )
	{
		//if( nErrDay_++ < 1 )
		//	cout << "WARNING MMakeSample_at::get_prediction_at() Hedge info not available.\n";
	}
	else
	{
		for( vector<hff::SigH>::const_iterator it = pvSigH_->begin(); it != pvSigH_->end(); ++it )
		{
			if( it->ticker == ticker_ )
			{
				psigh_ = &(*it);
				break;
			}
		}
	}

	// Hedge signal.
	if( psigh_ != 0 )
	{
		sig.northBP = psigh_->northBP;
		sig.northRST = psigh_->northRST;
		sig.northTRD = psigh_->northTRD;
		sig.tarON = psigh_->tarON;
	}
}

SignalCalculator::~SignalCalculator()
{
}

void SignalCalculator::set_news(const std::vector<QuoteInfo>* news)
{
	news_ = news;
}

void SignalCalculator::debug_fillImbBook(TickSources& ts, const vector<string>& markets, int idate, const string& ticker)
{
	debug_fillImbBook_ = true;

	for( auto it = markets.begin(); it != markets.end(); ++it )
	{
		string market = *it;
		string mCode = mto::code(market);
		vector<string> dirs = ts.stocksdirectory(market, idate);

		TickAccessMulti<QuoteInfo> ta;
		for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
			ta.AddRoot(*itd, mto::longTicker(market));

		TickSeries<QuoteInfo> ts;
		ta.GetTickSeries(ticker, idate, &ts);

		mTob_[mCode].clear();
		vector<QuoteInfo>& vTob = mTob_[mCode];
		ts.StartRead();
		QuoteInfo quote;
		while( ts.Read(&quote) )
			vTob.push_back(quote);
	}
}

void SignalCalculator::debug_booktrade_prob()
{
	debug_booktrade_prob_ = true;
}

// -------------------------------------------------------------------------
//
// Callback functions.
//
// -------------------------------------------------------------------------

void SignalCalculator::NbboCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	if( !pSessions_->isAfterOpenBeforeClose(msecs) )
		return;

	// Last event time.
	msecsLastEvent_ = msecs;

	// Update last nbbo.
	lastNbbo_ = provider->Nbbo();

	// First quote.
	if( !debug_first_quote )
	{
		if( firstQuote_.msecs == 0 && msecs > openMsecs_ )
		{
			QuoteInfo nbbo = provider->Nbbo();
			if( valid_quote(nbbo) && nbbo.ask >= nbbo.bid )
			{
				firstQuote_ = nbbo;
				psig_->sprdOpen = get_sprd(firstQuote_.bid, firstQuote_.ask) * basis_pts_;
			}
		}
	}

	if( quoteSampleProbability >= 1. || (quoteSampleProbability >= 0. && Random::Instance()->select(quoteSampleProbability)) )
		NewSig(msecs, provider, nbbo_event_);
}

void SignalCalculator::TradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	if( !pSessions_->isAfterOpenBeforeClose(msecs) )
		return;

	// Last event time.
	msecsLastEvent_ = msecs;

	TradeInfo this_trade = provider->LastTrade();
	if( valid_trade(this_trade) )
	{
		// Update last trade.
		lastTrade_ = this_trade;
		lastFillImbOld_ = 2. * (lastTrade_.price - get_mid(lastNbbo_)) / (lastNbbo_.ask - lastNbbo_.bid);

		// Seen a trade after valid quote.
		if( !validt_ && firstQuote_.msecs > 0 )
			validt_ = 1;

		// Update high and low.
		if( lastTrade_.price > high_ )
			high_ = lastTrade_.price;
		if( lastTrade_.price < low_ )
			low_ = lastTrade_.price;

		if( tradeSampleProbability >= 1. || (tradeSampleProbability >= 0. && Random::Instance()->select(tradeSampleProbability)) )
			//if( Random::Instance()->select(tradeSampleProbability) )
			NewSig(msecs, provider, trade_event_);
	}
}

void SignalCalculator::BookTradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	if( !pSessions_->isAfterOpenBeforeClose(msecs) )
		return;

	// Last event time.
	msecsLastEvent_ = msecs;

	TradeInfo this_trade = provider->LastBookTrade();
	if( valid_trade(this_trade) )
	{
		// Update last book trade.
		lastBookTrade_ = this_trade;
		lastFillImb_ = get_fillImb(provider);

		// Seen a trade after valid quote.
		if( !validt_ && firstQuote_.msecs > 0 )
			validt_ = 1;

		bool newsig_ok = false;
		if( booktradeSampleProbability >= 1. || (booktradeSampleProbability > 0. && Random::Instance()->select(booktradeSampleProbability)) )
			newsig_ok = NewSig(msecs, provider, book_trade_event_);

		if( newsig_ok && mTrades_.find(msecs) == mTrades_.end() )
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

void SignalCalculator::RegularCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	if( !pSessions_->isAfterOpenBeforeClose(msecs) )
		return;

	NewSig(msecs, provider, regular_sample_);
}

void SignalCalculator::TimeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, void* ref)
{
	//int* p = reinterpret_cast<int*>(ref);

	//int* p = (int*)ref;
	//int iRef = *p;
	//delete p;

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

bool SignalCalculator::NewSig(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, int sampleType)
{
	if( !validt_ || firstQuote_.msecs <= 0 || !valid_quote(lastNbbo_) || msecs <= firstQuote_.msecs || /*msecs <= openMsecs_ || msecs >= closeMsecs_ ||*/ !pSessions_->inSessionStrict(msecs) )
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
	if( sampleType & book_trade_event_ || sampleType & exploratory_event_ )
		sI.sampleType = sampleType + trade_and_exploratory_event_;
	sI.mqut = get_mid(lastNbbo_);
	sI.bid = lastNbbo_.bid;
	sI.ask = lastNbbo_.ask;

	calculate_trade_signals(sig, sI, msecs);
	calculate_quote_signals(sig, sI, msecs, provider);
	calculate_fillimb_signals(sI, msecs, provider, sampleType);
	calculate_exploratory_signals(sI, msecs, provider, sampleType);
	calculate_targets(sig, sI, provider);
	calculate_tob_signals(sI, provider);
	calculate_book_signals(sig, sI, provider);
	calculate_news_signals(sig, sI, msecs);
	calculate_signals(sig, sI, msecs);

	if( fabs(sI.mret60) > om_max_ret_ )
		return false;
	if( sampleType & book_trade_event_ || sampleType & exploratory_event_ )
	{
		if( !linMod.use_us_arca_trade_event && string(1, lastBookTrade_.flags) == "P" )
			return false;
	}

	//if( sampleType == exploratory_event_ && fabs(sI.dswmds) < 1e-6 && fabs(sI.dswmdp) < 1e-6 )
	//	return false;

	// Update linear model and write binary signal files if gptOK.
	sI.gptOK = 1;

	// Write signal files if gptOK & inUnivFit.
	//if( sig.inUnivFit )
	//	sI.valid = 1;

	sig.sI.push_back(sI);

	provider->SetTimeCB( msecs + linMod.transientMsecs, (void*)(sig.sI.size() - 1) );
	return true;
}

void SignalCalculator::calculate_trade_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs)
{
	int sec = sI.msso / 1000;

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
	sI.voluMom600 = get_voluMom( msecs, openMsecs_, 600, trdVolu1sCum_, trades_, vTindx1s_ );
	sI.voluMom3600 = get_voluMom( msecs, openMsecs_, 3600, trdVolu1sCum_, trades_, vTindx1s_ );
	sI.intraVoluMom = get_voluMom( msecs, openMsecs_, 0, trdVolu1sCum_, trades_, vTindx1s_ );

	if(sig.avgDlyVolume > 0) // Normalize volume momentum signals.
	{
		sI.voluMom600  = sI.voluMom600 * fVolM600_ / sig.avgDlyVolume;
		sI.voluMom3600 = sI.voluMom3600 * fVolM3600_ / sig.avgDlyVolume;
		if( sec > 0 )
		{
			float scaleFact = float((n1sec_ - 1)) / sec;
			sI.intraVoluMom = sI.intraVoluMom * scaleFact / sig.avgDlyVolume;
		}
	}

	sI.bollinger300 = get_vwap_sig( msecs, openMsecs_, 300, sumTrdPrc1sCum_, nTrds1sCum_, trades_, vTindx1s_, 9 );
	sI.bollinger900 = get_vwap_sig( msecs, openMsecs_, 900, sumTrdPrc1sCum_, nTrds1sCum_, trades_, vTindx1s_, 9 );
	sI.bollinger1800 = get_vwap_sig( msecs, openMsecs_, 1800, sumTrdPrc1sCum_, nTrds1sCum_, trades_, vTindx1s_, 9 );
	sI.bollinger3600 = get_vwap_sig( msecs, openMsecs_, 3600, sumTrdPrc1sCum_, nTrds1sCum_, trades_, vTindx1s_, 9 );
}


double SignalCalculator::get_mret_upto1s(int msecs,
		int length_msec, int firstQtMsec, double firstMidQt, double mqut,
		TickProvider<TradeInfo, QuoteInfo, OrderData>* provider, int lag)
{
	double mret = 0.;
	if( msecs > firstQtMsec )
	{
		double midFrom = firstMidQt;
		int msecsFrom = msecs - length_msec - lag;
		if( msecsFrom > firstQtMsec )
		{
			QuoteInfo quoteFrom = provider->NbboAt(msecsFrom);
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

void SignalCalculator::calculate_quote_signals(hff::SigC& sig, hff::StateInfo& sI,
		int msecs, TickProvider<TradeInfo, QuoteInfo, OrderData>* provider)
{
	float firstMidQt = get_mid(firstQuote_);
	int sec = sI.msso / 1000;

	sI.sprd = basis_pts_ * get_sprd(lastNbbo_.bid, lastNbbo_.ask);

	sI.mret1 = get_mret_upto1s(msecs, 1000, firstQuote_.msecs, firstMidQt, sI.mqut, provider);
	sI.mret5 = get_mret_upto1s(msecs, 5000, firstQuote_.msecs, firstMidQt, sI.mqut, provider);
	sI.mret15 = get_mret_upto1s(msecs, 15000, firstQuote_.msecs, firstMidQt, sI.mqut, provider);
	sI.mret30 = get_mret_upto1s(msecs, 30000, firstQuote_.msecs, firstMidQt, sI.mqut, provider);
	sI.mret60 = get_mret_upto1s(msecs, 60000, firstQuote_.msecs, firstMidQt, sI.mqut, provider);

	sI.mret120 = get_mret_upto1s(msecs, 120000, firstQuote_.msecs, firstMidQt, sI.mqut, provider);
	sI.mret300 = get_mret_upto1s(msecs, 300000, firstQuote_.msecs, firstMidQt, sI.mqut, provider);
	sI.mret600 = get_mret_upto1s(msecs, 600000, firstQuote_.msecs, firstMidQt, sI.mqut, provider);
	sI.mret1200 = get_mret_upto1s(msecs, 1200000, firstQuote_.msecs, firstMidQt, sI.mqut, provider);
	sI.mret2400 = get_mret_upto1s(msecs, 2400000, firstQuote_.msecs, firstMidQt, sI.mqut, provider);
	sI.mret4800 = get_mret_upto1s(msecs, 4800000, firstQuote_.msecs, firstMidQt, sI.mqut, provider);
	sI.mret300L = get_mret_upto1s(msecs, 300000, firstQuote_.msecs, firstMidQt, sI.mqut, provider, 300000);
	sI.mret600L = get_mret_upto1s(msecs, 600000, firstQuote_.msecs, firstMidQt, sI.mqut, provider, 600000);
	sI.mret1200L = get_mret_upto1s(msecs, 1200000, firstQuote_.msecs, firstMidQt, sI.mqut, provider, 1200000);
	sI.mret2400L = get_mret_upto1s(msecs, 2400000, firstQuote_.msecs, firstMidQt, sI.mqut, provider, 2400000);
	sI.mret4800L = get_mret_upto1s(msecs, 4800000, firstQuote_.msecs, firstMidQt, sI.mqut, provider, 4800000);

	sI.sipDiffMid = get_dret(msecs, openMsecs_, firstQuote_.msecs, quotes_, vQindx1s_, sip_, vSindx1s_);

	sI.persistent = get_persistent(sec, 4, quotes_, vQindx1s_, sI);

	// overnight return.
	if( sec >= sec_on_ )
		sI.sig10[7] = ret_on_;
	else if( sig.adjPrevClose > 0 )
		sI.sig10[7] = basis_pts_ * ( sI.mqut / sig.adjPrevClose - 1.);
	if( fabs(sI.sig10[7]) > max_ret_)
		sI.sig10[7] = max_ret_ * sI.sig10[7] / fabs(sI.sig10[7]);

	// max sizes.
	sI.maxbsize = get_max_bidSize(msecs, openMsecs_, vMaxBidSize1s_, 200, quotes_, vQindx1s_);
	sI.maxasize = get_max_askSize(msecs, openMsecs_, vMaxAskSize1s_, 200, quotes_, vQindx1s_);
	sI.maxbsize2 = get_max_bidSize(msecs, openMsecs_, vMaxBidSize1s_, 1200, quotes_, vQindx1s_);
	sI.maxasize2 = get_max_askSize(msecs, openMsecs_, vMaxAskSize1s_, 1200, quotes_, vQindx1s_);
}

void SignalCalculator::calculate_fillimb_signals(hff::StateInfo& sI, int msecs,
		TickProvider<TradeInfo, QuoteInfo, OrderData>* provider, int sampleType)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	string model = MEnv::Instance()->model;

	//float fi = 0.;
	if( sampleType & book_trade_event_ || sampleType & exploratory_event_ )
	{
		if( msecs - lastBookTrade_.msecs < 60000 )
		{
			QuoteInfo quote = provider->Bbo( (unsigned char)lastBookTrade_.flags );
			string market = string(1, lastBookTrade_.flags);

			if( valid_quote(quote) && quote.ask > quote.bid )
			{
				// Fill imbalance.
				//fi = lastFillImb_;
				sI.sig1[1] = lastFillImb_;

				int vsize = vfi_.size();
				if( vsize > 0 )
					sI.fIm1 = vfi_[vsize - 1];
				if( vsize > 1 )
					sI.fIm2 = vfi_[vsize - 2];

				vfi_.push_back(lastFillImb_);

				// eventTakeRat.
				if( lastFillImb_ > 0. ) // Buy.
				{
					int levelSizeTot = 0;
					if( model.substr(0, 1) == "U" )
					{
						auto ecns = linMod.get_ecns();
						for( vector<string>::const_iterator ite = ecns.begin(); ite != ecns.end(); ++ite )
						{
							QuoteInfo tob = provider->Bbo( (unsigned char)(*ite)[1] );
							if( fabs(tob.ask - lastBookTrade_.price) < ltmb_ )
							{
								levelSizeTot += tob.askSize;
							}
						}
					}

					if( quote.askSize > 0 && fabs(quote.ask - lastBookTrade_.price) < ltmb_ )
						sI.eventTakeRatMkt = (float)lastBookTrade_.qty / quote.askSize;
					if( levelSizeTot > 0 )
						sI.eventTakeRatTot = (float)lastBookTrade_.qty / levelSizeTot;
				}
				else if( lastFillImb_ < 0. ) // Sell.
				{
					int levelSizeTot = 0;
					if( model.substr(0, 1) == "U" )
					{
						auto ecns = linMod.get_ecns();
						for( vector<string>::const_iterator ite = ecns.begin(); ite != ecns.end(); ++ite )
						{
							QuoteInfo tob = provider->Bbo( (unsigned char)(*ite)[1] );
							if( fabs(tob.bid - lastBookTrade_.price) < ltmb_ )
							{
								levelSizeTot += tob.bidSize;
							}
						}
					}

					if( quote.bidSize > 0 && fabs(quote.bid - lastBookTrade_.price) < ltmb_ )
						sI.eventTakeRatMkt = (float)lastBookTrade_.qty / quote.bidSize;
					if( levelSizeTot > 0 )
						sI.eventTakeRatTot = (float)lastBookTrade_.qty / levelSizeTot;
				}

				//// bestPostTrade.
				//bool wasBest = false;
				//bool willBeBest = true;
				//if( fi > 0. )
				//{
				//	bool temp_wasBest = true;
				//	for( vector<string>::const_iterator ite = linMod.ecns.begin(); ite != linMod.ecns.end(); ++ite )
				//	{
				//		if( market != (*ite) )
				//		{
				//			QuoteInfo tob = provider->Bbo( (unsigned char)(*ite)[0] );
				//			if( quote.ask - ltmb_ > tob.ask )
				//			{
				//				temp_wasBest = false;
				//				break;
				//			}
				//		}
				//	}
				//	wasBest = temp_wasBest;

				//	if( wasBest && lastBookTrade_.qty >= tob.askSize )
				//	{
				//		float price_next_level = 0.;

				//		for( vector<string>::const_iterator ite = linMod.ecns.begin(); ite != linMod.ecns.end(); ++ite )
				//		{
				//			if( market != (*ite) )
				//			{
				//				QuoteInfo tob = provider->Bbo( (unsigned char)(*ite)[0] );
				//				if( price_next_level - ltmb_ > tob.ask )
				//				{
				//					willBeBeset = false;
				//					break;
				//				}
				//			}
				//		}
				//	}
				//}
				//else if( fi < 0. )
				//{
				//}
				//if( wasBest )
				//{
				//	if( willBeBest )
				//		sI.bestPostTrade = 1.;
				//	else
				//		sI.bestPostTrade = 0.;
				//}
				//else
				//	sI.bestPostTrade = -1.;

				if( debug_fillImbBook_ )
				{
					string market = string(1, lastBookTrade_.flags);
					vector<QuoteInfo>& tobs = mTob_[market];
					QuoteInfo quote_tm10;
					QuoteInfo quote_tm5;
					QuoteInfo quote_t;
					QuoteInfo quote_tp1;
					for( vector<QuoteInfo>::iterator it = tobs.begin(); it != tobs.end(); ++it )
					{
						if( it->msecs <= msecs - 10 )
						{
							quote_tm10 = *it;
							quote_tm5 = *it;
							quote_t = *it;
							quote_tp1 = *it;
						}
						else if( it->msecs <= msecs - 5 )
						{
							quote_tm5 = *it;
							quote_t = *it;
							quote_tp1 = *it;
						}
						else if( it->msecs <= msecs )
						{
							quote_t = *it;
							quote_tp1 = *it;
						}
						else if( it->msecs <= msecs + 1 )
						{
							quote_tp1 = *it;
						}
						else
							break;
					}
					sI.fillImbTm10 = get_fillImb(quote_tm10, lastBookTrade_);
					sI.fillImbTm5 = get_fillImb(quote_tm5, lastBookTrade_);
					sI.fillImbT = get_fillImb(quote_t, lastBookTrade_);
					sI.fillImbTp1 = get_fillImb(quote_tp1, lastBookTrade_);
				}
			}
		}
	}
	else
	{
		if( msecs - lastTrade_.msecs < 60000 )
			//sI.sig1[1] = lastFillImbOld_;
			sI.sig1[1] = lastFillImb_;
		clip(sI.sig1[1], om_fill_imb_clip_);
	}
	//lastFillImb_ = fi;
}

void SignalCalculator::calculate_exploratory_signals(hff::StateInfo& sI, int msecs,
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

			//bool use_earlier_book_trade = false;
			//if( use_earlier_book_trade )
				sI.sig1[1] = trade.flags; // Override fillImb.
			//else
				//sI.sig1[1] = lastFillImb_;

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
				double dswmds = (Pa - Pb) * (dSbOL * SaOL - dSaOL * SbOL) / (Sa + Sb) / (Sa + Sb) / Pmid * basis_pts_;
				double dswmdsTop = (Pa - Pb) * (dSb * Sa - dSa * Sb) / (Sa + Sb) / (Sa + Sb) / Pmid * basis_pts_;
				double dswmdp = (Sa * dPb + Sb * dPa) / (Sa + Sb) / Pmid * basis_pts_;
				sI.dswmds = dswmds;
				sI.dswmdsTop = dswmdsTop;
				sI.dswmdp = dswmdp;
			}

			mTrades_.erase(itt);
			mCompBkTop_.erase(itq);
		}
	}
}

float SignalCalculator::get_fillImb(const QuoteInfo& quote, const TradeInfo& trade)
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

float SignalCalculator::get_fillImb(TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	QuoteInfo quote = provider->Bbo( (unsigned char)lastBookTrade_.flags );
	string market = string(1, lastBookTrade_.flags);
	if( valid_quote(quote) && quote.ask > quote.bid )
		return get_fillImb(quote, lastBookTrade_);
	return 0.;
}

void SignalCalculator::calculate_targets(hff::SigC& sig, hff::StateInfo& sI,
		TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const vector<pair<int, int> >& vHorizonLag = nonLinMod.vHorizonLag;
	int nT = vHorizonLag.size();
	int secNow = (sI.msso - 1) / 1000 + 1;
	for( int iT = 0; iT < nT; ++iT )
	{
		int horizon = vHorizonLag[iT].first;
		int lag = vHorizonLag[iT].second;

		//if( !exact_target_ ) // approx. up to 1 sec.
		//{
		//	int secFrom = min(secNow + lag, n1sec_ - 1);
		//	int secTo = min(secNow + lag + horizon, n1sec_ - 1);
		//	if( secFrom >= n1sec_ - 1 )
		//		secFrom = n1sec_ - 1;
		//	if( secTo >= n1sec_ - 1 )
		//		secTo = n1sec_ - 1;
		//	if(vMid1s_[secFrom] > 0. && vMid1s_[secTo] > 0.)
		//	{
		//		sI.target[iT] = basis_pts_ * (vMid1s_[secTo] / vMid1s_[secFrom] - 1.);

		//		// clip
		//		if( horizon >= 60 && horizon < 600 )
		//			clip(sI.target[iT], om_target_clip_);
		//		else if( horizon >= 600 && horizon < 3600 )
		//			clip(sI.target[iT], tm_target_clip_);
		//		else if( horizon >= 3600 )
		//			clip(sI.target[iT], tm_target_60_clip_);

		//		// Copy. Original will be hedged.
		//		sI.targetUH[iT] = sI.target[iT];
		//	}
		//}
		//else if( exact_target_ ) // exact.
		{
			float midFrom = sI.mqut;
			int msecsFrom = sI.msso + linMod.openMsecs + lag * 1000;
			if( lag > 0 )
			{
				QuoteInfo quoteFrom = provider->NbboAt(msecsFrom);
				midFrom = get_mid(quoteFrom);
			}

			int msecsTo = msecsFrom + horizon * 1000;
			QuoteInfo quoteTo = provider->NbboAt(msecsTo);
			float midTo = get_mid(quoteTo);

			if( midFrom > 0. && midTo > 0. )
			{
				sI.target[iT] = basis_pts_ * (midTo / midFrom - 1.);

				// clip.
				if( horizon >= 60 && horizon < 600 )
					clip(sI.target[iT], om_target_clip_);
				else if( horizon >= 600 && horizon < 3600 )
					clip(sI.target[iT], tm_target_clip_);
				else if( horizon >= 3600 )
					clip(sI.target[iT], tm_target_60_clip_);

				// Copy. Originals will be hedged.
				sI.targetUH[iT] = sI.target[iT];

				// Hedge.
			}
		}
	}

	// maxHorizon to close.
	//{
		//int maxHorizon = nonLinMod.maxHorizon;
		//int secFrom = min(secNow + maxHorizon, n1sec_ - 1);
		//int secTo = n1sec_ - 1;
		//if( secFrom >= n1sec_ - 1 )
			//secFrom = n1sec_ - 1;
		//if( secTo >= n1sec_ - 1 )
			//secTo = n1sec_ - 1;
		//if(vMid1s_[secFrom] > 0. && vMid1s_[secTo] > 0.)
			//sI.target60Intra = basis_pts_ * (vMid1s_[secTo] / vMid1s_[secFrom] - 1.);
	//}

	// to close.
	{
		int secFrom = secNow;
		int secTo = n1sec_ - 1;
		if( secFrom >= n1sec_ - 1 )
			secFrom = n1sec_ - 1;
		if( secTo >= n1sec_ - 1 )
			secTo = n1sec_ - 1;
		if(vMid1s_[secFrom] > 0. && vMid1s_[secTo] > 0.)
		{
			sI.targetClose = basis_pts_ * (vMid1s_[secTo] / vMid1s_[secFrom] - 1.);
			sI.targetNextClose = sI.targetClose + sig.closeNextCloseRet;
		}
	}

	// clip.
	//clip(sI.target60Intra, tm_target_60_clip_);
	clip(sI.targetClose, tm_target_60_clip_);
	clip(sI.targetNextClose, tm_target_60_clip_);

	// Copy. Originals will be hedged.
	//sI.target60IntraUH = sI.target60Intra;
	sI.targetCloseUH = sI.targetClose;
	sI.targetNextCloseUH = sI.targetNextClose;
}

void SignalCalculator::calculate_tob_signals(hff::StateInfo& sI,
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

void SignalCalculator::calculate_book_signals(hff::SigC& sig, hff::StateInfo& sI,
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
				calcBookDepthSigsMO(nLevels, &bidSide[0], nLevels, &askSide[0],
						mid_0, sprdOff_0, sig.avgDlyVolume, nVolFrac, volumeFrac, bookDepthMO);
				calcBookDepthSigsQI(nLevels, &bidSide[0], nLevels, &askSide[0],
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

void SignalCalculator::calculate_news_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs)
{
	if( news_ != 0 && !news_->empty() )
	{
		for( vector<QuoteInfo>::const_iterator it = news_->begin(); it != news_->end(); ++it )
		{
			int rel = it->bidEx;
			int css = it->bidSize;
			if( it->msecs > msecs )
				break;
			else if( it->msecs == msecs )
			{
				if( it->bidEx > 75 && css != 50 )
				{
					sI.tsincen = 0;
					sI.news = it->bidSize;
				}
			}
			else if( it->msecs < msecs )
			{
				if( it->bidEx > 75 && css != 50 )
				{
					sI.tsincen = msecs - it->msecs;
					sI.news = it->bidSize;
				}
			}
		}
	}
}

void SignalCalculator::calculate_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	float firstMidQt = get_mid(firstQuote_);

	sig.logVolu = log10(sig.avgDlyVolume);
	sig.logPrice = log10(sig.adjPrevClose);
	sig.logMedSprd = log10(sig.medSprd);

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

void SignalCalculator::get_top_comp_book(QuoteInfo& compBkTop, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
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

void SignalCalculator::get_dsOL(TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, QuoteInfo& quoteFrom, double& SaOL, double& SbOL)
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
