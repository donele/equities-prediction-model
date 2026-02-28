#include <MFramework/MSelectTickers.h>
#include <MFramework/MEnv.h>
#include <jl_lib.h>
#include <gtlib/util.h>
#include <vector>
#include <string>
#include <map>
#include <unordered_set>
using namespace std;
using namespace gtlib;

MSelectTickers::MSelectTickers(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	verbose_(0),
	update_("job"), // "day", "marketday"
	source_("stockcharacteristics"), // "hforders", "hfuniverse"
	inUniv_(true),
	maxNticker_(0)
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("maxNticker") )
		maxNticker_ = atoi( conf.find("maxNticker")->second.c_str() );

	if( conf.count("source") )
		source_ = conf.find("source")->second;
	if( conf.count("update") )
		update_ = conf.find("update")->second;
	if( conf.count("sectype") )
		sectype_ = getConfigValuesString(conf, "sectype");

	// Tickers debug.
	vector<string> vticker;
	if( conf.count("ticker") )
	{
		pair<mmit, mmit> tickers = conf.equal_range("ticker");
		for( mmit mi = tickers.first; mi != tickers.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 1 )
				vticker.push_back(vs[0]);
		}
		MEnv::Instance()->tickersDebug = vticker;
	}
}

MSelectTickers::~MSelectTickers()
{
}

void MSelectTickers::beginJob()
{
	if(update_ == "job")
	{
		if(!MEnv::Instance()->tickersDebug.empty())
			MEnv::Instance()->tickers = MEnv::Instance()->tickersDebug;
		else if(source_ == "stockcharacteristics" && inUniv_ && sectype_.empty())
		{
			set_ticker_list();
			set_uid_list();
		}
	}

	return;
}

void MSelectTickers::beginMarket()
{
	string market = MEnv::Instance()->market;
	vector<string> tickers;
	if( marketTickers_.count(market) )
		MEnv::Instance()->tickers = marketTickers_[market];

	if( verbose_ > 2 )
	{
		int nT = tickers.size();
		printf("MSelectTickers::beginMarket() %s", market.c_str());
		if( nT > 0 )
		{
			printf(", %d tickers:", nT);
			int n = (nT > 3)? 3: nT;
			for( int i = 0; i < n; ++i )
				printf(" %s", tickers[i].c_str());
			if( nT > 3 )
				printf(", ...");
		}
		cout << endl;
	}
}

void MSelectTickers::beginMarketDay()
{
	if(update_ == "marketday")
	{
		if(!MEnv::Instance()->tickersDebug.empty())
			MEnv::Instance()->tickers = MEnv::Instance()->tickersDebug;
		else if(source_ == "hforders")
		{
			set_ticker_list_orders();
		}
	}
}

void MSelectTickers::endMarket()
{
}

void MSelectTickers::endJob()
{
	MEvent::Instance()->remove<map<string, vector<string> > >("", "marketTickers");
}

void MSelectTickers::set_ticker_list()
{
	const vector<int>& idates = MEnv::Instance()->idates;
	int nDates = idates.size();
	string model2 = MEnv::Instance()->model.substr(0, 2);
	const vector<string>& markets = MEnv::Instance()->markets;

	vector<string> tickers;
	if( !idates.empty() )
	{
		char cmd[1000];
		if( "US" == model2 || "CA" == model2 )
		{
			string market = markets[0];
			int idateFrom = get_idate_from(market, idates[0]);
			int idateTo = get_idate_to(market, idates[nDates - 1]);

			sprintf( cmd, "select distinct %s from stockcharacteristics "
				" where idate >= %d and idate <= %d "
				" %s "
				" and sectype >= 'A' and sectype <= 'Z' and sectype != 'P' and sectype != 'F' and sectype != 'X' "
				" and uniqueID is not null ",
				mto::compTicker(market).c_str(),
				idateFrom,
				idateTo,
				sel_univ().c_str() );
			vector<vector<string> > vv;
			GODBC::Instance()->read(mto::hf(market), cmd, vv);

			for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			{
				string ticker = trim((*it)[0]);
				if( !ticker.empty() )
					tickers.push_back(ticker);
			}

			tickers = comp_ticker(tickers, market);
			sort(tickers.begin(), tickers.end());
			marketTickers_[market] = tickers;
		}
		else if( "UF" == model2 )
		{
			string market = markets[0];
			int idateFrom = get_idate_from(market, idates[0]);
			int idateTo = get_idate_to(market, idates[nDates - 1]);

			sprintf( cmd, "select distinct %s from stockcharacteristics "
				" where idate >= %d and idate <= %d "
				" %s "
				" and sectype >= 'A' and sectype <= 'Z' and sectype = 'F' "
				" and uniqueID is not null ",
				mto::compTicker(market).c_str(),
				idateFrom,
				idateTo,
				sel_univ().c_str());
			vector<vector<string> > vv;
			GODBC::Instance()->read(mto::hf(market), cmd, vv);

			for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			{
				string ticker = trim((*it)[0]);
				if( !ticker.empty() )
					tickers.push_back(ticker);
			}

			tickers = comp_ticker(tickers, market);
			sort(tickers.begin(), tickers.end());
			marketTickers_[market] = tickers;
		}
		else
		{
			for( vector<string>::const_iterator it = markets.begin(); it != markets.end(); ++it )
			{
				string market = *it;
				int idateFrom = get_idate_from(market, idates[0]);
				int idateTo = get_idate_to(market, idates[nDates - 1]);

				sprintf( cmd, "select distinct %s from stockcharacteristics "
					" where market = '%s' and idate >= %d and idate <= %d %s "
					" and uniqueID is not null ",
					mto::compTicker(market).c_str(),
					mto::code(market).c_str(),
					idateFrom,
					idateTo,
					sel_univ().c_str() );
				vector<vector<string> > vv;
				GODBC::Instance()->read(mto::hf(market), cmd, vv);

				for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
				{
					string ticker = trim((*it)[0]);
					if( !ticker.empty() )
						tickers.push_back(ticker);
				}

				tickers = comp_ticker(tickers, market);
				sort(tickers.begin(), tickers.end());
				marketTickers_[market] = tickers;
			}
		}
	}
	MEvent::Instance()->add<map<string, vector<string> > >("", "marketTickers", marketTickers_);
}

void MSelectTickers::set_uid_list()
{
	const vector<int>& idates = MEnv::Instance()->idates;
	int nDates = idates.size();
	string model2 = MEnv::Instance()->model.substr(0, 2);
	const vector<string>& markets = MEnv::Instance()->markets;

	set<string> uids;
	if( !idates.empty() )
	{
		char cmd[1000];
		if( "US" == model2 || "CA" == model2 )
		{
			string market = markets[0];
			int idateFrom = get_idate_from(market, idates[0]);
			int idateTo = get_idate_to(market, idates[nDates - 1]);

			sprintf( cmd, "select distinct uniqueID from stockcharacteristics "
				" where idate >= %d and idate <= %d "
				" %s "
				" and sectype >= 'A' and sectype <= 'Z' and sectype != 'P' and sectype != 'F' and sectype != 'X' "
				" and uniqueID is not null ",
				idateFrom,
				idateTo,
				sel_univ().c_str() );

			vector<vector<string> > vv;
			GODBC::Instance()->read(mto::hf(market), cmd, vv);

			for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			{
				string uid = trim((*it)[0]);
				if( !uid.empty() )
					uids.insert(uid);
			}
		}
		else if( "UF" == model2 )
		{
			string market = markets[0];
			int idateFrom = get_idate_from(market, idates[0]);
			int idateTo = get_idate_to(market, idates[nDates - 1]);

			sprintf( cmd, "select distinct uniqueID from stockcharacteristics "
				" where idate >= %d and idate <= %d "
				" %s "
				" and sectype >= 'A' and sectype <= 'Z' and sectype = 'F' "
				" and uniqueID is not null ",
				idateFrom,
				idateTo,
				sel_univ().c_str() );

			vector<vector<string> > vv;
			GODBC::Instance()->read(mto::hf(market), cmd, vv);

			for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			{
				string uid = trim((*it)[0]);
				if( !uid.empty() )
					uids.insert(uid);
			}
		}
		else
		{
			for( vector<string>::const_iterator it = markets.begin(); it != markets.end(); ++it )
			{
				string market = *it;
				int idateFrom = get_idate_from(market, idates[0]);
				int idateTo = get_idate_to(market, idates[nDates - 1]);

				sprintf( cmd, "select /*distinct*/ uniqueID from stockcharacteristics "
					" where market = '%s' and idate >= %d and idate <= %d %s and uniqueID is not null ",
					mto::code(market).c_str(),
					idateFrom,
					idateTo,
					sel_univ().c_str() );

				vector<vector<string> > vv;
				GODBC::Instance()->read(mto::hf(market), cmd, vv);

				for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
				{
					string uid = trim((*it)[0]);
					if( !uid.empty() )
						uids.insert(uid);
				}
			}
		}
	}
	MEvent::Instance()->add<set<string> >("", "allUids", uids);
}

string MSelectTickers::sel_univ()
{
	string ret = "";
	string model2 = MEnv::Instance()->model.substr(0, 2);
	if( "US" == model2 || "CA" == model2 )
		ret = " and (inuniverse = 1 or (close_ > .5 and close_ * medvolume > 60000 and medVolatility > .005 and close_ < 900 and medmedsprd >= .00008 and medmedsprd < .04))";
	else
		ret = " and inuniverse = 1 ";
	return ret;
}

int MSelectTickers::get_idate_from(const string& market, int idate)
{
	for( int i = 0; i < 20; ++i )
		idate = prevClose(market, idate);
	idate = prevClose(market, idate);
	return idate;
}

int MSelectTickers::get_idate_to(const string& market, int idate)
{
	idate = prevClose(market, idate);
	return idate;
}

void MSelectTickers::set_ticker_list_orders()
{
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	string sExch = mto::code(market);

	string selMarket = "";
	if( mto::isInternational(market) && !isECN(market) )
		selMarket = " and exchange = '" + sExch + "' ";

	string dbSymbol = "symbol";
	if( mto::isInternational(market) )
		dbSymbol = "exchange + ':' + symbol";

	string cmd = (string)" select distinct (" + dbSymbol + ") "
		+ " from hfOrders "
		+ " where idate = " + itos(idate)
		+ selMarket;
	vector<vector<string> > vvs;
	GODBC::Instance()->read(mto::hfo(market, idate), cmd, vvs);

	vector<string> tickers;
	int cnt = 0;
	for( vector<vector<string> >::iterator it = vvs.begin(); it != vvs.end(); ++it )
	{
		string symbol = trim((*it)[0]);
		if( !symbol.empty() )
		{
			tickers.push_back(symbol);
			if( maxNticker_ > 0 && ++cnt >= maxNticker_ )
				break;
		}
	}
	sort(tickers.begin(), tickers.end());

	if(!sectype_.empty())
		tickers = select_sectype(tickers);
	MEnv::Instance()->tickers = tickers;
}

vector<string> MSelectTickers::select_sectype(const vector<string>& tickers)
{
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	string sExch = mto::code(market);

	string selMarket = "";
	if( mto::isInternational(market) && !isECN(market) )
		selMarket = " and exchange = '" + sExch + "' ";

	string dbSymbol = mto::ts(market);
	if( mto::isInternational(market) )
		dbSymbol = "exchange + ':' + symbol";

	string selSectype = " and sectype in (";
	for(string sectype : sectype_)
		selSectype += "'" + sectype + "',";
	selSectype = selSectype.substr(0, selSectype.size() - 1);
	selSectype += ")";

	unordered_set<string> tickers_sectype;
	string cmd = (string)" select " + dbSymbol
		+ " from stockcharacteristics "
		+ " where idate = " + itos(idate)
		+ selMarket
		+ selSectype;
	vector<vector<string> > vvs;
	GODBC::Instance()->read(mto::hf(market), cmd, vvs);
	for( vector<vector<string> >::iterator it = vvs.begin(); it != vvs.end(); ++it )
	{
		string symbol = trim((*it)[0]);
		if( !symbol.empty() )
			tickers_sectype.insert(symbol);
	}

	vector<string> selected;
	for(const string& ticker : tickers)
	{
		if(tickers_sectype.count(ticker))
			selected.push_back(ticker);
	}
	return selected;
}
