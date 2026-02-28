#include <MSigMod/MMakeSampleTP.h>
#include <MSigMod/SignalCalculatorTP.h>
#include <MFramework.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib_model/mFtns.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_signal/HedgeNF.h>
#include <gtlib_signal/HedgeNFadj.h>
#include <gtlib_signal/HedgeMarket.h>
#include <gtlib_signal/HedgeFullLen.h>
#include <gtlib_signal/HedgeFullLenNF.h>
#include <gtlib_signal/sigFtns.h>
#include <gtlib_signal/writeFtns.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <thread>
#include <algorithm>
using namespace std;
using namespace hff;
using namespace gtlib;

MMakeSampleTP::MMakeSampleTP(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	debugSigCal_(false),
	sample_all_(false),
	dayOK_(false),
	writeSigmoid_(false),
	preload_(true),
	earlySelectSig_(true),
	smoothTarget_(false),
	discardOutliers_(true),
	dropBadTarget_(false),
	doUniversalLinearModel_(false),
	doErrorCorrectionModel_(false),
	usUnivOverride_(false),
	removeHardToBorrow_(false),
	sampleMercatorTrade_(false),
	onlyMercatorTrade_(false),
	roundTradeSampleTimeMsecs_(false),
	allowNegSize_(false),
	sample_block_trade_(false),
	doSamplePendingUpdate_(true),
	allowNegSizeFrom_(0),
	euNegSizeTest_(false),
	persistAna_(false),
	requireTickValid_(true),
	requireTradeAfterFirstQuote_(true),
	rtrd_simple_def_(false),
	koreanTevtFilter_(false),
	allowEarlierQuote_(true),
	calculatePredSignal_(false),
	calculatePredSignalIt2_(false),
	removeInvalidSigsEarly_(false),
	descIt1_(""),
	sortTickers_(true),
	modelDateOmIt0_(0),
	modelDateTmIt0_(0),
	modelDateOmIt1_(0),
	sampling_("reg"),
	fitDesc_(MEnv::Instance()->fitDesc),
	schedType_(0),
	verbose_(0),
	iHedgeAlg_(2),
	nHedgeErrDay_(0),
	desiredSamplesOm_(0),
	desiredSamplesTm_(0),
	weight_source_("none"), // db, debugdb, disk, calculate, none (worldwide nolin)
	use_sigmoid_(false),
	write_ombin_reg_(false),
	write_ombin_tevt_(false),
	write_ombin_eevt_(false),
	write_ombin_cevt_(false),
	write_ombin_nevt_(false),
	write_tmbin_reg_(false),
	write_tmbin_tevt_(false),
	write_tmbin_eevt_(false),
	write_tmbin_cevt_(false),
	write_tmbin_nevt_(false),
	write_npz_(false),
	openDelay_(30),
	cntDay_(0),
	minMsecs_(0),
	maxMsecs_(86400000),
	filterHorizon_(0),
	filterLag_(0),
	fitterOmIt0_(nullptr), // prod 1 min model
	fitterTmIt0_(nullptr), // prod 40 min model
	fitterOmIt1_(nullptr),
	dpMilli_(nullptr),
	dataProvider_(nullptr),
	pEstTime_(nullptr)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debugSigCal") )
		debugSigCal_ = conf.find("debugSigCal")->second == "true";
	if( conf.count("sampleAll") )
		sample_all_ = conf.find("sampleAll")->second == "true";
	if( conf.count("writeSigmoid") )
		writeSigmoid_ = conf.find("writeSigmoid")->second == "true";
	if( conf.count("useSigmoid") )
		use_sigmoid_ = conf.find("useSigmoid")->second == "true";
	if( conf.count("sampleMercatorTrade") )
		sampleMercatorTrade_ = conf.find("sampleMercatorTrade")->second == "true";
	if( conf.count("onlyMercatorTrade") )
		onlyMercatorTrade_ = conf.find("onlyMercatorTrade")->second == "true";
	if( conf.count("sampleBlockTrade") )
		sample_block_trade_ = conf.find("sampleBlockTrade")->second == "true";
	if( conf.count("doSamplePendingUpdate") )
		doSamplePendingUpdate_ = conf.find("doSamplePendingUpdate")->second == "true";
	if( conf.count("requireTickValid") )
		requireTickValid_ = conf.find("requireTickValid")->second == "true";
	if( conf.count("requireTradeAfterFirstQuote") )
		requireTradeAfterFirstQuote_ = conf.find("requireTradeAfterFirstQuote")->second == "true";
	if( conf.count("roundTradeSampleTimeMsecs") )
		roundTradeSampleTimeMsecs_ = atoi(conf.find("roundTradeSampleTimeMsecs")->second.c_str());
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("sampling") )
		sampling_ = conf.find("sampling")->second;
	if( conf.count("schedType") )
		schedType_ = atoi(conf.find("schedType")->second.c_str());
	if( conf.count("openDelay") )
		openDelay_ = atoi(conf.find("openDelay")->second.c_str());
	if( conf.count("smoothTarget") )
		smoothTarget_ = conf.find("smoothTarget")->second == "true";
	if( conf.count("discardOutliers") )
		discardOutliers_ = conf.find("discardOutliers")->second == "true";
	if( conf.count("doUniversalLinearModel") )
		doUniversalLinearModel_ = conf.find("doUniversalLinearModel")->second == "true";
	if( conf.count("doErrorCorrectionModel") )
		doErrorCorrectionModel_ = conf.find("doErrorCorrectionModel")->second == "true";
	if( conf.count("usUnivOverride") )
		usUnivOverride_ = conf.find("usUnivOverride")->second == "true";
	if( conf.count("removeHardToBorrow") )
		removeHardToBorrow_ = conf.find("removeHardToBorrow")->second == "true";
	if( conf.count("preload") )
		preload_ = conf.find("preload")->second == "true";
	if( conf.count("rtrdSimpleDef") )
		rtrd_simple_def_ = conf.find("rtrdSimpleDef")->second == "true";
	if( conf.count("koreanTevtFilter") )
		koreanTevtFilter_ = conf.find("koreanTevtFilter")->second == "true";
	if( conf.count("allowEarlierQuote") )
		allowEarlierQuote_ = conf.find("allowEarlierQuote")->second == "true";
	if( conf.count("calculatePredSignal") )
		calculatePredSignal_ = conf.find("calculatePredSignal")->second == "true";
	if( conf.count("calculatePredSignalIt2") )
		calculatePredSignalIt2_ = conf.find("calculatePredSignalIt2")->second == "true";
	if( conf.count("sortTickers") )
		sortTickers_ = conf.find("sortTickers")->second == "true";
	if( conf.count("wmodel") )
		wmodel_ = conf.find("wmodel")->second;
	if( conf.count("coefModel") )
		coefModel_ = conf.find("coefModel")->second;
	if( conf.count("coefModelIt1") )
		coefModelIt1_ = conf.find("coefModelIt1")->second;
	if( conf.count("weightSource") )
		weight_source_ = conf.find("weightSource")->second;
	if( conf.count("writeOmBinReg") )
		write_ombin_reg_ = conf.find("writeOmBinReg")->second == "true";
	if( conf.count("writeOmBinTevt") )
		write_ombin_tevt_ = conf.find("writeOmBinTevt")->second == "true";
	if( conf.count("writeOmBinEevt") )
		write_ombin_eevt_ = conf.find("writeOmBinEevt")->second == "true";
	if( conf.count("writeOmBinCevt") )
		write_ombin_cevt_ = conf.find("writeOmBinCevt")->second == "true";
	if( conf.count("writeOmBinNevt") )
		write_ombin_nevt_ = conf.find("writeOmBinNevt")->second == "true";
	if( conf.count("writeTmBinReg") )
		write_tmbin_reg_ = conf.find("writeTmBinReg")->second == "true";
	if( conf.count("writeTmBinTevt") )
		write_tmbin_tevt_ = conf.find("writeTmBinTevt")->second == "true";
	if( conf.count("writeTmBinEevt") )
		write_tmbin_eevt_ = conf.find("writeTmBinEevt")->second == "true";
	if( conf.count("writeTmBinCevt") )
		write_tmbin_cevt_ = conf.find("writeTmBinCevt")->second == "true";
	if( conf.count("writeTmBinNevt") )
		write_tmbin_nevt_ = conf.find("writeTmBinNevt")->second == "true";
	if( conf.count("writeNpz") )
		write_npz_ = conf.find("writeNpz")->second == "true";
	if( conf.count("allowNegSize") )
		allowNegSize_ = conf.find("allowNegSize")->second == "true";
	if( conf.count("allowNegSizeFrom") )
		allowNegSizeFrom_ = atoi(conf.find("allowNegSizeFrom")->second.c_str());
	if( conf.count("euNegSizeTest") )
		euNegSizeTest_ = conf.find("euNegSizeTest")->second == "true";
	if( conf.count("persistAna") )
		persistAna_ = conf.find("persistAna")->second == "true";
	if( conf.count("removeInvalidSigsEarly") )
		removeInvalidSigsEarly_ = conf.find("removeInvalidSigsEarly")->second == "true";
	if( conf.count("minMsecs") )
		minMsecs_ = atoi(conf.find("minMsecs")->second.c_str());
	if( conf.count("maxMsecs") )
		maxMsecs_ = atoi(conf.find("maxMsecs")->second.c_str());
	write_om_ = write_ombin_reg_ || write_ombin_tevt_ || write_ombin_eevt_ || write_ombin_cevt_ || write_ombin_nevt_;
	write_reg_ = write_ombin_reg_ || write_tmbin_reg_;
	write_evt_ = write_ombin_tevt_ || write_ombin_eevt_ || write_ombin_cevt_ || write_ombin_nevt_
		|| write_tmbin_tevt_ || write_tmbin_eevt_ || write_tmbin_cevt_ || write_tmbin_nevt_;
}

MMakeSampleTP::~MMakeSampleTP()
{}

void MMakeSampleTP::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	string model = MEnv::Instance()->model;
	if( wmodel_.empty() )
		wmodel_ = model;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	const string& baseDir = MEnv::Instance()->baseDir;
	const vector<int>& allIdates = MEnv::Instance()->allIdates;
	uids_ = MEvent::Instance()->get<set<string> >("", "allUids");
	cout << uids_.size() << " instruments." << endl;
	tSources_.read(mto::region(MEnv::Instance()->markets[0]), MEnv::Instance()->sourceFlag);

	if(model[0] == 'U' || model[0] == 'E')
		removeHardToBorrow_ = true;

	if(model.substr(0,2) == "KR" && write_evt_)
		koreanTevtFilter_ = true;
}

void MMakeSampleTP::beginDay()
{
	cout << moduleName_ << "::beginDay()" << endl;
	nHedgeErrDay_ = 0;
	++cntDay_;
	int idate = MEnv::Instance()->idate;
	vector<string> markets = MEnv::Instance()->markets;
	const hff::LinearModel linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel nonLinMod = MEnv::Instance()->nonLinearModel;
	const string& model = MEnv::Instance()->model;
	mTickerUid_ = get_uid_map(markets, idate, uids_);
	const string& baseDir = MEnv::Instance()->baseDir;
	if( linMod.halt_tickers )
		statusChange_.beginDay(idate);

	if( sampling_ == "vol" )
		mercVol_.beginDay(model, idate);
	else if( sampling_ == "order" )
	{
		string market = markets[0];
		char cmd[1000];
		sprintf(cmd, "select symbol, ordermsecs from hforders where"
				" orderschedtype = %d and idate = %d"
				" order by symbol, ordermsecs", schedType_, idate);
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hfo(market, idate), cmd, vv);

		mSampleMsecs_.clear();
		vector<int> vMsecs;
		string prev_ticker;
		int prev_msecs = 0;
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			if(!prev_ticker.empty() && ticker != prev_ticker)
			{
				mSampleMsecs_[prev_ticker] = vMsecs;
				vMsecs.clear();
			}
			int msecs = atoi((*it)[1].c_str());
			if(msecs != prev_msecs)
				vMsecs.push_back(msecs);
			prev_msecs = msecs;
			prev_ticker = ticker;
		}
		if(!vMsecs.empty())
			mSampleMsecs_[prev_ticker] = vMsecs;
	}
	else if( sampling_ == "trade" )
	{
		string market = markets[0];
		char cmd[1000];
		sprintf(cmd, "select o.symbol, o.ordermsecs from hforders o inner join hforderevents e"
				" on o.orderid = e.orderid and o.idate = %d and e.idate = %d "
				" where o.orderschedtype = %d and e.qty > 0"
				" order by o.symbol, o.ordermsecs", idate, idate, schedType_);
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hfo(market, idate), cmd, vv);

		mSampleMsecs_.clear();
		vector<int> vMsecs;
		string prev_ticker;
		int prev_msecs = 0;
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			if(!prev_ticker.empty() && ticker != prev_ticker)
			{
				mSampleMsecs_[prev_ticker] = vMsecs;
				vMsecs.clear();
			}
			int msecs = atoi((*it)[1].c_str());
			if(msecs != prev_msecs)
				vMsecs.push_back(msecs);
			prev_msecs = msecs;
			prev_ticker = ticker;
		}
		if(!vMsecs.empty())
			mSampleMsecs_[prev_ticker] = vMsecs;
	}
	else if( sampling_ == "notrade" )
	{
		string market = markets[0];
		char cmd[1000];
		sprintf(cmd, "select o.symbol, o.ordermsecs from hforders o inner join hforderevents e"
				" on o.orderid = e.orderid and o.idate = %d and e.idate = %d"
				" where o.orderschedtype = %d"
				" group by o.symbol, o.ordermsecs"
				" having sum(e.qty) is null"
				" order by o.symbol, o.ordermsecs",
				idate, idate, schedType_);
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hfo(market, idate), cmd, vv);

		mSampleMsecs_.clear();
		vector<int> vMsecs;
		string prev_ticker;
		int prev_msecs = 0;
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			if(!prev_ticker.empty() && ticker != prev_ticker)
			{
				mSampleMsecs_[prev_ticker] = vMsecs;
				vMsecs.clear();
			}
			int msecs = atoi((*it)[1].c_str());
			if(msecs != prev_msecs)
				vMsecs.push_back(msecs);
			prev_msecs = msecs;
			prev_ticker = ticker;
		}
		if(!vMsecs.empty())
			mSampleMsecs_[prev_ticker] = vMsecs;
	}

	pSig_ = new vector<hff::SigC>();
	pHedge_ = getHedgeObject(model, idate);

	// okECNs.
	okECNs_.clear();
	auto ecns = linMod.get_ecns(idate);
	for( auto it = ecns.begin(); it != ecns.end(); ++it )
	{
		string market = *it;
		if( "US" == market || mto::dataOK(market, idate) )
			okECNs_.push_back(market);
	}

	bool doAfterHours = false;
	string market = markets[0];

	if(calculatePredSignal_)
	{
		{
			string targetName = "tar60;0";
			if( coefModel_.empty() )
				coefModel_ = model;
			string coefFitDir = get_fit_dir(baseDir, coefModel_, targetName, fitDesc_);

			int modelDateToUse = getModelDateToUse(idate, coefFitDir);
			if(modelDateOmIt0_ != modelDateToUse)
			{
				if(fitterOmIt0_ != nullptr)
					delete fitterOmIt0_;
				int nInput = 44;
				fitterOmIt0_ = new BoostedDecisionTree(nInput, getModelPath(coefFitDir, modelDateToUse));
				modelDateOmIt0_ = modelDateToUse;
			}
		}
		{
			string targetName = "tar600;60_1.0_tar3600;660_0.5";
			if( coefModel_.empty() )
				coefModel_ = model;
			string coefFitDir = get_fit_dir(baseDir, coefModel_, targetName, fitDesc_);

			int modelDateToUse = getModelDateToUse(idate, coefFitDir);
			if(modelDateTmIt0_ != modelDateToUse)
			{
				if(fitterTmIt0_ != nullptr)
					delete fitterTmIt0_;
				int nInput = 78;
				fitterTmIt0_ = new BoostedDecisionTree(nInput, getModelPath(coefFitDir, modelDateToUse));
				modelDateTmIt0_ = modelDateToUse;
			}
		}
		if(calculatePredSignalIt2_)
		{
			string targetName = "tar60;0";
			if( coefModelIt1_.empty() )
				coefModelIt1_ = model;
			string coefFitDir = get_fit_dir(baseDir, coefModelIt1_, targetName, fitDesc_);

			int modelDateToUse = getModelDateToUse(idate, coefFitDir);
			if(modelDateOmIt1_ != modelDateToUse)
			{
				if(fitterOmIt1_ != nullptr)
					delete fitterOmIt1_;
				int nInput = 44;
				fitterOmIt1_ = new BoostedDecisionTree(nInput, getModelPath(coefFitDir, modelDateToUse));
				modelDateOmIt1_ = modelDateToUse;
			}
		}
	}

	if( cntDay_ == 1 )
		pEstTime_ = new gtlib::EstTime(mTickerUid_.size());
}

int MMakeSampleTP::getModelDateToUse(int idate, const string& coefFitDir)
{
	vector<int> modelDates = getModelDates(coefFitDir);
	int dateToUse = 0;
	for( int modelDate : modelDates )
	{
		if( modelDate <= idate && modelDate > dateToUse )
			dateToUse = modelDate;
	}
	return dateToUse;
}

gtlib::Hedge* MMakeSampleTP::getHedgeObject(const string& model, int idate)
{
	string model02 = model.substr(0, 2);
	if( model02 == "US" )
		return new HedgeFullLenNF(idate);
	else
		return new HedgeFullLen();

	return nullptr;
}

void MMakeSampleTP::beginMarket()
{
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	const vector<string>& tickers = MEnv::Instance()->tickers;
	idate_ = idate;
	idate_p_ = prevClose(market, idate_);
	idate_p2_ = prevClose(market, idate_p_);
	idate_p3_ = prevClose(market, idate_p2_);
	idate_p4_ = prevClose(market, idate_p3_);
	idate_p5_ = prevClose(market, idate_p4_);
	idate_p6_ = prevClose(market, idate_p5_);
	idate_p7_ = prevClose(market, idate_p6_);
	idate_p8_ = prevClose(market, idate_p7_);
	idate_n_ = nextClose(market, idate_);
	dayOK_ = mto::dataOK(market, idate_);
	sessions_ = Sessions(market, idate_);
	auctionMkts_ = "";
	if( "US" == market )
		auctionMkts_ = "N";
	else if("E" == market.substr(0, 1))
	{
		for(string& m : MEnv::Instance()->markets)
		{
			auctionMkts_ += m.substr(1, 1);
		}
	}
	else if( "AT" == market )
		auctionMkts_ = "T";

	initTickProvider();

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	desiredSamplesOm_ = (linMod.closeMsecs - linMod.openMsecs) / (linMod.om_bin_sample_freq * 1000);
	desiredSamplesTm_ = (linMod.closeMsecs - linMod.openMsecs) / (linMod.tm_bin_sample_freq * 1000);
	if( sample_all_ )
	{
		desiredSamplesOm_ = max_int_;
		desiredSamplesTm_ = max_int_;
	}

	closeDBConnections(market);
}

void MMakeSampleTP::closeDBConnections(const string& market)
{
	GODBC::Instance()->close(mto::hf(market));

	// This is to save the database memory.
	if( "U" == mto::region(market) )
	{
		string db = "equitydata";
		GODBC::Instance()->close(db);
	}
	else
	{
		string db = mto::hf(market);
		GODBC::Instance()->close(db);
	}

}

void MMakeSampleTP::beginTicker(const string& ticker)
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;
	string model = MEnv::Instance()->model;
	string model2 = model.substr(0, 2);

	// If model is for multiple markets, skip the tickers from other markets.
	if(mto::isInternational(market) && market[1] != ticker[0])
		return;

	if( cntDay_ == 1 && pEstTime_ != nullptr )
	{
		boost::mutex::scoped_lock lock(est_mutex_);
		pEstTime_->beginTicker();
	}

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;

	auto itT = mTickerUid_.find(ticker);
	if( itT != mTickerUid_.end() )
	{
		string uid = itT->second;
		SigC sig;

		// Read from stockcharacteristics table.
		const hff::CharaContainer* ch = static_cast<const hff::CharaContainer*>(MEvent::Instance()->get("", "CharaContainer"));
		bool charaok = read_chara_data(ch, sig, model, market, uid, linMod.on_target_clip, removeHardToBorrow_, requireTickValid_,
				idate_, idate_p_, idate_p2_, idate_p3_, idate_p4_, idate_p5_, idate_p6_, idate_p7_, idate_p8_,  idate_n_);
		if( charaok && sig.avgDlyVolume > 0. && sig.avgDlyVolat > 0. )
		{
			const vector<OrderQty>* tradeQty = static_cast<const vector<OrderQty>*>(MEvent::Instance()->get(ticker, "tradeQty"));
			SignalCalculatorTP sigcal(idate, uid, ticker, sig, &sessions_, linMod.openMsecs, linMod.closeMsecs, tradeQty, linMod.exploratoryDelay);
			if(debugSigCal_)
				sigcal.setDebug();
			sigcal.setDiscardOutliers(discardOutliers_);
			sigcal.setDropBadTarget(dropBadTarget_);
			if( linMod.halt_tickers )
				sigcal.setStatusChange(&statusChange_);
			if(openDelay_ > 0)
				sigcal.setOpenDelay(openDelay_);
			if(sample_block_trade_)
				sigcal.sampleBlockTrade();
			if(linMod.excludeBlockTrade)
				sigcal.excludeBlockTrade();
			if(persistAna_)
				sigcal.doPersistAna();
			sigcal.samplePendingUpdate(doSamplePendingUpdate_);
			sigcal.requireTradeAfterFirstQuote(requireTradeAfterFirstQuote_);
			sigcal.setRtrdSimpleDef(rtrd_simple_def_);

			vector<int> vSampleMsecs;
			if( write_ombin_reg_ || write_tmbin_reg_ )
			{
				if(sampleMercatorTrade_)
				{
					const vector<int>* mercatorTradeTime = static_cast<const vector<int>*>(MEvent::Instance()->get(ticker, "mercatorTradeTime"));
					vSampleMsecs = getSampleMsecsTicker(*mercatorTradeTime, idate, ticker, sig.avgDlyVolat / basis_pts_);
				}
				else
					vSampleMsecs = getSampleMsecsTicker(idate, ticker, sig.avgDlyVolat / basis_pts_);
				int tmSampleScale = getTmSampleScale();
				sigcal.setTmSampleMsecs(vSampleMsecs, tmSampleScale);
			}

			// Booktrade sampling.
			if( write_evt_ )
			{
				if(write_ombin_nevt_ || write_tmbin_nevt_)
				{
					if(sample_all_)
						sigcal.setQuoteSampleStep(1);
					else
					{
						const vector<QuoteInfo>* quotes = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "nbbo"));
						int qStep = quotes->size() / desiredSamplesOm_ / 2;
						sigcal.setQuoteSampleStep(max(1, qStep));
					}
				}

				if(write_ombin_tevt_ || write_tmbin_tevt_)
				{
					if(sample_all_)
						sigcal.setBooktradeSampleStep(1);
					else
					{
						const vector<TradeInfo>* trades = static_cast<const vector<TradeInfo>*>(MEvent::Instance()->get(ticker, "trades"));
						int tStep = trades->size() / desiredSamplesOm_ / 3;
						sigcal.setBooktradeSampleStep(max(1, tStep));
					}
				}

				if( write_ombin_eevt_ || write_ombin_cevt_ )
					sigcal.doExploratory = true;
			}

			sigcal.setTradeSampleStep(0);
			TCM_classic tcmc(ticker, &sigcal, linMod.openMsecs, linMod.closeMsecs, auctionMkts_);
			if(idate <= linMod.allowNegSizeTo)
			{
				TCS_classic* tcsc = tcmc.SingleConverter();
				if(tcsc != nullptr) tcmc.SingleConverter()->SetAllowNegSize(true);
			}
			tcmc.SetRegularCB(vSampleMsecs);

			TickProviderMulti<UsecsTime, OrderDataMicro> provider;
			provider.AddConsumer(&tcmc);
			{
				boost::mutex::scoped_lock lock(load_mutex_);
				provider.LoadData(idate, ticker, dataProvider_);
			}

			provider.Run();
			sigcal.endTicker();

			if(koreanTevtFilter_)
			{
				sigcal.koreanTevtFilter(sig, allowEarlierQuote_);
			}

			if( pHedge_ != nullptr )
			{
				boost::mutex::scoped_lock lock(hedge_mutex_);
				if( iHedgeAlg_ == 0 ) // Use tcmc.
					pHedge_->updateTicker(ticker, sig.inUnivFit, sig.tarON, sig.tarClcl, linMod.openMsecs, linMod.closeMsecs, sigcal.getFirstTrade(), tcmc);
				else if( iHedgeAlg_ == 1 || iHedgeAlg_ == 3 || iHedgeAlg_ == 4 ) // Use my sample.
					pHedge_->updateTicker(ticker, sig.inUnivFit, sig.tarON, sig.tarClcl, linMod.openMsecs, linMod.closeMsecs, sigcal.getFirstTrade(), sigcal.getQuotes());
				else if( iHedgeAlg_ == 2 ) // Pass linMod and nonLinMod.
					pHedge_->updateTicker(linMod, nonLinMod, ticker, sig.inUnivFit, sig.close, sig.tarON, sig.tarClcl, sigcal.getFirstTrade(), sigcal.getQuotes(), smoothTarget_);
			}

			// Use local quotes sample. Do not use TickProvider.NbboAt().
			sigcal.calculate_targets(sig, sigcal.getQuotes(), smoothTarget_);
			const vector<QuoteInfo>* quotesNext = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "nbbo+1"));
			if(quotesNext != nullptr)
				sigcal.calculate_targets_next(sig, *quotesNext);
			sigcal.calculate_exchange_vol(sig);

			if(calculatePredSignal_)
			{
				calculate_pred_signals(sig);
				if(calculatePredSignalIt2_)
					calculate_pred_signals_it2(sig);
			}

			{
				boost::mutex::scoped_lock lock(sig_mutex_);
				if(verbose_ > 1)
					printf("%s sig cnt %d\n", ticker.c_str(), sig.sI.size());
				if( earlySelectSig_ )
					selectSig(sig);
				(*pSig_).push_back(std::move(sig));
			}
		}
	}

	// with MT
}

void MMakeSampleTP::endTicker(const string& ticker)
{
}

void MMakeSampleTP::endMarket()
{
	if( dpMilli_ != nullptr )
	{
		delete dpMilli_;
		dpMilli_ = nullptr;
	}
}

void MMakeSampleTP::endDay()
{
	if( cntDay_ == 1 && pEstTime_ != nullptr )
		pEstTime_->beginEndDay();

	if( nHedgeErrDay_ > 0 )
		cout << "WARNING MMakeSampleTP::endDay(): " << nHedgeErrDay_ << " tickers could not be hedged.\n";

	const LinearModel& linMod = MEnv::Instance()->linearModel;
	int idate = MEnv::Instance()->idate;
	const string& baseDir = MEnv::Instance()->baseDir;
	const string& model = MEnv::Instance()->model;
	//if( pHedge_ != nullptr )
		//pHedge_->calculateHedge(linMod.n1sec);
	calculate_hedge(pHedge_, pSig_);
	write_signal(pSig_);
	if(write_npz_)
	{
		//string np_path = get_np_path(baseDir, model, idate, "reg");
		string np_path = get_sig_path(baseDir, model, idate, "np", "reg");
		string np_txt_path = get_sigTxt_path(baseDir, model, idate, "np", "reg");
		write_npz(np_path, np_txt_path, pSig_, linMod, mTickerUid_);
	}

	if( cntDay_ == 1 && pEstTime_ != nullptr )
	{
		pEstTime_->endDay();
		delete pEstTime_;
	}
	delete pHedge_;
	delete pSig_;
}

void MMakeSampleTP::endJob()
{
	if(fitterOmIt0_ != nullptr)
		delete fitterOmIt0_;
	if(fitterTmIt0_ != nullptr)
		delete fitterTmIt0_;
	if(fitterOmIt1_ != nullptr)
		delete fitterOmIt1_;
}

vector<int> MMakeSampleTP::mergeRegTradeTime(const vector<int>& regTime, const vector<int>& tradeTime)
{
	int regSize = regTime.size();
	int tradeSize = tradeTime.size();

	int nMaxTradeTime = regSize / 2.;
	vector<int> sampledTradeTime;
	if(nMaxTradeTime < tradeSize)
	{
		float resample = tradeSize / nMaxTradeTime;
		for(float i = 0; i < tradeSize; i += resample)
		{
			int indx = static_cast<int>(i);
			sampledTradeTime.push_back(tradeTime[indx]);
		}
	}
	else
		sampledTradeTime = tradeTime;
	vector<int> ret = sampledTradeTime;

	if(!onlyMercatorTrade_)
	{
		int regIndx = 0;
		set<int> indxReplace;
		for(int tradeMsecs : tradeTime)
		{
			int minAbsDiff = 0;
			int closestIndx = 0;
			for(; regIndx < regSize; ++regIndx)
			{
				if(indxReplace.count(regIndx))
					continue;
				int regMsecs = regTime[regIndx];
				int diff = tradeMsecs - regMsecs;
				if(minAbsDiff == 0 || abs(diff) < minAbsDiff)
				{
					minAbsDiff = abs(diff);
					closestIndx = regIndx;
				}
				if(diff > 0)
				{
					--regIndx;
					break;
				}
			}
			indxReplace.insert(closestIndx);
			while(indxReplace.count(regIndx))
				--regIndx;
			if(regIndx < 0)
				regIndx = 0;
		}
		for(int i = 0; i < regSize; ++i)
			if(!indxReplace.count(i))
				ret.push_back(regTime[i]);
		sort(begin(ret), end(ret));
	}


	return ret;
}

vector<int> MMakeSampleTP::getSampleMsecsTicker(vector<int> tradeTime, int idate, const string& ticker, float medVolat)
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;

	if(roundTradeSampleTimeMsecs_ > 0)
	{
		for(int& t : tradeTime)
			t = t / roundTradeSampleTimeMsecs_ * roundTradeSampleTimeMsecs_;
	}

	vector<int> sampleMsecs;
	if( write_reg_ )
	{
		if( sampling_ == "reg" )
		{
			if(onlyMercatorTrade_)
				sampleMsecs = tradeTime;
			else
			{
				int stepSec = 0;
				if( write_ombin_reg_ )
					stepSec = linMod.om_bin_sample_freq;
				else if( write_tmbin_reg_ )
					stepSec = linMod.tm_bin_sample_freq;
				if(stepSec > 0)
				{
					sampleMsecs = getSampleMsecs(linMod.openMsecs, linMod.closeMsecs, stepSec);
					if(!tradeTime.empty())
						sampleMsecs = mergeRegTradeTime(sampleMsecs, tradeTime);
				}
			}
		}
	}
	return sampleMsecs;
}

vector<int> MMakeSampleTP::getSampleMsecsTicker(int idate, const string& ticker, float medVolat)
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;

	int stepSec = 0;
	if( write_ombin_reg_ )
		stepSec = linMod.om_bin_sample_freq;
	else if( write_tmbin_reg_ )
		stepSec = linMod.tm_bin_sample_freq;

	vector<int> sampleMsecs;
	if( stepSec > 0 && write_reg_ )
	{
		if( sampling_ == "irreg" )
			sampleMsecs = getSampleMsecsIrreg(ticker, idate, linMod.openMsecs, linMod.closeMsecs, stepSec);
		else if( sampling_ == "vol" )
			sampleMsecs = getSampleMsecs(mercVol_, ticker, linMod.openMsecs, linMod.closeMsecs, stepSec);
		else if( sampling_ == "volat" )
		{
			const vector<TradeInfo>* trades = static_cast<const vector<TradeInfo>*>(MEvent::Instance()->get(ticker, "trades"));
			sampleMsecs = getSampleMsecsVolat(linMod.openMsecs, linMod.closeMsecs, stepSec, medVolat, trades, sessions_);
		}
		else if( sampling_ == "order" || sampling_ == "trade" || sampling_ == "notrade")
			sampleMsecs = mSampleMsecs_[ticker];
		else if( sampling_ == "reg" )
			sampleMsecs = getSampleMsecs(linMod.openMsecs, linMod.closeMsecs, stepSec);
	}
	return sampleMsecs;
}

int MMakeSampleTP::getTmSampleScale()
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	int scale = 1;
	if( write_ombin_reg_ )
		scale = linMod.tm_bin_sample_freq / linMod.om_bin_sample_freq;
	return scale;
}

//
// Which signal to write.
//

void MMakeSampleTP::selectSig(SigC& sig)
{
	if( write_ombin_reg_ || write_tmbin_reg_ )
		select_sample_type("reg", regular_sample_, sig);
	if( write_ombin_tevt_ || write_tmbin_tevt_ )
		select_sample_type("tevt", book_trade_event_, sig);
	if( write_ombin_eevt_ || write_tmbin_eevt_ )
		select_sample_type("eevt", exploratory_event_, sig);
	if( write_ombin_cevt_ || write_tmbin_cevt_ )
		select_sample_type("cevt", trade_and_exploratory_event_, sig);
	if( write_ombin_nevt_ || write_tmbin_nevt_ )
		select_sample_type("nevt", nbbo_event_, sig);

	// remove invalid sigs.
	if(removeInvalidSigsEarly_)
	{
		sig.sI.erase(remove_if(sig.sI.begin(), sig.sI.end(),
					[](StateInfo x){return x.valid == 0 && x.validTm == 0;}), sig.sI.end());
	}
}

void MMakeSampleTP::select_sample_type(const string& desc, int sampleType, SigC& sig)
{
	if( sig.inUnivFit )
		select_events(sig.sI, sampleType);
}

void MMakeSampleTP::select_events(vector<StateInfo>& sI, int sampleType)
{
	if( regular_sample_ & sampleType )
		select_events_reg(sI);
	else
		select_events_evt(sI, sampleType);
}

void MMakeSampleTP::select_events_reg(vector<StateInfo>& sI)
{
	for( auto it = sI.begin(); it != sI.end(); ++it )
	{
		if( regular_sample_ & it->sampleType )
		{
			bool sprdOK = fabs(it->sprd) >= om_tree_min_fit_sprd_ && fabs(it->sprd) < om_tree_max_fit_sprd_;
			if( !discardOutliers_ || (it->gptOK && sprdOK) )
			{
				it->valid = 1;
				if( it->isTmSample )
					it->validTm = 1;
			}
		}
	}
}

void MMakeSampleTP::select_events_evt(vector<StateInfo>& sI, int sampleType)
{
	vector<int> vIndexTot = getGoodDataIndex(sI, sampleType);
	vector<int> vIndexOmWrite = getSampledIndex(vIndexTot, desiredSamplesOm_);
	vector<int> vIndexTmWrite = getSampledIndex(vIndexOmWrite, desiredSamplesTm_);

	if(verbose_ > 1)
	{
		printf("index cnt %d %d %d\n", vIndexTot.size(), vIndexOmWrite.size());
	}

	for( int i : vIndexOmWrite )
		sI[i].valid = 1;
	for( int i : vIndexTmWrite )
		sI[i].validTm = 1;
}

vector<int> MMakeSampleTP::getGoodDataIndex(vector<StateInfo>& sI, int sampleType)
{
	vector<int> vIndexTot;
	auto sIBeg = begin(sI);
	int last_msso = 0;
	for( auto it = begin(sI); it != end(sI); ++it )
	{
		if( sampleType & it->sampleType )
		{
			it->valid = 0;
			it->validTm = 0;
			bool sprdOK = fabs(it->sprd) >= om_tree_min_fit_sprd_ && fabs(it->sprd) < om_tree_max_fit_sprd_;
			bool persists = it->bidPersists && it->askPersists;
			bool persistsOK = !(it->sampleType & book_trade_event_) || persists; // don't check persists for reg-sample
			if( persistsOK && it->msso > last_msso )
			{
				if(!discardOutliers_ || it->gptOK && sprdOK)
					vIndexTot.push_back(distance(sIBeg, it));
			}
			last_msso = it->msso;
		}
	}
	return vIndexTot;
}

vector<int> MMakeSampleTP::getSampledIndex(const vector<int>& vIndexTot, int desiredSamples)
{
	vector<int> sampledIndex;
	double tot = vIndexTot.size();
	if( tot > 0. )
	{
		if( tot > desiredSamples )
		{
			double interval = tot / desiredSamples;
			for( int i = 0; i < desiredSamples; ++i )
				sampledIndex.push_back(vIndexTot[static_cast<int>(i * interval)]);
		}
		else
			sampledIndex = vIndexTot;
	}
	return sampledIndex;
}

//
// Write.
//

void MMakeSampleTP::calculate_hedge(gtlib::Hedge* pHedge, std::vector<hff::SigC>* pSig)
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	if( pHedge != nullptr )
	{
		pHedge->calculateHedge(linMod.n1sec);
		for( auto& sig : (*pSig) )
		{
			if( sig.inUnivFit == 1 )
			{
				const TargetHedger* pHedger = pHedge->getTargetHedger(sig.ticker);
				get_hedged_targets(sig, pHedger);
			}
		}
	}

}

void MMakeSampleTP::write_signal(std::vector<hff::SigC>* pSig, bool enough_day_ombin)
{
	int idate = MEnv::Instance()->idate;
	const string& baseDir = MEnv::Instance()->baseDir;
	const string& model = MEnv::Instance()->model;
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	const NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;

	if( (write_ombin_reg_ && enough_day_ombin) || write_tmbin_reg_ )
		write_sample_type(write_ombin_reg_, write_tmbin_reg_, idate, "reg", regular_sample_, pSig);
	if( write_ombin_tevt_ || write_tmbin_tevt_ )
		write_sample_type(write_ombin_tevt_, write_tmbin_tevt_, idate, "tevt", book_trade_event_, pSig);
	if( write_ombin_eevt_ || write_tmbin_eevt_ )
		write_sample_type(write_ombin_eevt_, write_tmbin_eevt_, idate, "eevt", exploratory_event_, pSig);
	if( write_ombin_cevt_ || write_tmbin_cevt_ )
		write_sample_type(write_ombin_cevt_, write_tmbin_cevt_, idate, "cevt", trade_and_exploratory_event_, pSig);
	if( write_ombin_nevt_ || write_tmbin_nevt_ )
		write_sample_type(write_ombin_nevt_, write_tmbin_nevt_, idate, "nevt", nbbo_event_, pSig);
}

void MMakeSampleTP::write_sample_type(bool write_om, bool write_tm, int idate,
		const string& desc, int sampleType, vector<SigC>* pSig)
{
	const string& baseDir = MEnv::Instance()->baseDir;
	const string& model = MEnv::Instance()->model;
	if( !earlySelectSig_ )
	{
		for( auto& sig : (*pSig) )
		{
			if( sig.inUnivFit )
				select_events(sig.sI, sampleType);
		}
	}
	if(write_om)
		write_signal_file(baseDir, model, idate, "om", desc, sampleType, pSig);
	if(write_tm)
		write_signal_file(baseDir, model, idate, "tm", desc, sampleType, pSig);
}

void MMakeSampleTP::write_signal_file(const string& baseDir, const string& model, int idate,
		const string& sigType, const string& desc, int sampleType, vector<SigC>* pSig)
{
	// Check if there is any data
	double cntAllSampleType = 0.;
	for(auto& sig : (*pSig))
	{
		for(int j = 0; j < sig.sI.size(); ++j)
		{
			const StateInfo& sI = sig.sI[j];
			if( sI.sampleType & sampleType && sI.valid )
				++cntAllSampleType;
		}
		if(cntAllSampleType > 100)
			break;
	}
	if(cntAllSampleType < 10)
		return;

	const LinearModel& linMod = MEnv::Instance()->linearModel;
	const NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	ofstream binReg( get_sig_path(baseDir, model, idate, sigType, desc).c_str(), ios::out | ios::binary );
	ofstream binTxtReg( get_sigTxt_path(baseDir, model, idate, sigType, desc).c_str(), ios::out );
	write_bin_header(sigType, binReg, binTxtReg, linMod, nonLinMod);
	int cnt = 0;

	if(sortTickers_)
	{
		vector<string> wTickers;
		for( auto& sig : (*pSig) )
		{
			const string& ticker = sig.ticker;
			wTickers.push_back(ticker);
		}
		int n = wTickers.size();
		vector<int> sortedIndex(n);
		gsl_heapsort_index<int, string>(sortedIndex, wTickers);
		for(int i : sortedIndex)
		{
			auto& sig = (*pSig)[i];
			if(sig.inUnivFit)
			{
				const string& ticker = sig.ticker;
				const string uid = mTickerUid_[ticker];
				cnt += write_bin_evt(sigType, binReg, binTxtReg, sig, uid, ticker,
						minMsecs_, maxMsecs_, sampleType, linMod, nonLinMod);
			}
		}

	}
	else
	{
		for( auto& sig : (*pSig) )
		{
			if( sig.inUnivFit )
			{
				const string& ticker = sig.ticker;
				const string& uid = mTickerUid_[sig.ticker];
				int cntTicker = write_bin_evt(sigType, binReg, binTxtReg, sig, uid, ticker,
						minMsecs_, maxMsecs_, sampleType, linMod, nonLinMod);
				if(verbose_ > 1)
					cout << "file cnt " << ticker << " " << cntTicker << endl;
				cnt += cntTicker;
			}
		}
	}
	finish_file(true, binReg, binTxtReg, cnt);
}

void MMakeSampleTP::finish_file(bool do_write, ofstream& ofbin, ofstream& ofbintxt, int& cnt)
{
	if( do_write )
	{
		update_ncols(ofbin, cnt);

		ofbin.close();
		ofbin.clear();

		ofbintxt.close();
		ofbintxt.clear();

		cnt = 0;
	}
}

void MMakeSampleTP::initTickProvider()
{
	string model = MEnv::Instance()->model;
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;

	if( dpMilli_ != nullptr )
	{
		delete dpMilli_;
		dpMilli_ = nullptr;
	}
	dpMilli_ = new TickDataProviderMilli<TradeInfo, OrderDataMicro>();

	// nbbodir.
	// linMod.primaryOnly
	vector<string> dirs = (linMod.primary_only) ? tSources_.bookdirectory(market, idate) : tSources_.nbbodirectory(market, idate);
	for( auto& dir : dirs )
		dpMilli_->AddTradeRoot(dir);

	// bookdir.
	if( "US" == market )
	{
		auto ecns = linMod.get_ecns(idate);
		for( auto& ecn : ecns )
		{
			vector<string> dirs = tSources_.bookdirectory(ecn, idate);
			for( auto& dir : dirs )
				dpMilli_->AddBookRoot(ecn[1], dir);
		}
	}
	else
	{
		// main market.
		vector<string> dirs = tSources_.bookdirectory(market, idate);
		if(euNegSizeTest_ && idate >= 20180912 && idate <= 20190610)
		{
			string backfill = "/mnt/gf1/tickC/eu_backfill/" + mto::code(market);
			bool useForBookTrades = false;
			dpMilli_->AddBookRoot(mto::code(market)[0], backfill, useForBookTrades);
			for( auto& dir : dirs )
				dpMilli_->AddBookTradeRoot(mto::code(market)[0], dir);
		}
		else if(model == "AS_test_L2trade")
		{
			bool useForBookTrades = false;
			for( auto& dir : dirs )
				dpMilli_->AddBookRoot(mto::code(market)[0], dir, useForBookTrades);
			string l2Trd = "/mnt/gf1/tickC/asia_trade/binLogL2/S";
			dpMilli_->AddBookTradeRoot('S', l2Trd);
		}
		else
		{
			for( auto& dir : dirs )
				dpMilli_->AddBookRoot(mto::code(market)[0], dir);
		}

		// ecns.
		// linMod.primaryOnly // redundant.
		if(!linMod.primary_only)
		{
			for( auto& ecn : okECNs_ )
			{
				vector<string> dirs = tSources_.bookdirectory(ecn, idate);
				if(euNegSizeTest_ && idate >= 20180912 && idate <= 20190610 && ecn == "ET")
				{
					string backfill = "/mnt/gf1/tickC/eu_backfill/" + ecn.substr(1,1);
					bool useForBookTrades = false;
					dpMilli_->AddBookRoot(ecn[1], backfill, useForBookTrades);
					for( auto& dir : dirs )
						dpMilli_->AddBookTradeRoot(ecn[1], dir);
				}
				else
				{
					for( auto& dir : dirs )
						dpMilli_->AddBookRoot(ecn[1], dir);
				}
			}
		}
	}
	if( preload_ )
	{
		if( cntDay_ == 1 )
			cout << market << " Begin preload " << getTimerInfoSimple() << endl;
		dpMilli_->PreloadData(idate, MEnv::Instance()->tickers);
		if( cntDay_ == 1 )
			cout << market << " End preload " << getTimerInfoSimple() << endl;
	}
	dataProvider_ = dpMilli_;
}

void MMakeSampleTP::get_hedged_targets(SigC& sig, const TargetHedger* pHedger)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	vector<hff::StateInfo>& sI = sig.sI;
	int Npts = sI.size();

	// Hedge object.
	if( pHedger == nullptr )
		++nHedgeErrDay_;
	else
	{
		if(!linMod.nfFromHfuniv)
		{
			sig.northBP = pHedger->getNorthBP();
			sig.northRST = pHedger->getNorthRST();
			sig.northTRD = pHedger->getNorthTRD();
		}
		sig.tarONuh = sig.tarON;
		sig.tarON = pHedger->getHedgedTarON(sig.tarON);
		sig.tarClcluh = sig.tarClcl;
		sig.tarClcl = pHedger->getHedgedTarClcl(sig.tarClcl);

		for( int k = 0; k < Npts; ++k )
		{
			int sec = sI[k].msso / 1000;
			for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
			{
				int len = nonLinMod.vHorizonLag[iT].first;
				int lag = nonLinMod.vHorizonLag[iT].second;
				sI[k].target[iT] = pHedger->getHedgedTarget(sI[k].target[iT], sec, iT, len, lag);
			}

			{
				sI[k].target11Close = pHedger->getHedgedTarget11Close(sI[k].target11Close, sec);
				sI[k].target71Close = pHedger->getHedgedTarget71Close(sI[k].target71Close, sec);
				sI[k].targetClose = pHedger->getHedgedTargetClose(sI[k].targetClose, sec);
			}

			sI[k].targetNextClose = 0.;
		}
	}
}

void MMakeSampleTP::calculate_pred_signals(SigC& sig)
{
	vector<vector<float>> vvInputOm = getOmInputsIt0(sig);
	vector<float> vPredOm = fitterOmIt0_->getPred(vvInputOm);

	vector<vector<float>> vvInputTm = getTmInputsIt0(sig);
	vector<float> vPredTm = fitterTmIt0_->getPred(vvInputTm);

	int n = sig.sI.size();
	for(int i = 0; i < n; ++i)
	{
		auto& sI = sig.sI[i];
		int msso = sI.msso;
		sI.predOm = vPredOm[i];
		sI.predTm = vPredTm[i];
		int j = i;
		
		// 30 sec
		while(j >= 0 && sig.sI[j].msso > msso - 30*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm30s = sig.sI[j].predOm;
			sI.predTm30s = sig.sI[j].predTm;
			sI.mret30predOm = sI.mret30 - sig.sI[j].predOm;
		}
		
		// 60 sec
		while(j >= 0 && sig.sI[j].msso > msso - 60*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm60s = sig.sI[j].predOm;
			sI.predTm60s = sig.sI[j].predTm;
			sI.mret60predOm = sI.mret60 - sig.sI[j].predOm;
			sI.mret120predTm1 = sI.mret120 - sig.sI[j].predTm; // 60 sec offset to match the beginning of prediction window
		}
		
		// 90 sec
		while(j >= 0 && sig.sI[j].msso > msso - 90*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm90s = sig.sI[j].predOm;
			sI.predTm90s = sig.sI[j].predTm;
		}
		
		// 120 sec
		while(j >= 0 && sig.sI[j].msso > msso - 120*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm120s = sig.sI[j].predOm;
			sI.predTm120s = sig.sI[j].predTm;
			sI.mret120predOm = sI.mret120 - sig.sI[j].predOm;
			sI.mret120predTm = sI.mret120 - sig.sI[j].predTm;
		}
		
		// 150 sec
		while(j >= 0 && sig.sI[j].msso > msso - 150*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm150s = sig.sI[j].predOm;
			sI.predTm150s = sig.sI[j].predTm;
		}
		
		// 180 sec
		while(j >= 0 && sig.sI[j].msso > msso - 180*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm180s = sig.sI[j].predOm;
			sI.predTm180s = sig.sI[j].predTm;
		}
		
		// 210 sec
		while(j >= 0 && sig.sI[j].msso > msso - 210*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm210s = sig.sI[j].predOm;
			sI.predTm210s = sig.sI[j].predTm;
		}
		
		// 240 sec
		while(j >= 0 && sig.sI[j].msso > msso - 240*1000)
			--j;
		if(j >= 0)
		{
			sI.mret300predTm1 = sI.mret300 - sig.sI[j].predTm; // 60 sec offset to match the beginning of prediction window
		}
		
		// 300 sec
		while(j >= 0 && sig.sI[j].msso > msso - 300*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm300s = sig.sI[j].predOm;
			sI.predTm300s = sig.sI[j].predTm;
			sI.mret300predOm = sI.mret300 - sig.sI[j].predOm;
			sI.mret300predTm = sI.mret300 - sig.sI[j].predTm;
		}
		
		// 540 sec
		while(j >= 0 && sig.sI[j].msso > msso - 540*1000)
			--j;
		if(j >= 0)
		{
			sI.mret600predTm1 = sI.mret600 - sig.sI[j].predTm; // 60 sec offset to match the beginning of prediction window
		}
		
		// 600 sec
		while(j >= 0 && sig.sI[j].msso > msso - 600*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm600s = sig.sI[j].predOm;
			sI.predTm600s = sig.sI[j].predTm;
			sI.mret600predOm = sI.mret600 - sig.sI[j].predOm;
			sI.mret600predTm = sI.mret600 - sig.sI[j].predTm;
		}
		
		// 1140 sec
		while(j >= 0 && sig.sI[j].msso > msso - 1140*1000)
			--j;
		if(j >= 0)
		{
			sI.mret1200predTm1 = sI.mret1200 - sig.sI[j].predTm; // 60 sec offset to match the beginning of prediction window
		}
		
		// 1200 sec
		while(j >= 0 && sig.sI[j].msso > msso - 1200*1000)
			--j;
		if(j >= 0)
		{
			sI.predTm1200s = sig.sI[j].predTm;
			sI.mret1200predTm = sI.mret1200 - sig.sI[j].predTm;
		}
		
		// 2340 sec
		while(j >= 0 && sig.sI[j].msso > msso - 2340*1000)
			--j;
		if(j >= 0)
		{
			sI.mret2400predTm1 = sI.mret2400 - sig.sI[j].predTm; // 60 sec offset to match the beginning of prediction window
		}
		
		// 2400 sec
		while(j >= 0 && sig.sI[j].msso > msso - 2400*1000)
			--j;
		if(j >= 0)
		{
			sI.predTm2400s = sig.sI[j].predTm;
			sI.mret2400predTm = sI.mret2400 - sig.sI[j].predTm;
		}
		
		// 4740 sec
		while(j >= 0 && sig.sI[j].msso > msso - 4740*1000)
			--j;
		if(j >= 0)
		{
			sI.mret4800predTm1 = sI.mret4800 - sig.sI[j].predTm; // 60 sec offset to match the beginning of prediction window
		}
		
		// 4800 sec
		while(j >= 0 && sig.sI[j].msso > msso - 4800*1000)
			--j;
		if(j >= 0)
		{
			sI.predTm4800s = sig.sI[j].predTm;
			sI.mret4800predTm = sI.mret4800 - sig.sI[j].predTm;
		}
	}
}

void MMakeSampleTP::calculate_pred_signals_it2(SigC& sig)
{
	vector<vector<float>> vvInputOm = getOmInputsIt1(sig);
	vector<float> vPredOm = fitterOmIt1_->getPred(vvInputOm);

	int n = sig.sI.size();
	for(int i = 0; i < n; ++i)
	{
		auto& sI = sig.sI[i];
		int msso = sI.msso;
		sI.predOmIt1 = vPredOm[i];
		int j = i;
		
		// 30 sec
		while(j >= 0 && sig.sI[j].msso > msso - 30*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm30sIt1 = sig.sI[j].predOmIt1;
			sI.mret30predOmIt1 = sI.mret30 - sig.sI[j].predOmIt1;
		}
		
		// 60 sec
		while(j >= 0 && sig.sI[j].msso > msso - 60*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm60sIt1 = sig.sI[j].predOmIt1;
			sI.mret60predOmIt1 = sI.mret60 - sig.sI[j].predOmIt1;
		}
		
		// 90 sec
		while(j >= 0 && sig.sI[j].msso > msso - 90*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm90sIt1 = sig.sI[j].predOmIt1;
		}
		
		// 120 sec
		while(j >= 0 && sig.sI[j].msso > msso - 120*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm120sIt1 = sig.sI[j].predOmIt1;
			sI.mret120predOmIt1 = sI.mret120 - sig.sI[j].predOmIt1;
		}
		
		// 150 sec
		while(j >= 0 && sig.sI[j].msso > msso - 150*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm150sIt1 = sig.sI[j].predOmIt1;
		}
		
		// 180 sec
		while(j >= 0 && sig.sI[j].msso > msso - 180*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm180sIt1 = sig.sI[j].predOmIt1;
		}
		
		// 210 sec
		while(j >= 0 && sig.sI[j].msso > msso - 210*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm210sIt1 = sig.sI[j].predOmIt1;
		}
		
		// 300 sec
		while(j >= 0 && sig.sI[j].msso > msso - 300*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm300sIt1 = sig.sI[j].predOmIt1;
			sI.mret300predOmIt1 = sI.mret300 - sig.sI[j].predOmIt1;
		}
		
		// 600 sec
		while(j >= 0 && sig.sI[j].msso > msso - 600*1000)
			--j;
		if(j >= 0)
		{
			sI.predOm600sIt1 = sig.sI[j].predOmIt1;
			sI.mret600predOmIt1 = sI.mret600 - sig.sI[j].predOmIt1;
		}
		
	}
}

vector<vector<float>> MMakeSampleTP::getOmInputsIt0(SigC& sig)
{
	vector<vector<float>> vvInput;
	for(auto sI : sig.sI)
	{
		vector<float> input;
		input.reserve(60);
		float time = sI.msso / 1000.;
		input.push_back(time);
		input.push_back(sI.sprd);
		input.push_back(sig.logVolu);
		input.push_back(sig.logPrice);
		input.push_back(sI.sig1[0]);
		input.push_back(sI.sig1[2]);
		input.push_back(sI.sig1[10]);
		input.push_back(sI.sig1[14]);
		input.push_back(sI.mret5);
		input.push_back(sI.relVolat);
		input.push_back(sig.avgDlyVolat);
		input.push_back(sI.relativeHiLo);
		input.push_back(sI.mret30);
		input.push_back(sI.sigBook[2]);
		input.push_back(sI.sigBook[3]);
		input.push_back(sI.sigBook[4]);
		input.push_back(sI.sigBook[6]);
		input.push_back(sI.sigBook[7]);
		input.push_back(sI.sigBook[8]);
		input.push_back(sI.mret15);
		input.push_back(sI.sigBook[21]);
		input.push_back(sI.sigBook[22]);
		input.push_back(sI.sigBook[23]);
		input.push_back(sI.mret120);
		input.push_back(sI.mret600);
		input.push_back(sig.hiloLag1Rat);
		input.push_back(sI.hiloLag1);
		input.push_back(sI.bollinger300);
		input.push_back(sI.relSprd);
		input.push_back(sI.sig1[1]);
		input.push_back(sI.voluMom15);
		input.push_back(sI.voluMom30);
		input.push_back(sI.voluMom60);
		input.push_back(sI.voluMom120);
		input.push_back(sI.intraVoluMom);
		input.push_back(sI.volSclBsz);
		input.push_back(sI.volSclAsz);
		input.push_back(sI.cwret30);
		input.push_back(sI.cwret60);
		input.push_back(sI.cwret120);
		input.push_back(sI.rtrd);
		input.push_back(sI.mrtrd);
		input.push_back(sI.dnmk);
		input.push_back(sI.snmk);

		vvInput.push_back(input);
	}
	return vvInput;
}

vector<vector<float>> MMakeSampleTP::getTmInputsIt0(SigC& sig)
{
	vector<vector<float>> vvInput;
	for(auto sI : sig.sI)
	{
		vector<float> input;
		input.reserve(60);
		float time = sI.msso / 1000.;
		input.push_back(time);
		input.push_back(sig.logVolu);
		input.push_back(sig.logPrice);
		input.push_back(sig.logMedSprd);
		input.push_back(sI.mret300);
		input.push_back(sI.mret300L);
		input.push_back(sI.mret600L);
		input.push_back(sI.mret1200L);
		input.push_back(sI.mret2400L);
		input.push_back(sI.mret4800L);
		input.push_back(sI.sig10[7]);
		input.push_back(sig.mretIntraLag1);
		input.push_back(sI.voluMom600);
		input.push_back(sI.voluMom3600);
		input.push_back(sI.sig10[0]);
		input.push_back(sI.sig10[1]);
		input.push_back(sI.sig10[4]);
		input.push_back(sI.sig10[8]);
		input.push_back(sI.sig10[9]);
		input.push_back(sI.sig10[10]);
		input.push_back(sI.sig10[11]);
		input.push_back(sI.sprd);
		input.push_back(sig.exchange);
		input.push_back(sig.northRST);
		input.push_back(sig.northTRD);
		input.push_back(sig.northBP);
		input.push_back(sig.hiloQAI);
		input.push_back(sig.avgDlyVolat);
		input.push_back(sig.prevDayVolume);
		input.push_back(sI.mretOpen);
		input.push_back(sI.quimMax2);
		input.push_back(sig.mretONLag1);
		input.push_back(sig.mretIntraLag2);
		input.push_back(sI.intraVoluMom);
		input.push_back(sI.voluMom300);
		input.push_back(sig.sprdOpen);
		input.push_back(sI.sigBook[8]);
		input.push_back(sI.sigBook[14]);
		input.push_back(sI.sigBook[15]);
		input.push_back(sI.sigBook[16]);
		input.push_back(sI.sigBook[17]);
		input.push_back(sI.sigBook[18]);
		input.push_back(sI.sigBook[0]);
		input.push_back(sI.sigBook[1]);
		input.push_back(sI.sigBook[2]);
		input.push_back(sI.sigBook[3]);
		input.push_back(sI.sigBook[4]);
		input.push_back(sI.relVolat);
		input.push_back(sI.sigBook[6]);
		input.push_back(sI.sigBook[7]);
		input.push_back(sI.relSprd);
		input.push_back(sI.sigBook[20]);
		input.push_back(sI.sigBook[21]);
		input.push_back(sI.sigBook[22]);
		input.push_back(sI.sigBook[23]);
		input.push_back(sI.sigBook[24]);
		input.push_back(sI.sigBook[25]);
		input.push_back(sI.mI600);
		input.push_back(sI.mI3600);
		input.push_back(sI.mIIntra);
		input.push_back(sig.hiloLag1Open);
		input.push_back(sig.hiloLag2Open);
		input.push_back(sig.hiloLag1Rat);
		input.push_back(sig.hiloLag2Rat);
		input.push_back(sig.hiloQAI2);
		input.push_back(sI.hiloLag1);
		input.push_back(sI.hiloLag2);
		input.push_back(sI.hilo900);
		input.push_back(sI.bollinger300);
		input.push_back(sI.bollinger900);
		input.push_back(sI.volSclBsz);
		input.push_back(sI.volSclAsz);
		input.push_back(sI.cwret300);
		input.push_back(sI.cwret300L);
		input.push_back(sI.rtrd);
		input.push_back(sI.mrtrd);
		input.push_back(sI.dnmk);
		input.push_back(sI.snmk);


		vvInput.push_back(input);
	}
	return vvInput;
}

vector<vector<float>> MMakeSampleTP::getOmInputsIt1(SigC& sig)
{
	vector<vector<float>> vvInput;
	for(auto sI : sig.sI)
	{
		vector<float> input;
		input.reserve(60);
		float time = sI.msso / 1000.;
		if(descIt1_ == "")
		{
			input.push_back(time);
			input.push_back(sI.sprd);
			input.push_back(sig.logVolu);
			input.push_back(sig.logPrice);
			input.push_back(sI.sig1[0]);
			input.push_back(sI.sig1[2]);
			input.push_back(sI.sig1[10]);
			input.push_back(sI.sig1[14]);
			input.push_back(sI.mret5);
			input.push_back(sI.relVolat);
			input.push_back(sig.avgDlyVolat);
			input.push_back(sI.relativeHiLo);
			input.push_back(sI.mret30);
			input.push_back(sI.sigBook[2]);
			input.push_back(sI.sigBook[3]);
			input.push_back(sI.sigBook[4]);
			input.push_back(sI.sigBook[6]);
			input.push_back(sI.sigBook[7]);
			input.push_back(sI.sigBook[8]);
			input.push_back(sI.mret15);
			input.push_back(sI.sigBook[21]);
			input.push_back(sI.sigBook[22]);
			input.push_back(sI.sigBook[23]);
			input.push_back(sI.mret120);
			input.push_back(sI.mret600);
			input.push_back(sig.hiloLag1Rat);
			input.push_back(sI.hiloLag1);
			input.push_back(sI.bollinger300);
			input.push_back(sI.relSprd);
			input.push_back(sI.sig1[1]);
			input.push_back(sI.voluMom15);
			input.push_back(sI.voluMom30);
			input.push_back(sI.voluMom60);
			input.push_back(sI.voluMom120);
			input.push_back(sI.intraVoluMom);
			input.push_back(sI.volSclBsz);
			input.push_back(sI.volSclAsz);
			input.push_back(sI.cwret30);
			input.push_back(sI.cwret60);
			input.push_back(sI.cwret120);
			input.push_back(sI.rtrd);
			input.push_back(sI.mrtrd);
			input.push_back(sI.dnmk);
			input.push_back(sI.snmk);

			input.push_back(sI.predOm);
			input.push_back(sI.predOm30s);
			input.push_back(sI.predOm60s);
			input.push_back(sI.predOm90s);
			input.push_back(sI.predOm120s);
			input.push_back(sI.predOm300s);
			input.push_back(sI.predOm600s);
			input.push_back(sI.mret30predOm);
			input.push_back(sI.mret60predOm);
			input.push_back(sI.mret120predOm);
			input.push_back(sI.mret300predOm);
			input.push_back(sI.mret600predOm);
			input.push_back(sI.predTm);
			input.push_back(sI.predTm30s);
			input.push_back(sI.predTm60s);
			input.push_back(sI.predTm90s);
			input.push_back(sI.predTm120s);
		}
		else if(descIt1_ == "onlyPastPred")
		{
			input.push_back(time);
			input.push_back(sI.sprd);
			input.push_back(sig.logVolu);
			input.push_back(sig.logPrice);
			input.push_back(sI.sig1[0]);
			input.push_back(sI.sig1[2]);
			input.push_back(sI.sig1[10]);
			input.push_back(sI.sig1[14]);
			input.push_back(sI.mret5);
			input.push_back(sI.relVolat);
			input.push_back(sig.avgDlyVolat);
			input.push_back(sI.relativeHiLo);
			input.push_back(sI.mret30);
			input.push_back(sI.sigBook[2]);
			input.push_back(sI.sigBook[3]);
			input.push_back(sI.sigBook[4]);
			input.push_back(sI.sigBook[6]);
			input.push_back(sI.sigBook[7]);
			input.push_back(sI.sigBook[8]);
			input.push_back(sI.mret15);
			input.push_back(sI.sigBook[21]);
			input.push_back(sI.sigBook[22]);
			input.push_back(sI.sigBook[23]);
			input.push_back(sI.mret120);
			input.push_back(sI.mret600);
			input.push_back(sig.hiloLag1Rat);
			input.push_back(sI.hiloLag1);
			input.push_back(sI.bollinger300);
			input.push_back(sI.relSprd);
			input.push_back(sI.sig1[1]);
			input.push_back(sI.voluMom15);
			input.push_back(sI.voluMom30);
			input.push_back(sI.voluMom60);
			input.push_back(sI.voluMom120);
			input.push_back(sI.intraVoluMom);
			input.push_back(sI.volSclBsz);
			input.push_back(sI.volSclAsz);
			input.push_back(sI.cwret30);
			input.push_back(sI.cwret60);
			input.push_back(sI.cwret120);
			input.push_back(sI.rtrd);
			input.push_back(sI.mrtrd);
			input.push_back(sI.dnmk);
			input.push_back(sI.snmk);

			input.push_back(sI.predOm30s);
			input.push_back(sI.predOm60s);
			input.push_back(sI.predOm90s);
			input.push_back(sI.predOm120s);
			input.push_back(sI.predOm300s);
			input.push_back(sI.predOm600s);
			input.push_back(sI.mret30predOm);
			input.push_back(sI.mret60predOm);
			input.push_back(sI.mret120predOm);
			input.push_back(sI.mret300predOm);
			input.push_back(sI.mret600predOm);
			input.push_back(sI.predTm30s);
			input.push_back(sI.predTm60s);
			input.push_back(sI.predTm90s);
			input.push_back(sI.predTm120s);
		}

		vvInput.push_back(input);
	}
	return vvInput;
}

