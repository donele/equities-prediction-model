#include <MSignal/MMakeHedge.h>
#include <MSignal/sig.h>
#include <MSignal/SigSet.h>
#include <jl_lib/jlutil_tickdata.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GTH.h>
#include <MFramework.h>
#include <map>
#include <string>
using namespace std;
using namespace hff;
using namespace sig;

MMakeHedge::MMakeHedge(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName, true),
debug_(false),
debug_sprd_(false),
write_(false),
iTicker_(0),
verbose_(0),
nThreads_(1)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debugSprd") )
		debug_sprd_ = conf.find("debugSprd")->second == "true";
	if( conf.count("hedgingMethod") )
		hedging_method_ = conf.find("hedgingMethod")->second;
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("nThreads") )
		nThreads_ = atoi( conf.find("nThreads")->second.c_str() );
}

MMakeHedge::~MMakeHedge()
{}

void MMakeHedge::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	uids_ = MEvent::Instance()->get<set<string> >("", "allUids");
	tSources_.read(mto::region(MEnv::Instance()->markets[0]), MEnv::Instance()->sourceFlag);
	string model = MEnv::Instance()->model;

	if( hedging_method_.empty() )
	{
		if( "US" == model.substr(0, 2) )
			hedging_method_ = "nf";
		else
			hedging_method_ = "mkt";
	}

	SigSet::Instance()->resize(nThreads_);
}

void MMakeHedge::beginDay()
{
	int idate = MEnv::Instance()->idate;
	string model = MEnv::Instance()->model;
	iTicker_ = 0;

	if( verbose_ > 0 )
	{
		cout << "\MMakeHedge::beginDay() " << idate << ".\n";
		if( verbose_ > 1 )
			PrintMemoryInfoSimple();
	}
	vSigH_.clear();
	if( !write_ )
		MEvent::Instance()->remove<vector<hff::SigH> >("", "vSigH");

	if( !write_ )
	{
		const map<string, vector<string> >* marketTickers = static_cast<const map<string, vector<string> >*>(MEvent::Instance()->get("", "marketTickers"));
		vector<string> markets = MEnv::Instance()->markets;
		for( vector<string>::iterator itm = markets.begin(); itm != markets.end(); ++itm )
		{
			market_ = *itm;
			hedge_begin_market();

			// ticker loop.
			map<string, vector<string> >::const_iterator itt = marketTickers->find(market_);
			if( itt != marketTickers->end() )
			{
				tickers_ = MEnv::Instance()->tickersDebug;
				if( tickers_.empty() )
					tickers_ = itt->second;

				if( tickers_.size() > MEnv::Instance()->nTickerMax )
					tickers_ = vector<string>(tickers_.begin(), tickers_.begin() + MEnv::Instance()->nTickerMax);

				if( nThreads_ < 2 )
				{
					int nTickers = tickers_.size();
					for( int i = 0; i < nTickers; ++i )
						hedge_begin_ticker(i);
				}
				else
				{
					list<boost::shared_ptr<boost::thread> > listThread;
					for( int i = 0; i < nThreads_; ++i )
					{
						listThread.push_back(
							boost::shared_ptr<boost::thread>(
								new boost::thread(boost::bind(&MMakeHedge::hedge_begin_ticker_thread, this, i))
							)
						);
					}

					// join ticker threads.
					for( list<boost::shared_ptr<boost::thread> >::iterator it = listThread.begin(); it != listThread.end(); ++it )
						(*it)->join();
				}
			}
		}
		get_hedge();
	}
}

void MMakeHedge::beginMarket()
{
	if( write_ )
	{
		market_ = MEnv::Instance()->market;
		hedge_begin_market();
	}
}

void MMakeHedge::hedge_begin_market()
{
	GTH::Instance()->init(market_);
	idate_ = MEnv::Instance()->idate;
	idate_p_ = prevClose(market_, idate_);
	idate_pp_ = prevClose(market_, idate_p_);
	idate_n_ = nextClose(market_, idate_);
	sessions_ = Sessions(market_, idate_);
}

void MMakeHedge::beginTicker(const string& ticker)
{
	if( write_ )
	{
		market_ = MEnv::Instance()->market;
		hedge_begin_ticker(iTicker_++);
	}
}

void MMakeHedge::hedge_begin_ticker_thread(int iThread)
{
	int nTickers = tickers_.size();
	int nTickerEachThread = ceil((float)nTickers / nThreads_);
	int indx1 = iThread * nTickerEachThread;
	if( indx1 < nTickers )
	{
		int indx2 = (iThread + 1) * nTickerEachThread;
		if( indx2 > nTickers || iThread == nThreads_ - 1 )
			indx2 = nTickers;

		if( indx2 > indx1 )
		{
			for( int i = indx1; i < indx2; ++i )
				hedge_begin_ticker(i);
		}
	}
}

void MMakeHedge::hedge_begin_ticker(int iTicker)
{
	string model = MEnv::Instance()->model;
	string ticker = tickers_[iTicker];
	int idate = MEnv::Instance()->idate;
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	const NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	string uid = GTH::Instance()->get(market_)->TickerToUnique(base_name(ticker), idate);
	if( uids_.count(uid) )
	{
		int iSig = -1;
		SigC& sig = SigSet::Instance()->get_sig_object(iSig);
		if( read_chara_weight(sig, model, market_, uid, idate_p_) )
		{
			string reg = mto::region(market_);

			vector<string> dirs = tSources_.nbbodirectory(market_, idate);

			vector<QuoteInfo> quotes;
			read_tickdata(quotes, market_, idate, ticker, dirs, sessions_);

			vector<TradeInfo> trades;
			read_tickdata(trades, market_, idate, ticker, dirs, sessions_);

			if( quotes.size() > min_quotes_ && trades.size() > min_trades_ )
			{
				get_trade_stateinfo(sig, &trades);
				get_quote_stateinfo(sig, &quotes, debug_sprd_);
				get_targets_stateinfo(sig, &quotes);
			}
		}
		get_signals(sig, sessions_);
		boost::mutex::scoped_lock lock(private_mutex_);
		vSigH_.push_back(hff::SigH(sig, ticker));
		SigSet::Instance()->free_sig_object(iSig);
	}
}

void MMakeHedge::endTicker(const string& ticker)
{
}

void MMakeHedge::endDay()
{
	if( write_ )
	{
		int idate = MEnv::Instance()->idate;
		if( verbose_ > 0 )
		{
			cout << "\MMakeHedge::endDay() " << idate << ".\n";
			if( verbose_ > 1 )
				PrintMemoryInfoSimple();
		}
		get_hedge();
	}
	else
	{
		MEvent::Instance()->remove<vector<double> >("", "hedge");
	}
}

void MMakeHedge::endJob()
{
}

void MMakeHedge::get_hedge()
{
	int idate = MEnv::Instance()->idate;
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	const NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	string model = MEnv::Instance()->model;

	if( hedging_method_ == "nf" )
	{
		sort(vSigH_.begin(), vSigH_.end(), hff::SigHLess());
		NorthfieldHedge nfh = NorthfieldHedge(idate, "equityData");

		vector<int> vIndx;
		int totN = vSigH_.size();
		for( int i = 0; i < totN; ++i )
		{
			if( vSigH_[i].inUnivFit == 1 )
			{
				nfh.AddToUniverse(vSigH_[i].ticker);
				vIndx.push_back(i);
			}
		}
		int hedgeN = vIndx.size();

		nfh.CalcHedgeMatrices(2, true);
		get_hedge_signal(nfh);

		vector<double> signals(hedgeN);
		vector<double> hedgedSignals(hedgeN);

		// Overnight target.
		{
			for( int i = 0; i < hedgeN; ++i )
			{
				int indx = vIndx[i];
				signals[i] = vSigH_[indx].tarON;
			}

			nfh.Hedge(&signals[0], &hedgedSignals[0]);
			for( int i = 0; i < hedgeN; ++i )
			{
				int indx = vIndx[i];
				vSigH_[indx].tarON = hedgedSignals[i];
			}
		}

		for( int k = 0; k < linMod.gpts; ++k )
		{

			// Hedge targets.
			for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
			{
				for( int i = 0; i < hedgeN; ++i )
				{
					int indx = vIndx[i];
					signals[i] = vSigH_[indx].sIH[k].target[iT];
				}

				nfh.Hedge(&signals[0], &hedgedSignals[0]);
				for( int i = 0; i < hedgeN; ++i )
				{
					int indx = vIndx[i];
					vSigH_[indx].sIH[k].target[iT] = hedgedSignals[i];
				}
			}

			// Hedge target60Intra.
			{
				for( int i = 0; i < hedgeN; ++i )
				{
					int indx = vIndx[i];
					signals[i] = vSigH_[indx].sIH[k].target60Intra;
				}

				nfh.Hedge(&signals[0], &hedgedSignals[0]);
				for( int i = 0; i < hedgeN; ++i )
				{
					int indx = vIndx[i];
					vSigH_[indx].sIH[k].target60Intra = hedgedSignals[i];
				}
			}

			// Hedge targetClose.
			{
				for( int i = 0; i < hedgeN; ++i )
				{
					int indx = vIndx[i];
					signals[i] = vSigH_[indx].sIH[k].targetClose;
				}

				nfh.Hedge(&signals[0], &hedgedSignals[0]);
				for( int i = 0; i < hedgeN; ++i )
				{
					int indx = vIndx[i];
					vSigH_[indx].sIH[k].targetClose = hedgedSignals[i];
				}
			}

			// Hedge targetNextClose.
			{
				for( int i = 0; i < hedgeN; ++i )
				{
					int indx = vIndx[i];
					signals[i] = vSigH_[indx].sIH[k].targetNextClose;
				}

				nfh.Hedge(&signals[0], &hedgedSignals[0]);
				for( int i = 0; i < hedgeN; ++i )
				{
					int indx = vIndx[i];
					vSigH_[indx].sIH[k].targetNextClose = hedgedSignals[i];
				}
			}
		}
	}
	else if( hedging_method_ == "factor" )
	{
		string rmDir = "L:\\crcls\\us";
		bool useUID = false;
		const unsigned nFact = 20;

		// Get hedgeN.
		vector<int> vIndx;
		int totN = vSigH_.size();
		for( int i = 0; i < totN; ++i )
		{
			if( vSigH_[i].inUnivFit == 1 )
				vIndx.push_back(i);
		}
		int hedgeN = vIndx.size();

		// symbols.
		vector<string> retVect(hedgeN);
		vector<string> symVect(hedgeN);
		for( int i = 0; i < hedgeN; ++i )
			symVect[i] = vSigH_[vIndx[i]].ticker;

		// Load factor data.
		double** factorData = 0;
		ReadEVFileForSyms(idate, rmDir, symVect, &factorData, nFact, useUID);

		if( factorData != 0 )
		{
			// Normalize the factor data.
			for( int f = 0; f < nFact; ++f )
			{
				double norm = 0.;
				for( int i = 0; i < hedgeN; ++i )
					norm += factorData[f][i] * factorData[f][i];
				norm = sqrt(norm);
				for( int i = 0; i < hedgeN; ++i )
					factorData[f][i] /= norm;
			}

			// Overnight target.
			{
				for( int f = 0; f < nFact; ++f )
				{
					// Calculate factorWeight.
					double factorWeight = 0.;
					for( int i = 0; i < hedgeN; ++i )
						factorWeight += factorData[f][i] * vSigH_[vIndx[i]].tarON;

					// Hedge.
					for( int i = 0; i < hedgeN; ++i )
						vSigH_[vIndx[i]].tarON -= factorWeight * factorData[f][i];
				}
			}

			// Intraday targets.
			for( int k = 0; k < linMod.gpts; ++k )
			{
				// Hedge targets.
				for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
				{
					for( int f = 0; f < nFact; ++f )
					{
						// Calculate factorWeight.
						double factorWeight = 0.;
						for( int i = 0; i < hedgeN; ++i )
							if( vSigH_[vIndx[i]].sIH[k].gptOK == 1 )
								factorWeight += factorData[f][i] * vSigH_[vIndx[i]].sIH[k].target[iT];

						// Hedge.
						for( int i = 0; i < hedgeN; ++i )
							vSigH_[vIndx[i]].sIH[k].target[iT] -= factorWeight * factorData[f][i];
					}
				}

				// Hedge target60Intra.
				{
					for( int f = 0; f < nFact; ++f )
					{
						// Calculate factorWeight.
						double factorWeight = 0.;
						for( int i = 0; i < hedgeN; ++i )
							if( vSigH_[vIndx[i]].sIH[k].gptOK == 1 )
								factorWeight += factorData[f][i] * vSigH_[vIndx[i]].sIH[k].target60Intra;

						// Hedge.
						for( int i = 0; i < hedgeN; ++i )
							vSigH_[vIndx[i]].sIH[k].target60Intra -= factorWeight * factorData[f][i];
					}
				}

				// Hedge targetClose.
				{
					for( int f = 0; f < nFact; ++f )
					{
						// Calculate factorWeight.
						double factorWeight = 0.;
						for( int i = 0; i < hedgeN; ++i )
							if( vSigH_[vIndx[i]].sIH[k].gptOK == 1 )
								factorWeight += factorData[f][i] * vSigH_[vIndx[i]].sIH[k].targetClose;

						// Hedge.
						for( int i = 0; i < hedgeN; ++i )
							vSigH_[vIndx[i]].sIH[k].targetClose -= factorWeight * factorData[f][i];
					}
				}

				// Hedge targetNextClose.
				{
					for( int f = 0; f < nFact; ++f )
					{
						// Calculate factorWeight.
						double factorWeight = 0.;
						for( int i = 0; i < hedgeN; ++i )
							if( vSigH_[vIndx[i]].sIH[k].gptOK == 1 )
								factorWeight += factorData[f][i] * vSigH_[vIndx[i]].sIH[k].targetNextClose;

						// Hedge.
						for( int i = 0; i < hedgeN; ++i )
							vSigH_[vIndx[i]].sIH[k].targetNextClose -= factorWeight * factorData[f][i];
					}
				}
			}

			for( int f = 0; f < nFact; ++f )
				delete[] factorData[f];
			delete factorData;
		}
		else
		{
			cout << "Factor hedging failed." << endl;
		}
	}
	else if( hedging_method_ == "mkt" )
	{
		// Get hedgeN.
		vector<int> vIndx;
		int totN = vSigH_.size();
		for( int i = 0; i < totN; ++i )
		{
			if( vSigH_[i].inUnivFit == 1 )
				vIndx.push_back(i);
		}
		int hedgeN = vIndx.size();

		if( hedgeN > 0 )
		{
			// Overnight target.
			{
				// Get mean.
				double sum = 0.;
				int n = 0;
				for( int i = 0; i < hedgeN; ++i )
				{
					sum += vSigH_[vIndx[i]].tarON;
					++n;
				}

				if( n > 0 )
				{
					// Hedge.
					double mean = sum / n;
					for( int i = 0; i < hedgeN; ++i )
						vSigH_[vIndx[i]].tarON -= mean;
				}
			}

			// Intraday targets.
			for( int k = 0; k < linMod.gpts; ++k )
			{
				// Hedge targets.
				for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
				{
					// Get mean.
					double sum = 0.;
					int n = 0;
					for( int i = 0; i < hedgeN; ++i )
					{
						if( vSigH_[vIndx[i]].sIH[k].gptOK == 1 )
						{
							sum += vSigH_[vIndx[i]].sIH[k].target[iT];
							++n;
						}
					}

					if( n > 0 )
					{
						// Hedge.
						double mean = sum / n;
						for( int i = 0; i < hedgeN; ++i )
							vSigH_[vIndx[i]].sIH[k].target[iT] -= mean;
					}
				}

				// Hedge target60Intra.
				{
					// Get mean.
					double sum = 0.;
					int n = 0;
					for( int i = 0; i < hedgeN; ++i )
					{
						if( vSigH_[vIndx[i]].sIH[k].gptOK == 1 )
						{
							sum += vSigH_[vIndx[i]].sIH[k].target60Intra;
							++n;
						}
					}

					if( n > 0 )
					{
						// Hedge.
						double mean = sum / n;
						for( int i = 0; i < hedgeN; ++i )
							vSigH_[vIndx[i]].sIH[k].target60Intra -= mean;
					}
				}

				// Hedge targetClose.
				{
					// Get mean.
					double sum = 0.;
					int n = 0;
					for( int i = 0; i < hedgeN; ++i )
					{
						if( vSigH_[vIndx[i]].sIH[k].gptOK == 1 )
						{
							sum += vSigH_[vIndx[i]].sIH[k].targetClose;
							++n;
						}
					}

					if( n > 0 )
					{
						// Hedge.
						double mean = sum / n;
						for( int i = 0; i < hedgeN; ++i )
							vSigH_[vIndx[i]].sIH[k].targetClose -= mean;
					}
				}

				// Hedge targetNextClose.
				{
					// Get mean.
					double sum = 0.;
					int n = 0;
					for( int i = 0; i < hedgeN; ++i )
					{
						if( vSigH_[vIndx[i]].sIH[k].gptOK == 1 )
						{
							sum += vSigH_[vIndx[i]].sIH[k].targetNextClose;
							++n;
						}
					}

					if( n > 0 )
					{
						// Hedge.
						double mean = sum / n;
						for( int i = 0; i < hedgeN; ++i )
							vSigH_[vIndx[i]].sIH[k].targetNextClose -= mean;
					}
				}
			}
		}
	}

	// add.
	if( !write_ )
		MEvent::Instance()->add<vector<hff::SigH> >("", "vSigH", vSigH_);
}

void MMakeHedge::get_hedge_signal(NorthfieldHedge& nfh)
{
	vector<int> missingIndx;
	double mnorthbp = 0.;
	double mnorthtrd = 0.;
	double mnorthrst = 0.;
	int nok = 0;
	int totN = vSigH_.size();
	for( int i = 0; i < totN; ++i )
	{
		SigH& sigh = vSigH_[i];

		NFStockParams nfParams;
		if( nfh.GetParams(sigh.ticker, &nfParams) && nfParams.sector >= 0 )
		{
			sigh.northBP = nfParams.northFactors[2];
			sigh.northTRD = nfParams.northFactors[4];
			sigh.northRST = nfParams.northFactors[5];

			mnorthbp += nfParams.northFactors[2];
			mnorthtrd += nfParams.northFactors[4];
			mnorthrst += nfParams.northFactors[5];
			++nok;
		}
		else
			missingIndx.push_back(i);
	}

	if( nok > 0 )
	{
		mnorthbp /= nok;
		mnorthtrd /= nok;
		mnorthrst /= nok;
	}

	int missingN = missingIndx.size();
	for( int i = 0; i < missingN; ++i )
	{
		int indx = missingIndx[i];
		SigH& sigh = vSigH_[indx];
		{
			sigh.northBP = mnorthbp;
			sigh.northTRD = mnorthtrd;
			sigh.northRST = mnorthrst;
		}
	}
}
