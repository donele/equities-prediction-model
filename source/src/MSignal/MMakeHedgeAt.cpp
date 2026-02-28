#include <MSignal/MMakeHedgeAt.h>
#include <MSignal/sig.h>
#include <MSignal/SigSet.h>
#include <gtlib_model/mFtns.h>
#include <jl_lib/jlutil_tickdata.h>
#include <jl_lib/Sessions.h>
#include <MFramework.h>
#include <map>
#include <string>
using namespace std;
using namespace hff;
using namespace sig;
using namespace gtlib;

MMakeHedgeAt::MMakeHedgeAt(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName, true),
debug_(false),
iTicker_(0),
verbose_(0),
nThreads_(1)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("hedgingMethod") )
		hedging_method_ = conf.find("hedgingMethod")->second;
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("nThreads") )
		nThreads_ = atoi( conf.find("nThreads")->second.c_str() );
}

MMakeHedgeAt::~MMakeHedgeAt()
{}

void MMakeHedgeAt::beginJob()
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

void MMakeHedgeAt::beginDay()
{
	int idate = MEnv::Instance()->idate;
	string model = MEnv::Instance()->model;
	iTicker_ = 0;

	if( verbose_ > 0 )
	{
		cout << "\MMakeHedgeAt::beginDay() " << idate << ".\n";
		if( verbose_ > 1 )
			PrintMemoryInfoSimple();
	}
	vSigH_.clear();
	MEvent::Instance()->remove<vector<hff::SigH> >("", "vSigH");

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
								new boost::thread(boost::bind(&MMakeHedgeAt::hedge_begin_ticker_thread, this, i))
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

void MMakeHedgeAt::beginMarket()
{
}

void MMakeHedgeAt::hedge_begin_market()
{
	GTH::Instance()->init(market_);
	idate_ = MEnv::Instance()->idate;
	idate_p_ = prevClose(market_, idate_);
	idate_pp_ = prevClose(market_, idate_p_);
	idate_n_ = nextClose(market_, idate_);
	sessions_ = Sessions(market_, idate_);
}

void MMakeHedgeAt::beginTicker(const string& ticker)
{
}

void MMakeHedgeAt::hedge_begin_ticker_thread(int iThread)
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

void MMakeHedgeAt::hedge_begin_ticker(int iTicker)
{
	string model = MEnv::Instance()->model;
	string ticker = tickers_[iTicker];
	int idate = MEnv::Instance()->idate;
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	const NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	const int& n1sec = linMod.n1sec;
	string uid = GTH::Instance()->get(market_)->TickerToUnique(base_name(ticker), idate);
	if( uids_.count(uid) )
	{
		int iSig = -1;
		SigC& sig = SigSet::Instance()->get_sig_object(iSig, n1sec);
		if( read_chara_weight(sig, model, market_, uid, idate_p_) )
		{
			string reg = mto::region(market_);
			vector<string> dirs = tSources_.nbbodirectory(market_, idate);

			vector<QuoteInfo> quotes;
			{
				boost::mutex::scoped_lock lock(q_mutex_);
				read_tickdata(quotes, market_, idate, ticker, dirs, sessions_);
			}

			vector<TradeInfo> trades;
			{
				boost::mutex::scoped_lock lock(t_mutex_);
				read_tickdata(trades, market_, idate, ticker, dirs, sessions_);
			}

			if( quotes.size() > min_quotes_ && trades.size() > min_trades_ )
			{
				if( !trades.empty() )
				{
					int firstTradeSec = (trades[0].msecs - linMod.openMsecs) / 1000;

					vector<double> vMid;
					get_mid_series(vMid, &quotes, linMod.openMsecs, linMod.closeMsecs, 0, false, true);

					for( int sec = firstTradeSec + 1; sec < n1sec - 1; ++sec )
					{
						// future 1 second return.
						double mid0 = vMid[sec];
						double mid1 = vMid[sec + 1];
						double ret = 0.;
						if( mid0 > 0. && mid1 > 0. )
						{
							ret = basis_pts_ * (mid1 / mid0 - 1.);
							clip(ret, sec_ret_clip_);
							if( debug_ && ticker == "S:CREM3" )
								printf("%s %d %.4f\n", sig.ticker.c_str(), sec, ret);
							sig.sI[sec].target[0] = ret;
							sig.sI[sec].gptOK = 1;

							//printf("Ret %10s%6d%9.3f\n", ticker.c_str(), sec, ret);
						}
					}
				}
			}
		}

		boost::mutex::scoped_lock lock(private_mutex_);
		vSigH_.push_back(hff::SigH(sig, ticker));
		SigSet::Instance()->free_sig_object(iSig);
	}
}

void MMakeHedgeAt::endTicker(const string& ticker)
{
}

void MMakeHedgeAt::endDay()
{
	MEvent::Instance()->remove<vector<double> >("", "hedge");
}

void MMakeHedgeAt::endJob()
{
}

void MMakeHedgeAt::get_hedge()
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

		// Hedge 1 second targets.
		for( int k = 0; k < linMod.n1sec - 1; ++k )
		{
			for( int i = 0; i < hedgeN; ++i )
			{
				int indx = vIndx[i];
				signals[i] = vSigH_[indx].sIH[k].target[0];
			}

			nfh.Hedge(&signals[0], &hedgedSignals[0]);
			for( int i = 0; i < hedgeN; ++i )
			{
				int indx = vIndx[i];
				vSigH_[indx].sIH[k].target[0] = hedgedSignals[i];
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

			for( int k = 0; k < linMod.n1sec - 1; ++k )
			{
				// Hedge 1 second targets.
				for( int f = 0; f < nFact; ++f )
				{
					// Calculate factorWeight.
					double factorWeight = 0.;
					for( int i = 0; i < hedgeN; ++i )
						factorWeight += factorData[f][i] * vSigH_[vIndx[i]].sIH[k].target[0];

					// Hedge.
					for( int i = 0; i < hedgeN; ++i )
						vSigH_[vIndx[i]].sIH[k].target[0] -= factorWeight * factorData[f][i];
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

		float debugSum = 0.;
		if( hedgeN > 0 )
		{
			for( int k = 0; k < linMod.n1sec - 1; ++k )
			{
				// Get mean.
				double sum = 0.;
				int n = 0;
				for( int i = 0; i < hedgeN; ++i )
				{
					if( vSigH_[vIndx[i]].sIH[k].gptOK == 1 )
					{
						sum += vSigH_[vIndx[i]].sIH[k].target[0];
						++n;

						//if( debug_ && k == 15666 )
						//{
						  //printf("%d %s %.4f\n", k, vSigH_[vIndx[i]].ticker.c_str(), vSigH_[vIndx[i]].sIH[k].target[0]);
						//}
					}
				}

				if( n > 0 )
				{
					// Hedge.
					double mean = sum / n;
					for( int i = 0; i < hedgeN; ++i )
					{
						if( debug_ && vSigH_[vIndx[i]].ticker == "S:CREM3" )
						{
							debugSum += vSigH_[vIndx[i]].sIH[k].target[0] - mean;
							printf("%s %d %.4f %.4f %.4f\n", vSigH_[vIndx[i]].ticker.c_str(), k, vSigH_[vIndx[i]].sIH[k].target[0], mean, debugSum);
						}
						vSigH_[vIndx[i]].sIH[k].target[0] -= mean;
					}
				}
			}
		}
	}

	// add.
	MEvent::Instance()->add<vector<hff::SigH> >("", "vSigH", vSigH_);
}

void MMakeHedgeAt::get_hedge_signal(NorthfieldHedge& nfh)
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
