#include <MSignal/MMakeSample.h>
#include <MSignal/SigSet.h>
#include <MSignal.h>
#include <gtlib_model/pathFtns.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <boost/thread.hpp>
using namespace std;
using namespace hff;
using namespace sig;
using namespace gtlib;
using namespace writeSig;

MMakeSample::MMakeSample(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
	debug_(false),
	debugErrCorr_(false),
	debug_wgt_(false),
	debug_sprd_(false),
	debug_quote_index_(false),
	batchUpdateBiLin_(true),
	dayOK_(false),
	verbose_(0),
	verbose_ols_(false),
	calculate_booksig_(true),
	do_universal_linear_model_(true),
	do_error_correction_model_(true),
	use_calculated_weights_(false),
	use_db_weights_(false),
	use_debug_db_weights_(false),
	do_linear_long_horizon_(false),
	debug_quote_trade_order_(false),
	hedge_at_(false),
	write_ombin_(false),
	write_tmbin_(false),
	write_weights_(false),
	debug_ombin_(false),
	hedge_warning_issued_(false),
	normalized_linear_model_(false),
	ridgeUniv_(0.),
	ridgeSS_(0.),
	cntDay_(0),
	nThreads_(1),
	minMsecs_(0),
	maxMsecs_(86400000),
	filterHorizon_(0),
	filterLag_(0),
	cntOmBin_(0),
	cntTmBin_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("wmodel") )
		wmodel_ = conf.find("wmodel")->second;
	if( conf.count("debugErrCorr") )
		debugErrCorr_ = conf.find("debugErrCorr")->second == "true";
	if( conf.count("debugWgt") )
		debug_wgt_ = conf.find("debugWgt")->second == "true";
	if( conf.count("debugSprd") )
		debug_sprd_ = conf.find("debugSprd")->second == "true";
	if( conf.count("debugQuoteIndex") )
		debug_quote_index_ = conf.find("debugQuoteIndex")->second == "true";
	if( conf.count("debugOmBin") )
		debug_ombin_ = conf.find("debugOmBin")->second == "true";
	if( conf.count("batchUpdateBiLin") )
		batchUpdateBiLin_ = conf.find("batchUpdateBiLin")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("verboseOLS") )
		verbose_ols_ = conf.find("verboseOLS")->second == "true";
	if( conf.count("calculateBooksig") )
		calculate_booksig_ = conf.find("calculateBooksig")->second == "true";
	if( conf.count("doUniversalLinearModel") )
		do_universal_linear_model_ = conf.find("doUniversalLinearModel")->second == "true";
	if( conf.count("doErrorCorrectionModel") )
		do_error_correction_model_ = conf.find("doErrorCorrectionModel")->second == "true";
	if( conf.count("useCalculatedWeights") )
		use_calculated_weights_ = conf.find("useCalculatedWeights")->second == "true";
	if( conf.count("useDBWeights") )
		use_db_weights_ = conf.find("useDBWeights")->second == "true";
	if( conf.count("useDebugDBWeights") )
		use_debug_db_weights_ = conf.find("useDebugDBWeights")->second == "true";
	if( conf.count("doLinearLongHorizon") )
		do_linear_long_horizon_ = conf.find("doLinearLongHorizon")->second == "true";
	if( conf.count("normalizedLinearModel") )
		normalized_linear_model_ = conf.find("normalizedLinearModel")->second == "true";
	if( conf.count("debugQuoteTradeOrder") )
		debug_quote_trade_order_ = conf.find("debugQuoteTradeOrder")->second == "true";
	if( conf.count("hedgeAt") )
		hedge_at_ = conf.find("hedgeAt")->second == "true";
	if( conf.count("writeOmBin") )
		write_ombin_ = conf.find("writeOmBin")->second == "true";
	if( conf.count("writeTmBin") )
		write_tmbin_ = conf.find("writeTmBin")->second == "true";
	if( conf.count("writeWeights") )
		write_weights_ = conf.find("writeWeights")->second == "true";
	if( conf.count("minMsecs") )
		minMsecs_ = atoi(conf.find("minMsecs")->second.c_str());
	if( conf.count("maxMsecs") )
		maxMsecs_ = atoi(conf.find("maxMsecs")->second.c_str());
	if( conf.count("filterHorizon") )
		filterHorizon_ = atoi(conf.find("filterHorizon")->second.c_str());
	if( conf.count("filterLag") )
		filterLag_ = atoi(conf.find("filterLag")->second.c_str());
	if( conf.count("ridgeUniv") )
		ridgeUniv_ = atof(conf.find("ridgeUniv")->second.c_str());
	if( conf.count("ridgeSS") )
		ridgeSS_ = atof(conf.find("ridgeSS")->second.c_str());
}

MMakeSample::~MMakeSample()
{}

void MMakeSample::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	string model = MEnv::Instance()->model;
	if( wmodel_.empty() )
		wmodel_ = model;
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	const NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	const string& baseDir = MEnv::Instance()->baseDir;
	const vector<int>& allIdates = MEnv::Instance()->allIdates;
	uids_ = MEvent::Instance()->get<set<string> >("", "allUids");
	cout << uids_.size() << " instruments." << endl;
	tSources_.read(mto::region(MEnv::Instance()->markets[0]), MEnv::Instance()->sourceFlag);

	vCorr_.resize(linMod.nHorizon);
	vCorrTot_.resize(nonLinMod.nHorizon);

	double ridgeReg = 100 * 20. / linMod.gridInterval;
	double ridgeRegTm = 100 * 20. / 300;
	if( write_ombin_ || write_weights_ )
	{
		if( ridgeUniv_ == 0. )
			ridgeUniv_ = ridgeReg;
		if( ridgeSS_ == 0. )
			ridgeSS_ = ridgeReg;

		if( use_calculated_weights_ )
			biLinModW_ = BiLinearModelWeights(linMod.nHorizon, linMod.num_time_slices, om_num_sig_, om_num_err_sig_);
		else
		{
			string covDir = get_cov_dir(baseDir, model);
			string weightDir = get_weight_dir(baseDir, model);
			int minPts = linMod.minPts(model);
			if( normalized_linear_model_ )
			{
				biLinModN_ = BiLinearModelN(covDir, weightDir,
						linMod.om_univ_fit_days, linMod.om_err_fit_days, minPts,
						linMod.nHorizon, linMod.num_time_slices, om_num_sig_,
						om_num_err_sig_, ridgeUniv_, ridgeSS_, uids_, allIdates, write_weights_, debug_wgt_);
				if( verbose_ols_ )
					biLinModN_.set_verbose();
			}
			else
			{
				biLinMod_ = BiLinearModel(covDir, weightDir,
						linMod.om_univ_fit_days, linMod.om_err_fit_days, minPts,
						linMod.nHorizon, linMod.num_time_slices, om_num_sig_,
						om_num_err_sig_, ridgeUniv_, ridgeSS_, uids_, allIdates, write_weights_, debug_wgt_);
				if( verbose_ols_ )
					biLinMod_.set_verbose();
			}
		}

	}
	else if( do_linear_long_horizon_ )
	{
		if( ridgeUniv_ == 0. )
			ridgeUniv_ = ridgeRegTm;
		if( ridgeSS_ == 0. )
			ridgeSS_ = ridgeRegTm;

		string covDir = get_tmCov_dir(baseDir, model);
		string weightDir = get_tmWeight_dir(baseDir, model);
		int minPts = linMod.minPtsTm(model);
		if( normalized_linear_model_ )
		{
			biLinModTN_ = BiLinearModelN(covDir, weightDir, minPts,
					linMod.om_univ_fit_days, linMod.om_err_fit_days,
					nonLinMod.nHorizon - linMod.nHorizon, 0, 0,
					tm_num_sig_, ridgeUniv_, ridgeSS_, uids_, allIdates, debug_wgt_);
			if( verbose_ols_ )
				biLinModTN_.set_verbose();
		}
		else
		{
			biLinModT_ = BiLinearModel(covDir, weightDir, minPts,
					linMod.om_univ_fit_days, linMod.om_err_fit_days,
					nonLinMod.nHorizon - linMod.nHorizon, 0, 0,
					tm_num_sig_, ridgeUniv_, ridgeSS_, uids_, allIdates, debug_wgt_);
			if( verbose_ols_ )
				biLinModT_.set_verbose();
		}
	}

	if( MEnv::Instance()->multiThreadTicker )
		nThreads_ = MEnv::Instance()->nMaxThreadTicker;
	SigSet::Instance()->resize(nThreads_);
}

void MMakeSample::beginDay()
{
	++cntDay_;
	hedge_warning_issued_ = false;

	const vector<string>& markets = MEnv::Instance()->markets;
	const string& market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	mTickerUid_ = get_uid_map(markets, idate, uids_);

	bool doAfterHours = false;
	longTicker_ = mto::longTicker(market);

	setOkEcns();
	linModBeginDay();
	openSigFile();
}

void MMakeSample::setOkEcns()
{
	okECNs_.clear();
	const LinearModel linMod = MEnv::Instance()->linearModel;
	const string& market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	auto ecns = linMod.get_ecns();
	for( vector<string>::const_iterator it = ecns.begin(); it != ecns.end(); ++it )
	{
		string market = *it;
		if( "US" == market || ("C" == market.substr(0, 1) && linMod.useMoreCAEcns) || mto::dataOK(market, idate) )
			okECNs_.push_back(market);
	}
}

bool MMakeSample::writeOmToday()
{
	return write_ombin_ && 
		(use_calculated_weights_ || debug_ombin_ ||
		(normalized_linear_model_ && 0 /*&& biLinModN_.goodModel()*/) ||
		(!normalized_linear_model_ && biLinMod_.goodModel())
		);
}

void MMakeSample::openSigFile()
{
	const string& baseDir = MEnv::Instance()->baseDir;
	const LinearModel linMod = MEnv::Instance()->linearModel;
	string model = MEnv::Instance()->model;
	int idate = MEnv::Instance()->idate;
	if( writeOmToday() )
	{
		omBin_.open( get_sig_path(baseDir, model, idate, "om", "reg").c_str(), ios::out | ios::binary );
		omBinTxt_.open( get_sigTxt_path(baseDir, model, idate,"om", "reg").c_str(), ios::out );
		write_omBin_header(omBin_, omBinTxt_);
	}
	bool enough_day_tmbin = !do_linear_long_horizon_ || cntDay_ > linMod.om_univ_fit_days + linMod.om_err_fit_days;
	if( write_tmbin_ && enough_day_tmbin )
	{
		tmBin_.open( get_sig_path(baseDir, model, idate, "tm", "reg").c_str(), ios::out | ios::binary );
		tmBinTxt_.open( get_sigTxt_path(baseDir, model, idate,"tm", "reg").c_str(), ios::out );
		write_tmBin_header(tmBin_, tmBinTxt_);
	}
}

void MMakeSample::linModBeginDay()
{
	const LinearModel linMod = MEnv::Instance()->linearModel;
	const NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	string model = MEnv::Instance()->model;
	const string& market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	if( write_ombin_ || write_weights_ )
	{
		if( use_calculated_weights_ )
		{
			if( use_db_weights_ )
			{
				string dbname = use_debug_db_weights_ ? mto::hfdbg(market) : mto::hfpar(market, idate);
				biLinModW_.read_db_weights(model, market, idate, linMod.mCode[0], mTickerUid_, linMod.gpts, dbname);
			}
			else
				biLinModW_.read_weights(idate, get_weight_dir(MEnv::Instance()->baseDir, wmodel_));
		}
		else
		{
			if( normalized_linear_model_ )
				biLinModN_.beginDay(idate);
			else
				biLinMod_.beginDay(idate);
		}

		for( int iT = 0; iT < linMod.nHorizon; ++iT )
		{
			vCorr_[iT].clear();
			vCorrTot_[iT].clear();
		}
	}
	else if( do_linear_long_horizon_ )
	{
		if( normalized_linear_model_ )
			biLinModTN_.beginDay(idate);
		else
			biLinModT_.beginDay(idate);

		for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
			vCorrTot_[iT].clear();
	}
}

void MMakeSample::beginMarket()
{
	string market = MEnv::Instance()->market;
	const vector<string>& tickers = MEnv::Instance()->tickers;
	idate_ = MEnv::Instance()->idate;
	idate_p_ = prevClose(market, idate_);
	idate_pp_ = prevClose(market, idate_p_);
	idate_n_ = nextClose(market, idate_);
	longTicker_ = mto::longTicker(market);

	string marketOK = market;
	if( MEnv::Instance()->sourceFlag == "CJ_CConly" )
		marketOK = "CC";
	else if( MEnv::Instance()->sourceFlag == "CJ_CHonly" )
		marketOK = "CH";
	else if( MEnv::Instance()->sourceFlag == "AS_chixonly" )
		marketOK = "AX";
	dayOK_ = mto::dataOK(market, idate_);
	cntOkTicker_ = 0;

	for( map<boost::thread::id, TickTobAcc*>::iterator it = mIdTta_.begin(); it != mIdTta_.end(); ++it )
		delete it->second;
	mIdTta_.clear();
	mIdObaMap_.clear();
	sessions_ = MSessions(market, idate_);
}

void MMakeSample::beginMarketDay()
{
	string market = MEnv::Instance()->market;
	GODBC::Instance()->close_all();
}

void MMakeSample::beginTicker(const string& ticker)
{
	if( !dayOK_ )
		return;

	string market = MEnv::Instance()->market;
	string model = MEnv::Instance()->model;
	int idate = MEnv::Instance()->idate;
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	const NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;

	auto itT = mTickerUid_.find(ticker);
	if( itT != mTickerUid_.end() )
	{
		string uid = itT->second;

		int iSig = -1;
		SigC& sig = SigSet::Instance()->get_sig_object(iSig);

		// Read from stockcharacteristics table.
		if( read_chara_data(sig, model, market, uid, idate_, idate_p_, idate_pp_, idate_n_) )
		{
			if( 1 )
			{
				// Read from the tick data.
				const vector<QuoteInfo>* quotes = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "nbbo"));
				const vector<TradeInfo>* trades = static_cast<const vector<TradeInfo>*>(MEvent::Instance()->get(ticker, "trades"));

				if( quotes != 0 && trades != 0 && quotes->size() >= min_quotes_ && trades->size() >= min_trades_ )
				{
					++cntOkTicker_;
					get_quote_stateinfo(sig, quotes, debug_quote_index_);
					get_trade_stateinfo(sig, trades);
					if( write_ombin_ || write_weights_ )
						get_filImb_stateinfo(sig, trades, quotes);

					// TOB signal.
					TickTobAcc* tta = get_tta(market, idate);
					get_tob_stateinfo(tta, sig, idate, ticker);

					// Orderbook signal.
					if( calculate_booksig_ )
					{
						map<string, OrderBkAcc<OrderData> >& obaMap = get_obaMap(market, idate);
						get_book_stateinfo(obaMap, sig, market, idate, ticker, okECNs_);
					}

					// Market Impact signal.
					if( write_tmbin_ )
					{
						const vector<OrderQty>* tradeQty = static_cast<const vector<OrderQty>*>(MEvent::Instance()->get(ticker, "tradeQty"));
						get_MI_signals(sig, tradeQty);
					}

					get_targets_stateinfo(sig, quotes);
					get_signals(sig, sessions_);
					get_prediction(sig, uid, ticker, idate);
					write_signal(sig, uid, ticker);
				}
			}
		}
		SigSet::Instance()->free_sig_object(iSig);
	}
}

void MMakeSample::endTicker(const string& ticker)
{
}

void MMakeSample::endMarket()
{
	if( dayOK_ && cntOkTicker_ == 0 )
	{
		string market = MEnv::Instance()->market;
		cerr << "MMakeSample::endMarket(): Read error " << market << endl;
		exit(22);
	}
}

void MMakeSample::endDay()
{
	int idate = MEnv::Instance()->idate;
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	const NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;

	if( write_ombin_ )
	{
		writeSig::update_ncols(omBin_, cntOmBin_);

		omBin_.close();
		omBin_.clear();

		omBinTxt_.close();
		omBinTxt_.clear();

		cntOmBin_ = 0;
	}

	if( write_tmbin_ )
	{
		writeSig::update_ncols(tmBin_, cntTmBin_);

		tmBin_.close();
		tmBin_.clear();

		tmBinTxt_.close();
		tmBinTxt_.clear();

		cntTmBin_ = 0;
	}

	if( write_ombin_ || write_weights_ )
	{
		for( int iT = 0; iT < linMod.nHorizon; ++iT )
		{
			cout << itos(linMod.vHorizonLag[iT].first) + ";" + itos(linMod.vHorizonLag[iT].second) << "\t";
			cout << "corr " << vCorr_[iT].corr() << " corrTot " << vCorrTot_[iT].corr() << endl;
		}

		if( !use_calculated_weights_ )
		{
			if( normalized_linear_model_ )
				biLinModN_.endDay(idate);
			else
				biLinMod_.endDay(idate);
		}
	}

	if( do_linear_long_horizon_ && (write_tmbin_ || write_weights_) )
	{
		for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
		{
			cout << itos(nonLinMod.vHorizonLag[iT].first) + ";" + itos(nonLinMod.vHorizonLag[iT].second) << "\t";
			cout << "corrTot " << vCorrTot_[iT].corr() << endl;
		}

		if( !use_calculated_weights_ )
		{
			if( normalized_linear_model_ )
				biLinModTN_.endDay(idate);
			else
				biLinModT_.endDay(idate);
		}
	}
}

void MMakeSample::endJob()
{
	if( write_ombin_ || write_weights_ )
	{
		if( normalized_linear_model_ )
			biLinModN_.endJob();
		else
			biLinMod_.endJob();
	}
	else if( do_linear_long_horizon_ )
	{
		if( normalized_linear_model_ )
			biLinModTN_.endJob();
		else
			biLinModT_.endJob();
	}
}

void MMakeSample::get_prediction(SigC& sig, const string& uid, const string& ticker, int idate)
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	const NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	const vector<int>& useom = linMod.useom;
	const vector<int>& useomErr = linMod.useomErr;

	vector<int> vIndx;
	vIndx.push_back(4);
	vIndx.push_back(5);
	vIndx.push_back(6);
	vIndx.push_back(7);
	vIndx.push_back(12);
	vIndx.push_back(13);

	// Hedge object.
	const SigH* psigh = 0;
	const vector<SigH>* pvSigH = static_cast<const vector<SigH>*>(MEvent::Instance()->get("", "vSigH"));
	if( pvSigH == 0 )
	{
		if( (write_tmbin_ ) && !hedge_warning_issued_ )
		{
			cout << "WARNING MMakeSample::get_prediction() Hedge info not available.\n";
			hedge_warning_issued_ = true;
		}
	}
	else
	{
		for( vector<SigH>::const_iterator it = pvSigH->begin(); it != pvSigH->end(); ++it )
		{
			if( it->ticker == ticker )
			{
				psigh = &(*it);
				break;
			}
		}
	}

	// Hedge signal.
	if( psigh != 0 )
	{
		sig.northBP = psigh->northBP;
		sig.northRST = psigh->northRST;
		sig.northTRD = psigh->northTRD;
		sig.tarON = psigh->tarON;
	}

	BiLinearModel::LinRegStatTicker* linRegStat = nullptr;
	if( !use_calculated_weights_ && (write_ombin_ || write_weights_) && batchUpdateBiLin_ )
		linRegStat = biLinMod_.getNewLinRegStatTicker();

	int timeFac = ceil( float(linMod.gpts - 1) / linMod.num_time_slices );
	for( int k = 0; k < linMod.gpts; ++k )
	{
		int timeIdx = k / timeFac;
		vector<StateInfo>& sI = sig.sI;
		int sec = k * linMod.gridInterval;

		// Get hedged targets.
		if( psigh != 0 )
		{
			if( !hedge_at_ )
			{
				for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
					sI[k].target[iT] = psigh->sIH[k].target[iT];
				//sI[k].target60Intra = psigh->sIH[k].target60Intra;
				sI[k].targetClose = psigh->sIH[k].targetClose;
				sI[k].targetNextClose = psigh->sIH[k].targetNextClose;
			}
			else if( hedge_at_ )
			{
				for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
				{
					int len = nonLinMod.vHorizonLag[iT].first;
					int lag = nonLinMod.vHorizonLag[iT].second;
					sI[k].target[iT] = psigh->get_hedged_target(sec, len, lag);
					if( debug_ )
					{
						if( iT == 0 && sig.ticker == "S:CCRO3" && sec == 15300 )
						{
							cout << "S:CCRO3 15300" << endl;
							cout << "tar " << sI[k].target[iT] << endl;
							psigh->get_hedged_target(sec, len, lag, debug_);
						}
					}
				}
				//sI[k].target60Intra = 0.;
				sI[k].targetClose = 0.;
				sI[k].targetNextClose = 0.;
			}
		}

		// Bilinear models om.
		if( sI[k].gptOK == 1 && (write_ombin_ || write_weights_) )
		{
			for( int iT = 0; iT < linMod.nHorizon; ++iT )
			{
				// Set gSigs.
				for( vector<int>::iterator it = vIndx.begin(); it != vIndx.end(); ++it )
					sI[k].sig1[*it] = 0.;

				// Construct predName from the filter with the matching horizon.
				int filterHorizon = 0;
				int filterLag = 0;
				if( filterHorizon_ > 0 )
				{
					filterHorizon = filterHorizon_;
					filterLag = filterLag_;
				}
				else
				{
					filterHorizon = linMod.vHorizonLag[iT].first;
					filterLag = linMod.vHorizonLag[iT].second;
				}

				// Read index prediction.
				int indxIndx = 0;
				const vector<IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
				for( vector<IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end() && indxIndx < 6; ++itf )
				{
					const IndexFilter& filter = *itf;
					if( filterHorizon == filter.horizon && filterLag == filter.targetLag )
					{
						string predName = filter.title() + "_pred";
						const vector<float>* vPred = static_cast<const vector<float>*>(MEvent::Instance()->get("", predName));
						if( vPred != 0 )
						{
							sI[k].sig1[vIndx[indxIndx]] = (*vPred)[sec];
							++indxIndx;
						}
					}
				}

				// Get predictors for universal model.
				for( int i = 0; i < om_num_basic_sig_; ++i )
				{
					int indx1 = i;
					int indx2 = i + om_num_basic_sig_;
					int indx3 = i + 2 * om_num_basic_sig_;
					int indx4 = i + 3 * om_num_basic_sig_;

					sI[k].om[indx1] = useom[indx1] * sI[k].sig1[i];
					sI[k].om[indx2] = useom[indx2] * sig.logVolu * sI[k].sig1[i];
					sI[k].om[indx3] = useom[indx3] * sig.logPrice * sI[k].sig1[i];
					sI[k].om[indx4] = useom[indx4] * sI[k].absSprd * sI[k].sig1[i];
				}

				// Get predictors for error correction (ticker specific) model.
				for( int i = 0; i < om_num_err_sig_; ++i )
					sI[k].omErr[i] = useomErr[i] * sI[k].sig1[i];

				// Predictions.
				double prIndex = 0.;
				double pr = 0.;
				double prErr = 0.;
				if( use_calculated_weights_ )
				{
					if( use_db_weights_ )
						pr = biLinModW_.predDB(timeIdx, uid, sI[k].om);
					else
					{
						prIndex = biLinModW_.predIndex(iT, timeIdx, sI[k].om) + biLinModW_.predErrIndex(iT, uid, sI[k].omErr);
						pr = biLinModW_.pred(iT, timeIdx, sI[k].om);
						prErr = biLinModW_.predErr(iT, uid, sI[k].omErr);
					}
				}
				else
				{
					if( normalized_linear_model_ )
					{
						prIndex = biLinModN_.predIndex(iT, timeIdx, sI[k].om) + biLinModN_.predErrIndex(iT, uid, sI[k].omErr);
						pr = biLinModN_.pred(iT, timeIdx, sI[k].om);
						prErr = biLinModN_.predErr(iT, uid, sI[k].omErr);
					}
					else
					{
						prIndex = biLinMod_.predIndex(iT, timeIdx, sI[k].om) + biLinMod_.predErrIndex(iT, uid, sI[k].omErr);
						pr = biLinMod_.pred(iT, timeIdx, sI[k].om);
						prErr = biLinMod_.predErr(iT, uid, sI[k].omErr);
					}
				}

				sI[k].predIndex[iT] = prIndex;
				sI[k].sigIndex1[iT] = sI[k].sig1[4];
				sI[k].sigIndex2[iT] = sI[k].sig1[5];
				sI[k].sigIndex3[iT] = sI[k].sig1[6];
				sI[k].sigIndex4[iT] = sI[k].sig1[7];
				sI[k].sigIndex5[iT] = sI[k].sig1[12];
				sI[k].sigIndex6[iT] = sI[k].sig1[13];

				sI[k].pred[iT] = pr;
				sI[k].predErr[iT] = prErr;

				// Get error targets.
				if( iT < linMod.nHorizon )
					sI[k].targetErr[iT] = sI[k].target[iT] - sI[k].pred[iT];
				else
					sI[k].targetErr[iT] = sI[k].target[iT];

				// Update bilinear model.
				if( !use_calculated_weights_ )
				{
					if( write_ombin_ || write_weights_ )
					{
						if( batchUpdateBiLin_ )
							batch_update_biLin(k, linRegStat, timeIdx, uid, sig, sI[k]);
						else
							update_biLin(k, iT, timeIdx, uid, sig, sI[k]);
					}
				}
				if( k >= skip_first_secs_ / linMod.gridInterval )
				{
					vCorr_[iT].add(sI[k].pred[iT], sI[k].target[iT]);
					vCorrTot_[iT].add(sI[k].pred[iT] + sI[k].predErr[iT], sI[k].target[iT]);
				}
			}

			float indxPred1 = sI[k].sigIndex1[0];
			float indxPred2 = 0.;
			if(linMod.market[0] == 'U')
				indxPred2 = sI[k].sigIndex3[0];
			else
				indxPred2 = sI[k].sigIndex2[0];

			if( sI[k].sprd > 20. )
			{
				sI[k].indxPredWS1 = indxPred1;
				sI[k].indxPredWS2 = indxPred2;
			}
			// Get predictors.
			sI[k].tm[0] = sI[k].sig10[0]; // qIWt.
			sI[k].tm[1] = sI[k].sig10[2]; // mret300.
			sI[k].tm[2] = sI[k].sig10[3]; // mret300L.
			sI[k].tm[5] = sI[k].sig10[4]; // hilo.
			sI[k].tm[6] = sI[k].sig10[10]; // TOBoff1.
			sI[k].tm[7] = sI[k].sig10[8]; // TOBqI2.

			for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
			{
				// Predictions.
				double prErr = 0.;
				if( normalized_linear_model_ )
					prErr = biLinModTN_.predErr(iT - linMod.nHorizon, uid, sI[k].tm);
				else
					prErr = biLinModT_.predErr(iT - linMod.nHorizon, uid, sI[k].tm);
				sI[k].predErr[iT] = prErr;

				sI[k].targetErr[iT] = sI[k].target[iT];

				// Update bilinear model.
				if( sig.inUnivFit == 1 &&  do_linear_long_horizon_ )
				{
					if( k % (300 / linMod.gridInterval) == 0 )
					{
						boost::mutex::scoped_lock lock(biLinMod_mutex_);
						if( normalized_linear_model_ )
							biLinModTN_.updateErr(iT - linMod.nHorizon, uid, sI[k].tm, sI[k].targetErr[iT]);
						else
							biLinModT_.updateErr(iT - linMod.nHorizon, uid, sI[k].tm, sI[k].targetErr[iT]);
					}
				}
				vCorrTot_[iT].add(sI[k].pred[iT] + sI[k].predErr[iT], sI[k].target[iT]);
			}
		}
	}
	if( !use_calculated_weights_ && (write_ombin_ || write_weights_) && batchUpdateBiLin_ )
	{
		boost::mutex::scoped_lock lock(biLinMod_mutex_);
		biLinMod_.update(linRegStat, uid);
	}
	if( linRegStat != nullptr )
		delete linRegStat;
}

void MMakeSample::update_biLin(int k, int iT, int timeIdx, const string& uid, const SigC& sig, const StateInfo& sI)
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	if( k >= skip_first_secs_ / linMod.gridInterval )
	{
		// Update univ model.
		if( sig.inUnivFit == 1 && do_universal_linear_model_)
		{
			boost::mutex::scoped_lock lock(biLinMod_mutex_);

			if( normalized_linear_model_ )
				biLinModN_.update(iT, timeIdx, sI.om, sI.target[iT]);
			else
				biLinMod_.update(iT, timeIdx, sI.om, sI.target[iT]);
		}

		// Update err corr model.
		if( do_error_correction_model_ )
		{
			//if( debugErrCorr_ || cntDay_ > linMod.om_univ_fit_days )
			if( debugErrCorr_ || biLinMod_.goodUnivModel() )
			{
				if( normalized_linear_model_ )
					biLinModN_.updateErr(iT, uid, sI.omErr, sI.targetErr[iT]);
				else
					biLinMod_.updateErr(iT, uid, sI.omErr, sI.targetErr[iT]);
			}
		}
	}
}

void MMakeSample::batch_update_biLin(int k, BiLinearModel::LinRegStatTicker* linRegStat, int timeIdx, const std::string& uid, const hff::SigC& sig, const hff::StateInfo& sI)
{
	int iT = 0;
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	if( k >= skip_first_secs_ / linMod.gridInterval )
	{
		// Update univ model.
		if( sig.inUnivFit == 1 && do_universal_linear_model_)
			linRegStat->olsmov[timeIdx].add(sI.om, sI.target[iT]);

		// Update err corr model.
		if( do_error_correction_model_ )
		{
			if( debugErrCorr_ || cntDay_ > linMod.om_univ_fit_days )
				linRegStat->olsmovErr.add(sI.omErr, sI.targetErr[iT]);
		}
	}
}

void MMakeSample::write_signal(const SigC& sig, const string& uid, const string& ticker)
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;

	// Write signals.
	if( write_ombin_ )
	{
		boost::mutex::scoped_lock lock(om_mutex_);
		if( writeOmToday() )
			cntOmBin_ += writeSig::write_omBin(omBin_, omBinTxt_, sig, uid, ticker, minMsecs_, maxMsecs_, debug_sprd_);
	}

	if( write_tmbin_ )
	{
		boost::mutex::scoped_lock lock(tm_mutex_);

		bool enough_day_tmbin = !do_linear_long_horizon_ || cntDay_ > linMod.om_univ_fit_days + linMod.om_err_fit_days;
		if( write_tmbin_ && enough_day_tmbin )
			cntTmBin_ += writeSig::write_tmBin(tmBin_, tmBinTxt_, sig, uid, ticker, minMsecs_, maxMsecs_, debug_sprd_);
	}
}

TickTobAcc* MMakeSample::get_tta(const string& market, int idate)
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	boost::thread::id id = boost::this_thread::get_id();
	map<boost::thread::id, TickTobAcc*>::iterator it = mIdTta_.find(id);
	if( it == mIdTta_.end() )
	{
		TickTobAcc* tta = 0;

		if( "US" == market )
		{
			tta = new TickTobAcc("", longTicker_, false);

			if(0)
			{
				vector<string> ordered_markets = linMod.get_ecns();
				sort(begin(ordered_markets), end(ordered_markets));
				for( const string& market : ordered_markets )
				{
					vector<string> dirs = tSources_.stocksdirectory(market, idate);
					for( const string& dir : dirs )
						tta->AddSpecificRoot(market.substr(1, 1), dir);
				}
			}
			else
			{
				tta->AddSpecificRoot("C", xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\C"));
				tta->AddSpecificRoot("D", xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\D"));
				tta->AddSpecificRoot("J", xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\J"));
				tta->AddSpecificRoot("N", xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\N"));
				tta->AddSpecificRoot("P", xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\P"));
				tta->AddSpecificRoot("Q", xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\Q"));
				tta->AddSpecificRoot("B", xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\B"));
				tta->AddSpecificRoot("Y", xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\Y"));
			}
		}
		else
		{
			tta = new TickTobAcc("", longTicker_, false, MEnv::Instance()->linearModel.openMsecs, MEnv::Instance()->linearModel.closeMsecs);

			// main makret.
			vector<string> dirs = tSources_.stocksdirectory(market, idate);
			for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
				tta->AddSpecificRoot( market, *itd, longTicker_ );

			// ecns.
			for( vector<string>::const_iterator it = okECNs_.begin(); it != okECNs_.end(); ++it )
			{
				string market = *it;
				vector<string> dirs = tSources_.stocksdirectory(market, idate);
				for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
					tta->AddSpecificRoot( market, *itd, longTicker_ );
			}
		}

		// Insert tta.
		{
			boost::mutex::scoped_lock lock(tta_mutex_);
			mIdTta_[id] = tta;
		}
		it = mIdTta_.find(id);
	}
	return it->second;
}

map<string, OrderBkAcc<OrderData> >& MMakeSample::get_obaMap(const string& market, int idate)
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	boost::thread::id id = boost::this_thread::get_id();
	map<boost::thread::id, map<string, OrderBkAcc<OrderData> > >::iterator ito = mIdObaMap_.find(id);
	if( ito == mIdObaMap_.end() )
	{
		{
			boost::mutex::scoped_lock lock(oba_mutex_);
			mIdObaMap_[id] = map<string, OrderBkAcc<OrderData> >();
		}
		ito = mIdObaMap_.find(id);

		if( "US" == market )
		{
			auto ecns = linMod.get_ecns();
			for( vector<string>::const_iterator ite = ecns.begin(); ite != ecns.end(); ++ite )
			{
				string ecn = *ite;
				ito->second[ecn] = OrderBkAcc<OrderData>();

				vector<string> dirs = tSources_.bookdirectory(ecn, idate);
				for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
					ito->second[ecn].AddRoot( *itd, longTicker_ );
			}
		}
		else
		{
			// main market.
			ito->second[market] = OrderBkAcc<OrderData>();

			vector<string> dirs = tSources_.bookdirectory(market, idate);
			for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
				ito->second[market].AddRoot( *itd, longTicker_ );

			// ecns.
			for( vector<string>::const_iterator ite = okECNs_.begin(); ite != okECNs_.end(); ++ite )
			{
				string ecn = *ite;
				ito->second[ecn] = OrderBkAcc<OrderData>();

				vector<string> dirs = tSources_.bookdirectory(ecn, idate);
				for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
					ito->second[ecn].AddRoot( *itd, longTicker_ );
			}
		}
	}
	return ito->second;
}
