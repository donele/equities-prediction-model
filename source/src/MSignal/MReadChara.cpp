#include <MSignal/MReadChara.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/GTH.h>
#include <gtlib_signal/sigFtns.h>
#include <MFramework.h>
#include <map>
#include <string>
using namespace std;
using namespace gtlib;

MReadChara::MReadChara(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false)
	//readHfuniverse_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	//if( conf.count("readHfuniverse") )
		//readHfuniverse_ = conf.find("readHfuniverse")->second == "true";
}

MReadChara::~MReadChara()
{}

void MReadChara::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	MEvent::Instance()->add<hff::CharaContainer>("", "CharaContainer", &chara_); // "&" is to add the pointer only.
}

void MReadChara::beginDay()
{
	int idate = MEnv::Instance()->idate;
	uids_ = MEvent::Instance()->get<set<string> >("", "allUids");
	vector<string> markets = MEnv::Instance()->markets;
	mTickerUid_ = get_uid_map(markets, idate, uids_);
	cout << moduleName_ << "::beginDay()" << endl;
	for( auto market : markets )
	{
		//string market = *it;
		int idate = MEnv::Instance()->idate;
		int idate_p = prevClose(market, idate);
		int idate_p2 = prevClose(market, idate_p);
		int idate_p3 = prevClose(market, idate_p2);
		int idate_p4 = prevClose(market, idate_p3);
		int idate_p5 = prevClose(market, idate_p4);
		int idate_p6 = prevClose(market, idate_p5);
		int idate_p7 = prevClose(market, idate_p6);
		int idate_p8 = prevClose(market, idate_p7);
		int idate_n = nextClose(market, idate);

		// Read.
		read_chara(market, idate_p8);
		read_chara(market, idate_p7);
		read_chara(market, idate_p6);
		read_chara(market, idate_p5);
		read_chara(market, idate_p4);
		read_chara(market, idate_p3);
		read_chara(market, idate_p2);
		read_chara(market, idate_p);
		read_chara(market, idate);
		read_chara(market, idate_n);

		// Delete old data.
		chara_.deleteOlder(market, idate_p8);
	}
}

void MReadChara::endDay()
{
}

void MReadChara::endJob()
{
}

void MReadChara::read_chara(const string& market, int idate)
{
	// If read already, return.
	if( chara_.exists(market, idate) )
		return;

	auto& mUidChara = chara_.createAndReturn(market, idate);

	char cmd[2000];
	if( market == "US" || market == "CJ" )
	{
		sprintf(cmd, "select uniqueID, %s, medVolume, medVolatility, medmedSprd, medNquotes, medNtrades, volatility, "
			" close_, open_, high, low, nTrades, nQuotes, volume, adjust, inUniverse, shareout, "
			" market, sectype "
			" from stockcharacteristics "
			" where %s and uniqueID is not null ",
			mto::ts(market).c_str(),
			mto::selChara(market, idate).c_str());
	}
	else
	{
		sprintf(cmd, "select uniqueID, %s, medVolume, medVolatility, medmedSprd, medNquotes, medNtrades, volatility, "
			" close_, open_, high, low, nTrades, nQuotes, volume, adjust, inUniverse, shareout, "
			" tickValid "
			" from stockcharacteristics "
			" where %s and uniqueID is not null ",
			mto::ts(market).c_str(),
			mto::selChara(market, idate).c_str());
	}
	vector<vector<string> > vv;
	GODBC::Instance()->read(mto::hf(market), cmd, vv);

	int univSize = 0;
	auto& uids = MEvent::Instance()->get<set<string> >("", "allUids");
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		hff::CharaInfo chara;
		string uid = trim((*it)[0]);
		if( uids.count(uid) )
		{
			chara.ticker = trim((*it)[1]);
			chara.medVolume = atof((*it)[2].c_str()); // prod: hfuniverse.volume
			chara.medVolatility = atof((*it)[3].c_str()); // prod: hfuniverse.medvolume
			chara.medmedSprd = atof((*it)[4].c_str()); // prod: hfuniverse.medsprd
			chara.medNquotes = atof((*it)[5].c_str());
			chara.medNtrades = atof((*it)[6].c_str());
			chara.volatility = atof((*it)[7].c_str());
			chara.close = atof((*it)[8].c_str()); // prod: hfuniverse.close
			chara.open = atof((*it)[9].c_str());
			chara.high = atof((*it)[10].c_str());
			chara.low = atof((*it)[11].c_str());
			chara.nTrades = atoi((*it)[12].c_str());
			chara.nQuotes = atoi((*it)[13].c_str());
			chara.volume = atoi((*it)[14].c_str());
			chara.adjust = atof((*it)[15].c_str());
			chara.inUniv = atoi((*it)[16].c_str());
			if(chara.inUniv > 0)
				++univSize;
			chara.shareout = atof((*it)[17].c_str());
			if( market == "US" || market == "CJ" )
			{
				chara.market = trim((*it)[18]);
				chara.sectype = trim((*it)[19]);
			}
			else if( market != "US" && market != "CJ" )
				chara.tickValid = atoi((*it)[18].c_str());

			mUidChara[uid] = chara;
		}
	}

	{
		char cmd[2000];
		sprintf(cmd, "select symbol, maxposition from hforderparams where %s and idate = %d ",
				mto::selOrder(market, idate).c_str(), idate);
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hf(market), cmd, vv);
		if(vv.size() > .5 * univSize)
		{
			for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			{
				int maxposition = atoi((*it)[1].c_str());
				if(maxposition > 0)
				{
					string ticker = mto::compTicker(trim((*it)[0]), market);
					string uid = mTickerUid_[ticker];
					auto itch = mUidChara.find(uid);
					if( itch != mUidChara.end() )
					{
						auto& chara = itch->second;
						chara.positiveMaxPos = 1;
					}
				}
			}
		}
		else
		{
			for(auto& kv : mUidChara)
				kv.second.positiveMaxPos = 1;
		}
	}

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	if(linMod.medFromHfuniv || linMod.nfFromHfuniv || linMod.inUnivFromHfuniv )
	{
		int idate_n = nextClose(market, idate);
		char cmd[2000];
		if(mto::isInternational(market))
		{
			sprintf(cmd, "select ch.uniqueid, u.symbol, u.volume, u.volatility, u.medsprd, u.nffundrst, u.nffundtrd, u.nffundbp, u.inuniverse"
					" from hfuniverse u inner join stockcharacteristics ch"
					" on u.idate = %d and ch.idate = %d and u.symbol = ch.%s and u.exchange = ch.market",
					idate_n, idate_n, mto::ts(market).c_str());
		}
		else
		{
			sprintf(cmd, "select ch.uniqueid, u.symbol, u.volume, u.volatility, u.medsprd, u.nffundrst, u.nffundtrd, u.nffundbp, u.inuniverse"
					" from hfuniverse u inner join stockcharacteristics ch"
					" on u.idate = %d and ch.idate = %d and u.symbol = ch.%s",
					idate_n, idate_n, mto::ts(market).c_str());
		}

		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hf(market), cmd, vv);

		// Set inuniv to zero, then read from hfuniv
		if(linMod.inUnivFromHfuniv)
		{
			for(auto& kv : mUidChara)
				kv.second.inUniv = 0;
		}

		// Read columns from hfuniv
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string uid = trim((*it)[0]);
			auto itch = mUidChara.find(uid);
			if( itch != mUidChara.end() )
			{
				auto& chara = itch->second;

				if(linMod.inUnivFromHfuniv)
				{
					if(atoi((*it)[8].c_str()) > 0)
						chara.inUniv = 1;
				}
				if(linMod.medFromHfuniv)
				{
					chara.medVolume = atof((*it)[2].c_str());
					chara.medVolatility = atof((*it)[3].c_str())/basis_pts_;
					chara.medmedSprd = atof((*it)[4].c_str());
				}
				if(linMod.nfFromHfuniv)
				{
					chara.nffundrst = atof((*it)[5].c_str());
					chara.nffundtrd = atof((*it)[6].c_str());
					chara.nffundbp = atof((*it)[7].c_str());
				}
			}
		}
	}
}
