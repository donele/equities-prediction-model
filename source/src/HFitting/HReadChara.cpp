#include <HFitting/HReadChara.h>
#include <HLib.h>
#include <map>
#include <string>
#include "TFile.h"
using namespace std;

HReadChara::HReadChara(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
debug_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

HReadChara::~HReadChara()
{}

void HReadChara::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	HEvent::Instance()->add<hff::CharaContainer>("", "CharaContainer", &chara_);

	set_uid_list();
}

void HReadChara::beginMarket()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	int idate_p = prevClose(market, idate);
	int idate_pp = prevClose(market, idate_p);
	int idate_n = nextClose(market, idate);

	// Read.
	if( chara_.m[market].find(idate_pp) == chara_.m[market].end() )
		read_chara(market, idate_pp);
	if( chara_.m[market].find(idate_p) == chara_.m[market].end() )
		read_chara(market, idate_p);
	if( chara_.m[market].find(idate) == chara_.m[market].end() )
		read_chara(market, idate);
	read_chara(market, idate_n);

	// Delete old data.
	map<int, map<string, hff::CharaInfo> >& m = chara_.m[market];
	for( map<int, map<string, hff::CharaInfo> >::iterator it = m.begin(); it != m.end(); )
	{
		if( it->first < idate_pp )
			m.erase(it++);
		else
			++it;
	}
}

void HReadChara::endMarket()
{
}

void HReadChara::endJob()
{
}

void HReadChara::set_uid_list()
{
	int nDates = HEnv::Instance()->nDates();
	const vector<int>& idates = HEnv::Instance()->idates();
	int idateFrom = idates[0];
	int idateTo = idates[nDates - 1];

	uids_.clear();
	const vector<string>& markets = HEnv::Instance()->markets();
	for( vector<string>::const_iterator it = markets.begin(); it != markets.end(); ++it )
	{
		string market = *it;

		char cmd[1000];
		sprintf( cmd, "select distinct uniqueID from stockcharacteristics "
			" where market = '%s' and idate >= %d and idate <= %d and uniqueID is not null ",
			mto::code(market).c_str(),
			idateFrom,
			idateTo );
		vector<vector<string> > vv;
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);

		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string uid = trim((*it)[0]);
			if( !uid.empty() )
				uids_.insert(uid);
		}
	}
	HEvent::Instance()->add<set<string> >("", "allUids", uids_);
}

void HReadChara::read_chara(string market, int idate)
{
	chara_.m[market][idate] = map<string, hff::CharaInfo>();
	map<string, hff::CharaInfo>& charaMap = chara_.m[market][idate];

	char cmd[2000];
	sprintf(cmd, "select uniqueID, medVolume, medVolatility, medmedSprd, volatility, "
		" close_, open_, high, low, nTrades, nQuotes, volume, adjust, tickValid, inUniverse "
		" from stockcharacteristics "
		" where %s and uniqueID is not null ",
		mto::selChara(market, idate).c_str());
	vector<vector<string> > vv;
	GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);

	// multithread
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		hff::CharaInfo chara;
		string uid = trim((*it)[0]);
		if( uids_.count(uid) )
		{
			chara.medVolume = atof((*it)[1].c_str());
			chara.medVolatility = atof((*it)[2].c_str());
			chara.medmedSprd = atof((*it)[3].c_str());
			chara.volatility = atof((*it)[4].c_str());
			chara.close = atof((*it)[5].c_str());
			chara.open = atof((*it)[6].c_str());
			chara.high = atof((*it)[7].c_str());
			chara.low = atof((*it)[8].c_str());
			chara.nTrades = atoi((*it)[9].c_str());
			chara.nQuotes = atoi((*it)[10].c_str());
			chara.volume = atoi((*it)[11].c_str());
			chara.adjust = atof((*it)[12].c_str());
			chara.tickValid = atoi((*it)[13].c_str());
			chara.inUniv = atoi((*it)[14].c_str());

			charaMap[uid] = chara;
		}
	}
}
