#include <jl_lib/TickerSector.h>
#include <jl_lib/mto.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GODBC.h>
using namespace std;

TickerSector::TickerSector(const string& market, int idate)
:idate_(idate)
{
	tickerSector_.clear();
	if( mto::isInternational(market) )
	{
		char cmd[400];
		sprintf( cmd, " select i.code, i.industry from gprcinfo2 i, gprcval v "
			" where i.code = v.code "
			" and v.date_ = (select max(date_) from cprcdly where date_ <= '%s') "
			" and %s ",
			itoq(idate).c_str(),
			mto::selVal(market).c_str() );
		vector<vector<string> > vv;
		GODBC::Instance()->get("qai")->ReadTable(cmd, &vv);
		map<string, string> codeSector;
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string code = trim((*it)[0]);
			string sector = trim((*it)[1]);
			if( !sector.empty() && sector != "0" && sector != "0000" )
				codeSector[code] = sector;
		}

		char cmdChara[400];
		sprintf( cmdChara, "select symbol, uniqueid from stockcharacteristics "
			" where idate = (select max(idate) from stockcharacteristics where idate <= %d) "
			" and market = '%s' "
			" and uniqueid is not null ",
			idate,
			mto::code(market).c_str() );
		vector<vector<string> > vvChara;
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmdChara, &vvChara);

		for( vector<vector<string> >::iterator it = vvChara.begin(); it != vvChara.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			string uid = trim((*it)[1]);
			string code = "";
			int uidSize = uid.size();
			if( uidSize > 2 )
				code = uid.substr(2, uidSize - 2);

			map<string, string>::iterator itm = codeSector.find(code);
			if( itm != codeSector.end() )
			{
				string sector = itm->second;
				tickerSector_[ticker] = sector;
			}
		}
	}
	else if( "C" == mto::region(market) )
	{
		char cmd[400];
		sprintf( cmd, " select i.code, i.sic from cprcinfo i, cprcdly v "
			" where i.code = v.code "
			" and v.date_ = (select max(date_) from cprcdly where date_ <= '%s') ",
			itoq(idate).c_str() );
		vector<vector<string> > vv;
		GODBC::Instance()->get("qai")->ReadTable(cmd, &vv);
		map<string, string> codeSector;
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string code = trim((*it)[0]);
			string sector = trim((*it)[1]);
			if( !sector.empty() && sector != "0" && sector != "0000" )
				codeSector[code] = sector;
		}

		char cmdChara[400];
		sprintf( cmdChara, "select ticker, uniqueid from stockcharacteristics "
			" where idate = (select max(idate) from stockcharacteristics where idate <= %d) "
			" and uniqueid is not null ",
			idate );
		vector<vector<string> > vvChara;
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmdChara, &vvChara);

		for( vector<vector<string> >::iterator it = vvChara.begin(); it != vvChara.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			string uid = trim((*it)[1]);
			string code = "";
			int uidSize = uid.size();
			if( uidSize > 2 )
				code = uid.substr(2, uidSize - 2);

			map<string, string>::iterator itm = codeSector.find(code);
			if( itm != codeSector.end() )
			{
				string sector = itm->second;
				tickerSector_[ticker] = sector;
			}
		}
	}
	else if( "U" == mto::region(market) )
	{
		char cmd[400];
		sprintf( cmd, " select r.id, i.sic from prc.prcinfo i, secmap m, secmstr r, prc.prcdly d "
			" where m.vencode = i.code and r.seccode = m.seccode and d.code = i.code "
			" and r.type_ = 1 and m.ventype = 14 "
			" and d.date_ = (select max(date_) from cprcdly where date_ <= '%s') ",
			itoq(idate).c_str() );
		vector<vector<string> > vv;
		GODBC::Instance()->get("qai")->ReadTable(cmd, &vv);
		map<string, string> uidSector;
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string uid = trim((*it)[0]);
			string sector = trim((*it)[1]);
			if( !sector.empty() && sector != "0" && sector != "0000" )
				uidSector[uid] = sector;
		}

		char cmdChara[400];
		sprintf( cmdChara, "select ticker, uniqueid from stockcharacteristics "
			" where idate = (select max(idate) from stockcharacteristics where idate <= %d) "
			" and uniqueid is not null ",
			idate );
		vector<vector<string> > vvChara;
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmdChara, &vvChara);

		for( vector<vector<string> >::iterator it = vvChara.begin(); it != vvChara.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			string uid = trim((*it)[1]);

			map<string, string>::iterator itm = uidSector.find(uid);
			if( itm != uidSector.end() )
			{
				string sector = itm->second;
				tickerSector_[ticker] = sector;
			}
		}
	}
}

string TickerSector::get_sector(string ticker)
{
	ticker = base_name(ticker);
	string ret = "";
	map<string, string>::iterator it = tickerSector_.find(ticker);
	if( it != tickerSector_.end() )
		ret = it->second;
	return ret;
}

set<string> TickerSector::get_sectors()
{
	set<string> ret;
	for( map<string, string>::iterator it = tickerSector_.begin(); it != tickerSector_.end(); ++it )
		ret.insert(it->second);

	return ret;
}

int TickerSector::get_idate()
{
	return idate_;
}
