#include <HFitting/HMakeSample.h>
#include <HFitting.h>
#include <HLib.h>
#include <jl_lib.h>
#include <jl_lib/jlutil_tickdata.h>
#include <map>
#include <string>
#include "TFile.h"
#include <boost/thread.hpp>
using namespace std;
using namespace hff;
using namespace sig;

HMakeSample::HMakeSample(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
debug_(false),
verbose_(0),
write_bmtxt_(false),
write_omtxt_(false),
write_ombin_(false),
write_tmtxt_(false),
write_tmbin_(false),
debug_write_bin_(false),
txt_corr_(true),
cntDay_(0),
nThreads_(1),
cntOmBin_(0),
cntTmBin_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debug_write_bin") )
		debug_write_bin_ = conf.find("debug_write_bin")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("writeBmTxt") )
		write_bmtxt_ = conf.find("writeBmTxt")->second == "true";
	if( conf.count("writeOmTxt") )
		write_omtxt_ = conf.find("writeOmTxt")->second == "true";
	if( conf.count("writeOmBin") )
		write_ombin_ = conf.find("writeOmBin")->second == "true";
	if( conf.count("writeTmTxt") )
		write_tmtxt_ = conf.find("writeTmTxt")->second == "true";
	if( conf.count("writeTmBin") )
		write_tmbin_ = conf.find("writeTmBin")->second == "true";
	if( conf.count("txtCorr") )
		txt_corr_ = conf.find("txtCorr")->second == "true";
}

HMakeSample::~HMakeSample()
{}

void HMakeSample::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	pid_ = get_pid();

	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
	const hff::NonLinearModel& nonLinMod = HEnv::Instance()->nonLinearModel();
	uids_ = HEvent::Instance()->get<set<string> >("", "allUids");
	cout << uids_.size() << " instruments." << endl;
	tSources_.read(HEnv::Instance()->markets()[0]);

	vAccMov_ = vector<AccMov>(linMod.nHorizon, var_fit_days_);
	vAccMovTarget_ = vector<AccMov>(linMod.nHorizon, var_fit_days_);
	double ridgeReg = 100 * 20. / linMod.gridInterval;
	biLinMod_ = BiLinearModel(linMod.nHorizon, linMod.num_time_slices, om_num_sig_, om_num_err_sig_, ridgeReg, ridgeReg, uids_, pid_, txt_corr_);

	if( HEnv::Instance()->multiThreadTicker() )
		nThreads_ = HEnv::Instance()->nMaxThreadTicker();
	vSig_ = vector<SigC>(nThreads_, SigC(linMod, nonLinMod));
	vSigInUse_ = vector<int>(nThreads_);
}

void HMakeSample::beginDay()
{
	int idate = HEnv::Instance()->idate();
	vector<string> markets = HEnv::Instance()->markets();
	const hff::LinearModel linMod = HEnv::Instance()->linearModel();
	string model = HEnv::Instance()->model();

	// okECNs.
	okECNs_.clear();
	for( vector<string>::const_iterator it = linMod.ecns.begin(); it != linMod.ecns.end(); ++it )
	{
		string market = *it;
		if( mto::dataOK(market, idate) )
			okECNs_.push_back(market);
	}

	bool doAfterHours = false;
	string market = markets[0];
	longTicker_ = mto::longTicker(market);

	if( write_bmtxt_ )
		writeSig::write_bmTxt_header(bmTxt_, model, idate);
	if( write_omtxt_ )
		writeSig::write_omTxt_header(omTxt_, model, idate);
	if( write_tmtxt_ )
		writeSig::write_tmTxt_header(tmTxt_, model, idate);

	bool write_bin = debug_write_bin_ || cntDay_ > linMod.om_univ_fit_days() + linMod.om_err_fit_days();
	if( write_bin )
	{
		if( write_ombin_ )
			writeSig::write_omBin_header(omBin_, omBinTxt_, model, idate);
		if( write_tmbin_ )
			writeSig::write_tmBin_header(tmBin_, tmBinTxt_, model, idate);
	}

	// Read hedge.
	string hPath = get_hedge_path(model, idate);
	hInfo_ = HedgeInfo(hPath, linMod.gpts);
	hValid_ = hInfo_.valid();

	// idatesFirst.
	{
		int idateFirstBiLin = 0;
		vector<int> idates = HEnv::Instance()->idates();
		if( cntDay_ >= linMod.om_univ_fit_days() )
			idateFirstBiLin = idates[cntDay_ - linMod.om_univ_fit_days()];
		else
			idateFirstBiLin = idates[0];

		biLinMod_.beginDay(idate/*, idateFirstBiLin, idateFirstBiLin*/);
	}

	{
		int idateFirstVar = 0;
		vector<int> idates = HEnv::Instance()->idates();
		if( cntDay_ >= var_fit_days_ )
			idateFirstVar = idates[cntDay_ - var_fit_days_ + 1];
		else
			idateFirstVar = idates[0];

		//varMod_.beginDay(idate, idateFirstVar);

		if( linMod.nHorizon > 1 )
		{
			for( vector<AccMov>::iterator it = vAccMov_.begin(); it != vAccMov_.end(); ++it )
				it->beginDay(idate, idateFirstVar);
			for( vector<AccMov>::iterator it = vAccMovTarget_.begin(); it != vAccMovTarget_.end(); ++it )
				it->beginDay(idate, idateFirstVar);

			for( int i = 0; i < vAccMov_.size(); ++i )
			{
				double rms = vAccMov_[i].getRMS();
				double mean = vAccMovTarget_[i].getRMS();
				//cout << rms << "\t" << rms * rms << "\t" << rms / mean << "\t" << (rms * rms) / mean << endl;
			}
		}
	}

	corrPr_.clear();
	corrPrTot_.clear();
	corrBm0_.clear();
	corrBm1_.clear();
	corrBm2_.clear();
	corrBm3_.clear();
	corrBm11_.clear();
	corrBm12_.clear();
	corrBm13_.clear();
	corrT20T40_.clear();
	corrP20T20_.clear();
	corrP40T40_.clear();
	corrP20T_.clear();
	corrP40T_.clear();
	corrPgT_.clear();
}

void HMakeSample::beginMarket()
{
	string market = HEnv::Instance()->market();
	const vector<string>& tickers = HEnv::Instance()->tickers();
	idate_ = HEnv::Instance()->idate();
	idate_p_ = prevClose(market, idate_);
	idate_pp_ = prevClose(market, idate_p_);
	idate_n_ = nextClose(market, idate_);
	longTicker_ = mto::longTicker(market);

	if( hValid_ )
		get_uid_map(market, idate_p_);

	for( map<boost::thread::id, TickTobAcc*>::iterator it = mIdTta_.begin(); it != mIdTta_.end(); ++it )
		delete it->second;
	mIdTta_.clear();
	mIdObaMap_.clear();
}

void HMakeSample::beginTicker(const string& ticker)
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
	const hff::NonLinearModel& nonLinMod = HEnv::Instance()->nonLinearModel();

	map<string, string>::iterator itT = mTickerUid_.find(ticker);
	if( itT != mTickerUid_.end() )
	{
		string uid = itT->second;

		int iSig = -1;
		SigC& sig = get_sig_object(iSig);

		// Read from stockcharacteristics table.
		bool read_chara_ok = read_chara_data(sig, market, uid, idate_, idate_p_, idate_pp_, idate_n_);
		if( read_chara_ok )
		{
			if( 0 )
			{
				// Read from the tick data.
				const vector<QuoteInfo>* quotes = static_cast<const vector<QuoteInfo>*>(HEvent::Instance()->get(ticker, "nbbo"));
				const vector<TradeInfo>* trades = static_cast<const vector<TradeInfo>*>(HEvent::Instance()->get(ticker, "trades"));

				if( quotes != 0 && trades != 0 && quotes->size() > min_quotes_ && trades->size() > min_trades_ )
				{
					//sig.dayok = 1;

					get_trade_stateinfo(sig, trades);
					get_quote_stateinfo(sig, quotes);
					get_filImb_stateinfo(sig, trades, quotes);
					get_targets_stateinfo(sig, quotes);

					// TOB signal.
					TickTobAcc* tta = get_tta(market, idate);
					get_tob_stateinfo(tta, sig, idate, ticker);

					// Orderbook signal.
					map<string, OrderBkAcc<OrderData> >& obaMap = get_obaMap(market, idate);
					get_book_stateinfo(obaMap, sig, market, idate, ticker, okECNs_);

					// Market Impact signal.
					const vector<OrderQty>* tradeQty = static_cast<const vector<OrderQty>*>(HEvent::Instance()->get(ticker, "tradeQty"));
					get_MI_signals(sig, tradeQty);

					get_signals(sig);
					get_prediction(sig, uid, idate);
					write_signal(sig, uid, ticker);
				}
			}
			else
			{
				vector<string> dirs = tSources_.nbbodirectory(market, idate);
				vector<QuoteInfo> quotes;
				read_tickdata(quotes, market, idate, ticker, dirs);

				vector<TradeInfo> trades;
				read_tickdata(trades, market, idate, ticker, dirs);

				if( quotes.size() > min_quotes_ && trades.size() > min_trades_ )
				{
					//sig.dayok = 1;

					get_trade_stateinfo(sig, &trades);
					get_quote_stateinfo(sig, &quotes);
					get_filImb_stateinfo(sig, &trades, &quotes);
					get_targets_stateinfo(sig, &quotes);

					//// TOB signal.
					//TickTobAcc* tta = new TickTobAcc("", longTicker_, false, linMod.openMsecs, linMod.closeMsecs);
					//tta->AddSpecificRoot( market, tSources_.stocksdirectory(market, idate), longTicker_ );
					//for( vector<string>::const_iterator it = okECNs_.begin(); it != okECNs_.end(); ++it )
					//{
					//	string market = *it;
					//	tta->AddSpecificRoot( market, tSources_.stocksdirectory(market, idate), longTicker_ );
					//}
					//get_tob_stateinfo(tta, sig, idate, ticker);
					//delete tta;

					//// Orderbook signal.
					//map<string, OrderBkAcc<OrderData> > obaMap;
					//obaMap[market] = OrderBkAcc<OrderData>();
					//obaMap[market].AddRoot( tSources_.bookdirectory(market, idate), longTicker_ );
					//for( vector<string>::const_iterator it = okECNs_.begin(); it != okECNs_.end(); ++it )
					//{
					//	string market = *it;
					//	obaMap[market] = OrderBkAcc<OrderData>();
					//	obaMap[market].AddRoot( tSources_.bookdirectory(market, idate), longTicker_ );
					//}
					//get_book_stateinfo(obaMap, sig, market, idate, ticker, okECNs_);

					// TOB signal.
					TickTobAcc* tta = new TickTobAcc("", longTicker_, false, linMod.openMsecs, linMod.closeMsecs);
					{
						// main market.
						vector<string> dirs = tSources_.stocksdirectory(market, idate);
						for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
							tta->AddSpecificRoot( market, *itd, longTicker_ );

						// ecns.
						for( vector<string>::const_iterator ite = okECNs_.begin(); ite != okECNs_.end(); ++ite )
						{
							string market = *ite;

							vector<string> dirs = tSources_.stocksdirectory(market, idate);
							for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
								tta->AddSpecificRoot( market, *itd, longTicker_ );
						}
						get_tob_stateinfo(tta, sig, idate, ticker);
						delete tta;
					}

					// Orderbook signal.
					map<string, OrderBkAcc<OrderData> > obaMap;
					{
						obaMap[market] = OrderBkAcc<OrderData>();

						// main market.
						vector<string> dirs = tSources_.bookdirectory(market, idate);
						for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
							obaMap[market].AddRoot( *itd, longTicker_ );

						// ecns.
						for( vector<string>::const_iterator ite = okECNs_.begin(); ite != okECNs_.end(); ++ite )
						{
							string market = *ite;
							obaMap[market] = OrderBkAcc<OrderData>();

							vector<string> dirs = tSources_.bookdirectory(market, idate);
							for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
								obaMap[market].AddRoot( *itd, longTicker_ );
						}
						get_book_stateinfo(obaMap, sig, market, idate, ticker, okECNs_);
					}

					// Market Impact signal.
					const vector<OrderQty>* tradeQty = static_cast<const vector<OrderQty>*>(HEvent::Instance()->get(ticker, "tradeQty"));
					get_MI_signals(sig, tradeQty);

					get_signals(sig);
					get_prediction(sig, uid, idate);
					write_signal(sig, uid, ticker);
				}
			}
		}
		free_sig_object(iSig);
	}
}

void HMakeSample::endTicker(const string& ticker)
{
}

void HMakeSample::endMarket()
{
}

void HMakeSample::endDay()
{
	if( write_bmtxt_ )
	{
		bmTxt_.close();
		bmTxt_.clear();
	}

	if( write_omtxt_ )
	{
		omTxt_.close();
		omTxt_.clear();
	}

	if( write_ombin_ )
	{
		writeSig::update_ncols(omBin_, cntOmBin_);

		omBin_.close();
		omBin_.clear();

		omBinTxt_.close();
		omBinTxt_.clear();

		cntOmBin_ = 0;
	}

	if( write_tmtxt_ )
	{
		tmTxt_.close();
		tmTxt_.clear();
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

	++cntDay_;

	cout << "corr " << corrPr_.corr() << " corrTot " << corrPrTot_.corr() << endl;
	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
	if( linMod.nHorizon > 1 )
	{
		cout << "corrT20T40 " << corrT20T40_.corr() << " corrP20T20 " << corrP20T20_.corr() << " corrP40T40 " << corrP40T40_.corr()
			<< " corrP20T " << corrP20T_.corr() << " corrP40T " << corrP40T_.corr() << " corrPgT " << corrPgT_.corr() << endl;

		cout << "corrBm0 " << corrBm0_.corr() << " corrBm1 " << corrBm1_.corr()
			<< " corrBm2 " << corrBm2_.corr() << " corrBm3 " << corrBm3_.corr()
			<< " corrBm11 " << corrBm11_.corr() << " corrBm12 " << corrBm12_.corr()
			<< " corrBm13 " << corrBm13_.corr() << endl;
	}
}

void HMakeSample::endJob()
{
	biLinMod_.endJob();
}

void HMakeSample::get_uid_map(string market, int idate_p)
{
	mTickerUid_.clear();

	char cmd[1000];
	sprintf(cmd, "select %s, uniqueID from stockcharacteristics "
		" where %s and uniqueID is not null ",
		mto::compTicker(market).c_str(),
		mto::selChara(market, idate_p).c_str());
	vector<vector<string> > vv;
	GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);

	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string ticker = trim((*it)[0]);
		string uid = trim((*it)[1]);
		if( uids_.count(uid) && !ticker.empty() )
			mTickerUid_[ticker] = uid;
	}
}

void HMakeSample::get_prediction(hff::SigC& sig, string uid, int idate)
{
	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
	const hff::NonLinearModel& nonLinMod = HEnv::Instance()->nonLinearModel();
	const vector<int>& useom = linMod.useom;
	const vector<int>& useomErr = linMod.useomErr;

	int timeFac = ceil( float(linMod.gpts - 1) / linMod.num_time_slices );
	for( int k = 0; k < linMod.gpts; ++k )
	{
		int timeIdx = k / timeFac;
		vector<hff::StateInfo>& sI = sig.sI;
		int time = k * linMod.gridInterval;

		// Hedge targets.
		for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
			sI[k].target[iT] -= hInfo_.vMean[k][iT];
		sI[k].target60Intra -= hInfo_.mean60Intra[k];
		sI[k].targetClose -= hInfo_.meanClose[k];
		sI[k].targetNextClose -= hInfo_.meanNextClose[k];

		if( sI[k].gptOK == 1 )
		{
			// Get predictors.
			for( int i = 0; i < hff::om_num_basic_sig_; ++i )
			{
				int indx1 = i;
				int indx2 = i + hff::om_num_basic_sig_;
				int indx3 = i + 2 * hff::om_num_basic_sig_;
				int indx4 = i + 3 * hff::om_num_basic_sig_;

				sI[k].om[indx1] = useom[indx1] * sI[k].sig1[i];
				sI[k].om[indx2] = useom[indx2] * sig.logVolu * sI[k].sig1[i];
				sI[k].om[indx3] = useom[indx3] * sig.logPrice * sI[k].sig1[i];
				sI[k].om[indx4] = useom[indx4] * sI[k].absSprd * sI[k].sig1[i];
			}

			for( int i = 0; i < hff::om_num_err_sig_; ++i )
				sI[k].omErr[i] = useomErr[i] * sI[k].sig1[i];

			// Calculate predictions.
			for( int iT = 0; iT < linMod.nHorizon; ++iT )
			{
				double pr = biLinMod_.pred(iT, timeIdx, sI[k].om);
				sI[k].pred[iT] = pr;
				sI[k].pr1 += pr;

				double prErr = biLinMod_.predErr(iT, uid, sI[k].omErr);
				sI[k].predErr[iT] = prErr;
				sI[k].pr1err += prErr;

				//double prVar = varMod_.pred(iT, sI[k].om);
				//sI[k].predVar[iT] = prVar;
			}

			// Get error targets.
			for( int iT = 0; iT < linMod.nHorizon; ++iT )
				sI[k].targetErr[iT] = sI[k].target[iT] - sI[k].pred[iT];
			for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
				sI[k].targetErr[iT] = sI[k].target[iT];

			// bmpred
			float bmpred_simple = sI[k].pr1 + sI[k].pr1err;
			//float bmpred = get_bmpred(sI[k].pred, sI[k].predErr);

			//if( cntDay_ >= om_fit_days_ * 2 + var_fit_days_ )
			//	sI[k].bmpred = get_bmpred(sI[k].pred, sI[k].predErr, sI[k].predVar);
			//else
			//	sI[k].bmpred = bmpred_simple;
			sI[k].bmpred = bmpred_simple;

			// update bilinear model.
			if( k >= skip_first_secs_ / linMod.gridInterval )
			{
				if( sig.inUnivFit == 1 )
				{
					int timeIdx = k / timeFac;
					for( int iT = 0; iT < linMod.nHorizon; ++iT )
					{
						boost::mutex::scoped_lock lock(biLinMod_mutex_);
						biLinMod_.update(iT, timeIdx, sI[k].om, sI[k].target[iT]);
					}

					// calculate corr.
					{
						// target1
						float target1 = 0.;
						for( int iT = 0; iT < linMod.nHorizon; ++iT )
							target1 += sI[k].target[iT];

						corrPr_.add(sI[k].pr1, target1);
						corrPrTot_.add(sI[k].pr1 + sI[k].pr1err, target1);

						if( linMod.nHorizon > 1 )
						{
							float bmpred1 = get_bmpred_fixed(sI[k].pred, sI[k].predErr, 0.5);
							float bmpred2 = get_bmpred_fixed(sI[k].pred, sI[k].predErr, 1.0);
							float bmpred3 = get_bmpred_fixed(sI[k].pred, sI[k].predErr, 1.5);
							float bmpred11 = get_bmpred_var(sI[k].pred, sI[k].predErr, 0.5);
							float bmpred12 = get_bmpred_var(sI[k].pred, sI[k].predErr, 1.0);
							float bmpred13 = get_bmpred_var(sI[k].pred, sI[k].predErr, 1.5);

							//double var0 = (sI[k].target[0] - (sI[k].pred[0] + sI[k].predErr[0]))*(sI[k].target[0] - (sI[k].pred[0] + sI[k].predErr[0]));
							//corrVar_.add(sI[k].predVar[0], var0);
							//corrBmF_.add(sI[k].pred[0] + sI[k].predErr[0], target1);
							corrBm0_.add(bmpred_simple, target1);
							corrBm1_.add(bmpred1, target1);
							corrBm2_.add(bmpred2, target1);
							corrBm3_.add(bmpred3, target1);
							corrBm11_.add(bmpred11, target1);
							corrBm12_.add(bmpred12, target1);
							corrBm13_.add(bmpred13, target1);
							corrT20T40_.add(sI[k].target[0], sI[k].target[1]);
							corrP20T20_.add(sI[k].pred[0] + sI[k].predErr[0], sI[k].target[0]);
							corrP40T40_.add(sI[k].pred[1] + sI[k].predErr[1], sI[k].target[1]);
							corrP20T_.add(sI[k].pred[0] + sI[k].predErr[0], target1);
							corrP40T_.add(sI[k].pred[1] + sI[k].predErr[0], target1);
							corrPgT_.add(sI[k].pred[0] + sI[k].pred[1], target1);
						}
					}
				}
				if( cntDay_ >= linMod.om_univ_fit_days() )
				{
					for( int iT = 0; iT < linMod.nHorizon; ++iT )
						biLinMod_.updateErr(iT, uid, sI[k].omErr, sI[k].targetErr[iT]);
				}
			}

			if( linMod.nHorizon > 1 )
			{
				// update variance model.
				//if( cntDay_ >= om_fit_days_ * 2 )
				{
					if( k >= skip_first_secs_ / linMod.gridInterval )
					{
						if( sig.inUnivFit == 1 )
						{
							for( int iT = 0; iT < linMod.nHorizon; ++iT )
							{
								double diff = sI[k].target[iT] - (sI[k].pred[iT] + sI[k].predErr[iT]);
								double var = diff * diff;
								boost::mutex::scoped_lock lock(varMod_mutex_);
								//varMod_.update(iT, sI[k].om, var);
								vAccMov_[iT].add( diff );
								vAccMovTarget_[iT].add(sI[k].target[iT]);
							}
						}
					}
				}
			}
		}
	}
}

void HMakeSample::write_signal(SigC& sig, string uid, string ticker)
{
	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();

	// Write weights.

	// Write signals.
	bool write_bin = debug_write_bin_ || cntDay_ > linMod.om_univ_fit_days() + linMod.om_err_fit_days();
	if( write_omtxt_ || write_ombin_ || write_bmtxt_ )
	{
		boost::mutex::scoped_lock lock(om_mutex_);
		if( write_bmtxt_ )
			writeSig::write_bmTxt(bmTxt_, sig, uid, ticker);
		if( write_omtxt_ )
			writeSig::write_omTxt(omTxt_, sig, uid, ticker);
		if( write_ombin_ && write_bin )
			writeSig::write_omBin(omBin_, omBinTxt_, sig, uid, ticker, cntOmBin_);
	}

	if( write_tmtxt_ || write_tmbin_ )
	{
		boost::mutex::scoped_lock lock(tm_mutex_);
		if( write_tmtxt_ )
			writeSig::write_tmTxt(tmTxt_, sig, uid, ticker);
		if( write_tmbin_ && write_bin )
			writeSig::write_tmBin(tmBin_, tmBinTxt_, sig, uid, ticker, cntTmBin_);
	}
}

SigC& HMakeSample::get_sig_object(int& iSig)
{
	for( int i = 0; i < nThreads_; ++i )
	{
		if( vSigInUse_[i] == 0 )
		{
			vSigInUse_[i] = 1;
			iSig = i;
			const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
			const hff::NonLinearModel& nonLinMod = HEnv::Instance()->nonLinearModel();
			vSig_[i].init(linMod, nonLinMod);
			return vSig_[i];
		}
	}
	exit(6);
	return vSig_[0];
}

void HMakeSample::free_sig_object(int iSig)
{
	vSigInUse_[iSig] = 0;
}

TickTobAcc* HMakeSample::get_tta(string market, int idate)
{
	boost::thread::id id = boost::this_thread::get_id();
	map<boost::thread::id, TickTobAcc*>::iterator it = mIdTta_.find(id);
	if( it == mIdTta_.end() )
	{
		TickTobAcc* tta = new TickTobAcc("", longTicker_, false, HEnv::Instance()->linearModel().openMsecs, HEnv::Instance()->linearModel().closeMsecs);

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
		{
			boost::mutex::scoped_lock lock(tta_mutex_);
			mIdTta_[id] = tta;
		}
		it = mIdTta_.find(id);
	}
	return it->second;
}

map<string, OrderBkAcc<OrderData> >& HMakeSample::get_obaMap(string market, int idate)
{
	boost::thread::id id = boost::this_thread::get_id();
	map<boost::thread::id, map<string, OrderBkAcc<OrderData> > >::iterator it = mIdObaMap_.find(id);
	if( it == mIdObaMap_.end() )
	{
		{
			boost::mutex::scoped_lock lock(oba_mutex_);
			mIdObaMap_[id][market] = OrderBkAcc<OrderData>();
		}
		it = mIdObaMap_.find(id);

		// main market.
		vector<string> dirs = tSources_.bookdirectory(market, idate);
		for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
			it->second[market].AddRoot( *itd, longTicker_ );

		// ecns.
		for( vector<string>::const_iterator ite = okECNs_.begin(); ite != okECNs_.end(); ++ite )
		{
			string market = *ite;
			it->second[market] = OrderBkAcc<OrderData>();

			vector<string> dirs = tSources_.bookdirectory(market, idate);
			for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
				it->second[market].AddRoot( *itd, longTicker_ );
		}
	}
	return it->second;
}

double HMakeSample::get_bmpred(vector<float>& pred, vector<float>& predErr, vector<float>& predVar)
{
	int N = predVar.size();
	float minVar = -1.;
	int indxMinVar = -1;
	for( int i = 0; i < N; ++i )
	{
		if( minVar < 0 || predVar[i] < minVar )
		{
			minVar = predVar[i];
			indxMinVar = i;
		}
	}

	double bmpred = 0.;
	if( minVar > 0 && indxMinVar > 0 )
	{
		for( int i = 0; i < N; ++i )
		{
			double fac = 1.;
			if( i != indxMinVar )
				fac = minVar / predVar[i];
			bmpred += (pred[i] + predErr[i]) * fac;
		}
		return bmpred;
	}
	else
	{
		for( int i = 0; i < N; ++i )
			bmpred += (pred[i] + predErr[i]);
	}

	return bmpred;
}

double HMakeSample::get_bmpred_fixed(vector<float>& pred, vector<float>& predErr, double power)
{
	int N = pred.size();
	double bmpred = 0.;
	for( int i = 0; i < N; ++i )
	{
		double fac = pow((double)(i + 1), power);
		bmpred += (pred[i] + predErr[i]) / fac;
	}

	return bmpred;
}

double HMakeSample::get_bmpred_var(vector<float>& pred, vector<float>& predErr, double power)
{
	vector<float> vVar;
	for( vector<AccMov>::iterator it = vAccMov_.begin(); it != vAccMov_.end(); ++it )
	{
		double rms = it->getRMS();
		vVar.push_back( rms );
	}

	int N = vVar.size();

	float minVar = -1.;
	int indxMinVar = -1;
	for( int i = 0; i < N; ++i )
	{
		if( minVar < 0 || vVar[i] < minVar )
		{
			minVar = vVar[i];
			indxMinVar = i;
		}
	}

	double bmpred = 0.;
	if( minVar > 0 && indxMinVar >= 0 )
	{
		for( int i = 0; i < N; ++i )
		{
			double fac = 1.;
			if( i != indxMinVar )
				fac = pow((double)minVar / vVar[i], power);
			bmpred += (pred[i] + predErr[i]) * fac;
		}
		return bmpred;
	}
	else
	{
		for( int i = 0; i < N; ++i )
			bmpred += (pred[i] + predErr[i]);
	}

	return bmpred;
}
