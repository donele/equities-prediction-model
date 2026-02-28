#include <MSignal/MMakeSample_at.h>
#include <MSignal/SigSet.h>
#include <MSignal.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <jl_lib/jlutil_tickdata.h>
#include <map>
#include <string>
#include <boost/thread.hpp>
using namespace std;
using namespace hff;
using namespace sig;
using namespace gtlib;

MMakeSample_at::MMakeSample_at(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
dayOK_(false),
verbose_(0),
nErrDay_(0),
calculate_booksig_(true),
do_universal_linear_model_(true),
do_error_correction_model_(true),
use_db_weights_(false),
write_ombin_(false),
write_tmbin_(false),
write_weights_(false),
txt_corr_(true),
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
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("calculateBooksig") )
		calculate_booksig_ = conf.find("calculateBooksig")->second == "true";
	if( conf.count("doUniversalLinearModel") )
		do_universal_linear_model_ = conf.find("doUniversalLinearModel")->second == "true";
	if( conf.count("doErrorCorrectionModel") )
		do_error_correction_model_ = conf.find("doErrorCorrectionModel")->second == "true";
	if( conf.count("useDBWeights") )
		use_db_weights_ = conf.find("useDBWeights")->second == "true";
	if( conf.count("writeOmBin") )
		write_ombin_ = conf.find("writeOmBin")->second == "true";
	if( conf.count("writeTmBin") )
		write_tmbin_ = conf.find("writeTmBin")->second == "true";
	if( conf.count("writeWeights") )
		write_weights_ = conf.find("writeWeights")->second == "true";
	if( conf.count("txtCorr") )
		txt_corr_ = conf.find("txtCorr")->second == "true";
	if( conf.count("nThreads") )
		nThreads_ = atoi( conf.find("nThreads")->second.c_str() );
	if( conf.count("minMsecs") )
		minMsecs_ = atoi(conf.find("minMsecs")->second.c_str());
	if( conf.count("maxMsecs") )
		maxMsecs_ = atoi(conf.find("maxMsecs")->second.c_str());
	if( conf.count("filterHorizon") )
		filterHorizon_ = atoi(conf.find("filterHorizon")->second.c_str());
	if( conf.count("filterLag") )
		filterLag_ = atoi(conf.find("filterLag")->second.c_str());
}

MMakeSample_at::~MMakeSample_at()
{}

void MMakeSample_at::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	pid_ = get_pid();

	string model = MEnv::Instance()->model;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	uids_ = MEvent::Instance()->get<set<string> >("", "allUids");
	cout << uids_.size() << " instruments." << endl;
	tSources_.read(mto::region(MEnv::Instance()->markets[0]), MEnv::Instance()->sourceFlag);

	double ridgeReg = 100 * 20. / linMod.gridInterval;
	if( write_ombin_ || write_weights_ )
	{
		if( write_weights_ )
		{
			biLinMod_ = BiLinearModel(linMod.nHorizon, linMod.num_time_slices, om_num_sig_, om_num_err_sig_, ridgeReg, ridgeReg, uids_, pid_);
			biLinMod_.weight_dir(get_weight_dir(model));
			biLinMod_.minPts(linMod.minPts);

			vCorr_.resize(linMod.nHorizon);
			vCorrTot_.resize(linMod.nHorizon);
		}
		else
			biLinModW_ = BiLinearModelWeights(linMod.nHorizon, linMod.num_time_slices, om_num_sig_, om_num_err_sig_);
	}

	if( MEnv::Instance()->multiThreadTicker )
		nThreads_ = MEnv::Instance()->nMaxThreadTicker;
	SigSet::Instance()->resize(nThreads_);
}

void MMakeSample_at::beginDay()
{
	int idate = MEnv::Instance()->idate;
	vector<string> markets = MEnv::Instance()->markets;
	const hff::LinearModel linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel nonLinMod = MEnv::Instance()->nonLinearModel;
	string model = MEnv::Instance()->model;
	mTickerUid_ = get_uid_map(markets, idate, uids_);

	// okECNs.
	okECNs_.clear();
	auto ecns = linMod.get_ecns();
	for( vector<string>::const_iterator it = ecns.begin(); it != ecns.end(); ++it )
	{
		string market = *it;
		if( "US" == market || mto::dataOK(market, idate) )
			okECNs_.push_back(market);
	}

	bool doAfterHours = false;
	string market = markets[0];
	longTicker_ = mto::longTicker(market);

	if( write_ombin_ )
	{
		omBin_.open( get_sig_path(model, idate,"om", "reg").c_str(), ios::out | ios::binary );
		omBinTxt_.open( get_sigTxt_path(model, idate, "om", "reg").c_str(), ios::out );
		writeSig::write_omBin_header(omBin_, omBinTxt_);
	}
	if( write_tmbin_ )
	{
		tmBin_.open( get_sig_path(model, idate, "tm", "reg").c_str(), ios::out | ios::binary );
		tmBinTxt_.open( get_sigTxt_path(model, idate, "tm", "reg").c_str(), ios::out );
		writeSig::write_tmBin_header(tmBin_, tmBinTxt_);
	}

	// Read the linear model.
	if( write_ombin_ || write_weights_ )
	{
		// This part should be rewritten following MMakeSample.cpp

		//if( write_weights_ )
		//{
		//	vector<int> idates = MEnv::Instance()->idates;

		//	int idateFirstUniv = 99999999;
		//	if( do_universal_linear_model_ && linMod.om_univ_fit_days() > 0 )
		//	{
		//		if( cntDay_ >= linMod.om_univ_fit_days() )
		//			idateFirstUniv = idates[cntDay_ - linMod.om_univ_fit_days()];
		//		else if( cntDay_ > 0 )
		//			idateFirstUniv = idates[0];
		//	}

		//	int idateFirstErr = 99999999;
		//	if( do_error_correction_model_ && linMod.om_err_fit_days() > 0 )
		//	{
		//		if( cntDay_ >= linMod.om_err_fit_days() )
		//			idateFirstErr = idates[cntDay_ - linMod.om_err_fit_days()];
		//		else if( cntDay_ > 0 )
		//			idateFirstErr = idates[0];
		//	}

		//	biLinMod_.beginDay(idate, idateFirstUniv, idateFirstErr);
		//	if( write_weights_ )
		//	{
		//		if( /*debugErrCorr_ ||*/ cntDay_ >= linMod.om_univ_fit_days() + linMod.om_err_fit_days() )
		//			biLinMod_.write_weights(nextClose(market, idate));
		//	}

		//	for( int iT = 0; iT < linMod.nHorizon; ++iT )
		//	{
		//		vCorr_[iT].clear();
		//		vCorrTot_[iT].clear();
		//	}
		//}
		//else
		{
			if( use_db_weights_ )
			{
				string dbname = mto::hfpar(market, idate);
				biLinModW_.read_db_weights(model, market, idate, linMod.mCode[0], mTickerUid_, linMod.gpts, dbname);
			}
			else
				biLinModW_.read_weights(idate, get_weight_dir(model));
		}
	}
}

void MMakeSample_at::beginMarket()
{
	string market = MEnv::Instance()->market;
	const vector<string>& tickers = MEnv::Instance()->tickers;
	idate_ = MEnv::Instance()->idate;
	idate_p_ = prevClose(market, idate_);
	idate_pp_ = prevClose(market, idate_p_);
	idate_n_ = nextClose(market, idate_);
	longTicker_ = mto::longTicker(market);
	dayOK_ = mto::dataOK(market, idate_);

	//if( dayOK_ )
	//	get_uid_map(market, idate_p_);

	for( map<boost::thread::id, TickTobAcc*>::iterator it = mIdTta_.begin(); it != mIdTta_.end(); ++it )
		delete it->second;
	mIdTta_.clear();
	mIdObaMap_.clear();
	sessions_ = MSessions(market, idate_);
}

void MMakeSample_at::beginMarketDay()
{
	string market = MEnv::Instance()->market;
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

void MMakeSample_at::beginTicker(const string& ticker)
{
	if( !dayOK_ )
		return;

	string market = MEnv::Instance()->market;
	string model = MEnv::Instance()->model;
	int idate = MEnv::Instance()->idate;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;

	vector<int> vMsso;
	const vector<int>* pvMsso = static_cast<const vector<int>*>(MEvent::Instance()->get(ticker, "vmsso"));
	if( pvMsso != 0 )
		vMsso = *pvMsso;
	auto itT = mTickerUid_.find(ticker);
	if( itT != mTickerUid_.end() && !vMsso.empty() )
	{
		string uid = itT->second;
		int iSig = -1;
		SigC& sig = SigSet::Instance()->get_sig_object(iSig, vMsso);

		// Read from stockcharacteristics table.
		if( read_chara_data(sig, model, market, uid, idate_, idate_p_, idate_pp_, idate_n_) )
		{
			// Read from the tick data.
			const vector<QuoteInfo>* quotes = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "nbbo"));
			const vector<TradeInfo>* trades = static_cast<const vector<TradeInfo>*>(MEvent::Instance()->get(ticker, "trades"));

			if( quotes != 0 && trades != 0 && quotes->size() >= min_quotes_ && trades->size() >= min_trades_ )
			{
				//sig.dayok = 1;

				//vector<int> vTindx1s;
				//vector<int> vQindx1s;
				//get_trade_index_1s(vTindx1s, trades);
				//get_quote_index_1s(vQindx1s, quotes);
				vector<int> vTindx1s = get_index_1s(trades, linMod.openMsecs, linMod.closeMsecs);
				vector<int> vQindx1s = get_index_1s(quotes, linMod.openMsecs, linMod.closeMsecs);

				get_trade_stateinfo_at(sig, trades, vTindx1s);
				get_quote_stateinfo_at(sig, quotes, vQindx1s);
				get_filImb_stateinfo_at(sig, trades, quotes, vTindx1s, vQindx1s);
				get_targets_stateinfo_at(sig, quotes);

				// TOB signal.
				TickTobAcc* tta = get_tta(market, idate);
				get_tob_stateinfo_at(tta, sig, idate, ticker);

				// Orderbook signal.
				if( calculate_booksig_ )
				{
					map<string, OrderBkAcc<OrderData> >& obaMap = get_obaMap(market, idate);
					get_book_stateinfo_at(obaMap, sig, market, idate, ticker, okECNs_);
				}

				// Market Impact signal.
				if( write_tmbin_ )
				{
					const vector<OrderQty>* tradeQty = static_cast<const vector<OrderQty>*>(MEvent::Instance()->get(ticker, "tradeQty"));
					get_MI_signals_at(sig, tradeQty);
				}

				get_signals_at(sig, sessions_);
				get_prediction_at(sig, uid, ticker, idate);

				//string write_ticker = ticker;
				//if( !linMod.writeCompTicker )
					//write_ticker = baseTicker(write_ticker);
				write_signal(sig, uid, ticker);
			}
		}
		SigSet::Instance()->free_sig_object(iSig);
	}
}

void MMakeSample_at::endTicker(const string& ticker)
{
}

void MMakeSample_at::endMarket()
{
}

void MMakeSample_at::endDay()
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;

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

	if( write_weights_ )
	{
		for( int iT = 0; iT < linMod.nHorizon; ++iT )
		{
			cout << itos(linMod.vHorizonLag[iT].first) + ";" + itos(linMod.vHorizonLag[iT].second) << "\t";
			cout << "corr " << vCorr_[iT].corr() << " corrTot " << vCorrTot_[iT].corr() << endl;
		}
	}

	++cntDay_;
	nErrDay_ = 0;
}

void MMakeSample_at::endJob()
{
	if( write_weights_ )
		biLinMod_.endJob();
}

void MMakeSample_at::get_prediction_at(SigC& sig, const string& uid, const string& ticker, int idate)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	const vector<int>& useom = linMod.useom;
	const vector<int>& useomErr = linMod.useomErr;
	vector<hff::StateInfo>& sI = sig.sI;
	int Npts = sI.size();

	vector<int> vIndx;
	vIndx.push_back(4);
	vIndx.push_back(5);
	vIndx.push_back(6);
	vIndx.push_back(7);
	vIndx.push_back(12);
	vIndx.push_back(13);

	// Hedge object.
	const hff::SigH* psigh = 0;
	const vector<hff::SigH>* pvSigH = static_cast<const vector<hff::SigH>*>(MEvent::Instance()->get("", "vSigH"));
	if( pvSigH == 0 )
	{
		if( nErrDay_++ < 1 )
			cout << "WARNING MMakeSample_at::get_prediction_at() Hedge info not available.\n";
	}
	else
	{
		for( vector<hff::SigH>::const_iterator it = pvSigH->begin(); it != pvSigH->end(); ++it )
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

	int n1secSlice = ceil( float(linMod.n1sec - 1) / linMod.num_time_slices );
	for( int k = 0; k < Npts; ++k )
	{
		int sec = sI[k].msso / 1000;
		int timeIdx = sec / n1secSlice;

		if( psigh != 0 )
		{
			for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
				sI[k].target[iT] = psigh->sIH[k].target[iT];
			sI[k].target60Intra = psigh->sIH[k].target60Intra;
			sI[k].targetClose = psigh->sIH[k].targetClose;
			sI[k].targetNextClose = psigh->sIH[k].targetNextClose;
		}

		if( sI[k].gptOK == 1 && write_ombin_ )
		{
			for( int iT = 0; iT < linMod.nHorizon; ++iT )
			{
				// Set gSigs.
				for( vector<int>::iterator it = vIndx.begin(); it != vIndx.end(); ++it )
					sI[k].sig1[*it] = 0.;

				// Contruct predName from the filter with the matching horizon.
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
				int indxIndx = 0;
				const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
				for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end() && indxIndx < 6; ++itf )
				{
					const hff::IndexFilter& filter = *itf;
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

				// Get predictors.
				for( int i = 0; i < hff::om_num_basic_sig_; ++i )
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

				for( int i = 0; i < om_num_err_sig_; ++i )
					sI[k].omErr[i] = useomErr[i] * sI[k].sig1[i];

				// Calculate predictions.
				double prIndex = 0.;
				double pr = 0.;
				double prErr = 0.;
				if( write_weights_ )
				{
					prIndex = biLinMod_.predIndex(iT, timeIdx, sI[k].om) + biLinMod_.predErrIndex(iT, uid, sI[k].omErr);
					pr = biLinMod_.pred(iT, timeIdx, sI[k].om);
					prErr = biLinMod_.predErr(iT, uid, sI[k].omErr);
				}
				else
				{
					prIndex = biLinModW_.predIndex(iT, timeIdx, sI[k].om) + biLinModW_.predErrIndex(iT, uid, sI[k].omErr);
					pr = biLinModW_.pred(iT, timeIdx, sI[k].om);
					prErr = biLinModW_.predErr(iT, uid, sI[k].omErr);
				}

				sI[k].predIndex[iT] = prIndex;
				sI[k].sigIndex1[iT] = sI[k].om[4];
				sI[k].sigIndex2[iT] = sI[k].om[5];
				sI[k].sigIndex3[iT] = sI[k].om[6];
				sI[k].sigIndex4[iT] = sI[k].om[7];
				sI[k].sigIndex5[iT] = sI[k].om[12];
				sI[k].sigIndex6[iT] = sI[k].om[13];

				sI[k].pred[iT] = pr;

				sI[k].predErr[iT] = prErr;

				// Get error targets.
				if( iT < linMod.nHorizon )
					sI[k].targetErr[iT] = sI[k].target[iT] - sI[k].pred[iT];
				else
					sI[k].targetErr[iT] = sI[k].target[iT];

				if( write_weights_ )
					update_biLin(k, iT, timeIdx, uid, sig, sI[k]);
			}
		}
	}
}

void MMakeSample_at::update_biLin(int k, int iT, int timeIdx, const string& uid, const SigC& sig, const StateInfo& sI)
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	if( k >= skip_first_secs_ / linMod.gridInterval /*&& sI.ask >= sI.bid */)
	{
		vCorr_[iT].add(sI.pred[iT], sI.target[iT]);
		vCorrTot_[iT].add(sI.pred[iT] + sI.predErr[iT], sI.target[iT]);

		// Update univ model.
		if( sig.inUnivFit == 1 && do_universal_linear_model_)
		{
			boost::mutex::scoped_lock lock(biLinMod_mutex_);
			biLinMod_.update(iT, timeIdx, sI.om, sI.target[iT]);
		}

		// Update err corr model.
		if( do_error_correction_model_ )
		{
			if( /*debugErrCorr_ ||*/ cntDay_ >= linMod.om_univ_fit_days() )
			{
				biLinMod_.updateErr(iT, uid, sI.omErr, sI.targetErr[iT]);
			}
		}
	}
}

void MMakeSample_at::write_signal(const SigC& sig, const string& uid, const string& ticker)
{
}

TickTobAcc* MMakeSample_at::get_tta(const string& market, int idate)
{
	boost::thread::id id = boost::this_thread::get_id();
	map<boost::thread::id, TickTobAcc*>::iterator it = mIdTta_.find(id);
	if( it == mIdTta_.end() )
	{
		TickTobAcc* tta = 0;

		if( "US" == market )
		{
			//tta = new TickTobAcc("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob");
			tta = new TickTobAcc("", longTicker_, false);
			tta->AddSpecificRoot("C", xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\C"));
			tta->AddSpecificRoot("D", xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\D"));
			tta->AddSpecificRoot("J", xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\J"));
			tta->AddSpecificRoot("N", xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\N"));
			tta->AddSpecificRoot("P", xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\P"));
			tta->AddSpecificRoot("Q", xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\Q"));
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

map<string, OrderBkAcc<OrderData> >& MMakeSample_at::get_obaMap(const string& market, int idate)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
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
