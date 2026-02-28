#include <HFitting/HMakeHedge.h>
#include <HFitting/sig.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <map>
#include <string>
#include "TFile.h"
using namespace std;
using namespace hff;
using namespace sig;

HMakeHedge::HMakeHedge(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
debug_(false),
dayOK_(false),
verbose_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
}

HMakeHedge::~HMakeHedge()
{}

void HMakeHedge::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	uids_ = HEvent::Instance()->get<set<string> >("", "allUids");
}

void HMakeHedge::beginDay()
{
	int idate = HEnv::Instance()->idate();
	if( verbose_ >= 1 )
	{
		cout << "\HMakeHedge::beginDay() " << idate << ".\n";
		if( verbose_ > 1 )
			PrintMemoryInfoSimple();
	}
	mSigH_.clear();
	if( verbose_ >= 1 )
	{
		cout << "\HMakeHedge::beginDay() " << idate << ".\n";
		if( verbose_ > 1 )
			PrintMemoryInfoSimple();
	}
}

void HMakeHedge::beginMarket()
{
	string market = HEnv::Instance()->market();
	GTH::Instance()->init(market);
	idate_ = HEnv::Instance()->idate();
	idate_p_ = prevClose(market, idate_);
	idate_pp_ = prevClose(market, idate_p_);
	idate_n_ = nextClose(market, idate_);
	dayOK_ = mto::dataOK(market, idate_);
}

void HMakeHedge::beginTicker(const string& ticker)
{
	if( dayOK_ )
	{
		int idate = HEnv::Instance()->idate();
		string market = HEnv::Instance()->market();
		const LinearModel& linMod = HEnv::Instance()->linearModel();
		const NonLinearModel& nonLinMod = HEnv::Instance()->nonLinearModel();
		string uid = GTH::Instance()->get(market)->TickerToUnique(base_name(ticker), idate);
		if( uids_.count(uid) )
		{
			SigC sig(linMod, nonLinMod);
			// Read from stockcharacteristics table.
			if( read_chara_data(sig, market, uid, idate_, idate_p_, idate_pp_, idate_n_) )
			{
				// Read from the tick data.
				string reg = mto::region(market);
				//sig.dayok = 0;
				const vector<QuoteInfo>* quotes = 0;
				if( reg == "E" || reg == "C" || reg == "U" )
					quotes = static_cast<const vector<QuoteInfo>*>(HEvent::Instance()->get(ticker, "nbbo"));
				else
					quotes = static_cast<const vector<QuoteInfo>*>(HEvent::Instance()->get(ticker, "quotes"));
				const vector<TradeInfo>* trades = static_cast<const vector<TradeInfo>*>(HEvent::Instance()->get(ticker, "trades"));
				if( quotes != 0 && trades != 0 && quotes->size() > min_quotes_ && trades->size() > min_trades_ )
				{
					//sig.dayok = 1;
					get_trade_stateinfo(sig, trades);
					get_quote_stateinfo(sig, quotes);
					get_filImb_stateinfo(sig, trades, quotes);
					get_targets_stateinfo(sig, quotes);
				}
				boost::mutex::scoped_lock lock(private_mutex_);
				mSigH_[uid] = hff::SigH(sig, ticker);
			}
		}
	}
}

void HMakeHedge::endTicker(const string& ticker)
{
}

void HMakeHedge::endDay()
{
	int idate = HEnv::Instance()->idate();
	if( verbose_ >= 1 )
	{
		cout << "\HMakeHedge::endDay() " << idate << ".\n";
		if( verbose_ > 1 )
			PrintMemoryInfoSimple();
	}
	get_hedge();
}

void HMakeHedge::endJob()
{
}

void HMakeHedge::get_hedge()
{
	const LinearModel& linMod = HEnv::Instance()->linearModel();
	const NonLinearModel& nonLinMod = HEnv::Instance()->nonLinearModel();

	vector<double> vMean;
	int timeFac = ceil( float(linMod.gpts - 1) / linMod.num_time_slices );
	for( int k = 0; k < linMod.gpts; ++k )
	{
		// Get N.
		int hedgeN = 0;
		for( map<string, SigH>::iterator it = mSigH_.begin(); it != mSigH_.end(); ++it )
		{
			if( it->second.inUnivFit == 1 )
				++hedgeN;
		}

		// Hedge targets.
		for( int iT = 0; iT < nonLinMod.nHorizon; ++iT )
		{
			// Get mean.
			double sum = 0.;
			for( map<string, SigH>::iterator it = mSigH_.begin(); it != mSigH_.end(); ++it )
			{
				if( it->second.sIH[k].gptOK == 1 )
					sum += it->second.sIH[k].target[iT];
			}
			double mean = sum / hedgeN;
			vMean.push_back(mean);
		}

		// Hedge target60Intra.
		{
			// Get mean.
			double sum = 0.;
			for( map<string, SigH>::iterator it = mSigH_.begin(); it != mSigH_.end(); ++it )
			{
				if( it->second.sIH[k].gptOK == 1 )
					sum += it->second.sIH[k].target60Intra;
			}
			double mean = sum / hedgeN;
			vMean.push_back(mean);
		}

		// Hedge targetClose.
		{
			// Get mean.
			double sum = 0.;
			for( map<string, SigH>::iterator it = mSigH_.begin(); it != mSigH_.end(); ++it )
			{
				if( it->second.sIH[k].gptOK == 1 )
					sum += it->second.sIH[k].targetClose;
			}
			double mean = sum / hedgeN;
			vMean.push_back(mean);
		}

		// Hedge targetNextClose.
		{
			// Get mean.
			double sum = 0.;
			for( map<string, SigH>::iterator it = mSigH_.begin(); it != mSigH_.end(); ++it )
			{
				if( it->second.sIH[k].gptOK == 1 )
					sum += it->second.sIH[k].targetNextClose;
			}
			double mean = sum / hedgeN;
			vMean.push_back(mean);
		}
	}

	// Write Hedge.
	string path = get_hedge_path(HEnv::Instance()->model(), idate_);
	ofstream ofs(path.c_str());

	ofs << vMean.size() << "\t";
	ofs << nonLinMod.nHorizon << "\t";
	for( vector<double>::iterator it = vMean.begin(); it != vMean.end(); ++it )
	{
		char buf[1000];
		sprintf(buf, "%.10f\t", *it);
		ofs << buf;
	}

	cout << "hedge written at " << path << endl;
}
