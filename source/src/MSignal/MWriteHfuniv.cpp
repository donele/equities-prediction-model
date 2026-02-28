#include <MSignal/MWriteHfuniv.h>
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

MWriteHfuniv::MWriteHfuniv(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
dbdate_monday_(false),
freeze_univ_(false),
udate_is_trading_day_(true),
verbose_(0),
efficient_write_(true),
write_weights_(true),
nThreads_(1),
db_offset_(3)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("freezeUniv") )
		freeze_univ_ = conf.find("freezeUniv")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("efficientWrite") )
		efficient_write_ = conf.find("efficientWrite")->second == "true";
	if( conf.count("writeWeights") )
		write_weights_ = conf.find("writeWeights")->second == "true";
	if( conf.count("nThreads") )
		nThreads_ = atoi( conf.find("nThreads")->second.c_str() );
	if( conf.count("dbOffset") )
		db_offset_ = atoi(conf.find("dbOffset")->second.c_str());
}

MWriteHfuniv::~MWriteHfuniv()
{}

void MWriteHfuniv::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	if( MEnv::Instance()->multiThreadTicker )
		nThreads_ = MEnv::Instance()->nMaxThreadTicker;
	SigSet::Instance()->resize(nThreads_);
}

void MWriteHfuniv::beginDay()
{
	int idate = MEnv::Instance()->idate;
	vector<string> markets = MEnv::Instance()->markets;
	const hff::LinearModel linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel nonLinMod = MEnv::Instance()->nonLinearModel;
	string model = MEnv::Instance()->model;

	udate_is_trading_day_ = true;
	if( idate != prevClose(markets[0], nextClose(markets[0], idate)) ) // holiday.
		udate_is_trading_day_ = false;

	//if( udate_is_trading_day_ && write_weights_ )
	if( udate_is_trading_day_ && write_weights_ )
	{
		// Do this dayok or not.
		int dbdate = idate;
		for( int i = 0; i < db_offset_; ++i )
			dbdate = nextClose( markets[0], dbdate );
		cout << "Writing for " << dbdate << endl;
		write_begin(idate, dbdate, model, linMod.markets, write_weights_debug_, linMod.gpts);
	}
}

void MWriteHfuniv::beginMarket()
{
	if( udate_is_trading_day_ )
	{
		string model02 = MEnv::Instance()->model.substr(0, 2);
		string market = MEnv::Instance()->market;
		const vector<string>& tickers = MEnv::Instance()->tickers;
		idate_ = MEnv::Instance()->idate;
		idate_p_ = prevClose(market, idate_);

		idate_p_good_ = get_good_idate(model02, market, idate_p_); // Using idate_p_ for compatibility with Jeff's code. Fitting includes idate_'s data, so use the value from idate_p_.
		get_uid_map(market, idate_p_good_);
	}
}

void MWriteHfuniv::beginTicker(const string& ticker)
{
	if( udate_is_trading_day_ )
	{
		string model = MEnv::Instance()->model;
		string market = MEnv::Instance()->market;
		int idate = MEnv::Instance()->idate;
		const hff::LinearModel linMod = MEnv::Instance()->linearModel;

		auto itT = mTickerUid_.find(ticker);
		if( itT != mTickerUid_.end() )
		{
			string uid = itT->second;

			int iSig = -1;
			SigC& sig = SigSet::Instance()->get_sig_object(iSig);

			// Read from stockcharacteristics table.
			if( read_chara_weight(sig, model, market, uid, idate_p_good_) )
			{
				calculate_hedge(sig, uid, ticker, idate);
				if( write_weights_ )
					write_ticker(model, market, uid, baseTicker(ticker), sig);
			}
			SigSet::Instance()->free_sig_object(iSig);
		}
	}
}

void MWriteHfuniv::endTicker(const string& ticker)
{
}

void MWriteHfuniv::endMarket()
{
}

void MWriteHfuniv::endDay()
{
	if( udate_is_trading_day_ && write_weights_ )
	{
		const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
		string model = MEnv::Instance()->model;
		string market = MEnv::Instance()->markets[0];

		// Do this dayok or not.
		write_end(model, market, linMod.mCode[0], freeze_univ_);
	}
}

void MWriteHfuniv::endJob()
{
}

int MWriteHfuniv::get_good_idate(const string& model02, const string& market, int idate)
{
	int ret_default = idate;

	for( int cnt = 0; cnt < 20; ++cnt )
	{
		int nuniv_today = get_nuniv(model02, market, idate);

		vector<int> past5days;
		{
			int idate_prev = prevClose(market, idate);
			for( int i = 0; i < 5; ++i )
			{
				past5days.push_back(idate_prev);
				idate_prev = prevClose(market, idate_prev);
			}
		}

		int max_nuniv = 0;
		for( vector<int>::iterator it = past5days.begin(); it != past5days.end(); ++it )
		{
			int idate_prev = *it;
			int nuniv_prev = get_nuniv(model02, market, idate_prev);
			if( nuniv_prev > max_nuniv )
				max_nuniv = nuniv_prev;
		}
		bool enough_univ = nuniv_today > 0.7 * max_nuniv;

		if( enough_univ )
			return idate;
		else
			idate = prevClose(market, idate);
	}

	return ret_default;
}

int MWriteHfuniv::get_nuniv(const string& model02, const string& market, int idate)
{
	string requirements;
	if( model02 == "UF" )
		requirements = " and sectype = 'F' ";
	else if( model02 == "US" )
		requirements = " and sectype != 'F' ";

	char cmd[1000];
	sprintf( cmd, "select %s, uniqueID from stockcharacteristics "
		" where inuniverse = 1 and %s and uniqueID is not null %s",
		mto::compTicker(market).c_str(),
		mto::selChara(market, idate).c_str(),
		requirements.c_str() );
	vector<vector<string> > vv;
	GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);
	return vv.size();
}

bool MWriteHfuniv::get_uid_map(const string& market, int idate)
{
	mTickerUid_.clear();

	char cmd[1000];
	sprintf(cmd, "select %s, uniqueID from stockcharacteristics "
		" where %s and uniqueID is not null ",
		mto::compTicker(market).c_str(),
		mto::selChara(market, idate).c_str());
	vector<vector<string> > vv;
	GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);

	if( !vv.empty() )
	{
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			string uid = trim((*it)[1]);
			if( !ticker.empty() )
				mTickerUid_[ticker] = uid;
		}
		return true;
	}
	return false;
}

void MWriteHfuniv::calculate_hedge(SigC& sig, const string& uid, const string& ticker, int idate)
{
	// Hedge object.
	const SigH* psigh = 0;
	const vector<SigH>* pvSigH = static_cast<const vector<SigH>*>(MEvent::Instance()->get("", "vSigH"));
	if( pvSigH != 0 )
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
}

string MWriteHfuniv::select_exchange(const vector<string>& markets)
{
	string ret;

	int msize = markets.size();
	if( msize == 1 )
	{
		ret = (string)" = '" + markets[0][1] + "' ";
	}
	else if( msize > 1 )
	{
		ret = "";
		for( vector<string>::const_iterator it = markets.begin(); it != markets.end(); ++it )
		{
			if( ret.empty() )
				ret = " in (";
			else
				ret += ", ";

			ret += (string)"'" + (*it)[1] + "'";
		}
		ret += ") ";
	}

	return ret;
}

void MWriteHfuniv::write_begin(int idate, int dbdate, const string& model, const vector<string>& markets, bool db_debug, int gpts)
{
	if( !markets.empty() )
	{
		dbname_ = db_debug ? mto::hfdbg(markets[0]) : mto::hf(markets[0]);
		if( db_debug )
			efficient_write_ = true;
	}
	else
		return;

	vSymbolInuniv_.clear();
	dbdate_ = dbdate;
	dbdate_p_ = prevClose(markets[0], dbdate_);
	string model02 = model.substr(0, 2);

	QuoteTime dbtime(dbdate_, 120000, mto::tz(markets[0]));
	if( dbtime.WeekDay() == 1 ) // Monday.
		dbdate_monday_ = true;
	else
		dbdate_monday_ = false;

	// Read portfolio.
	char cmd[1000];
	if( model02 == "US" )
	{
		sprintf(cmd, "select hfp.symbol from "
				" (select distinct(b.symbol) from hfbalanceposition b where b.pos != 0 "
				" union select distinct(h.symbol) from hfliveposition h where h.pos != 0) "
				" hfp join stockcharacteristics sc "
				" on (hfp.symbol = sc.ticker) "
				" where sc.sectype != 'F' and sc.idate = %i ", idate);
	}
	else if( model02 == "UF" )
	{
		sprintf(cmd, "select hfp.symbol from "
				" (select distinct(b.symbol) from hfbalanceposition b where b.pos != 0 "
				" union select distinct(h.symbol) from hfliveposition h where h.pos != 0) "
				" hfp join stockcharacteristics sc "
				" on (hfp.symbol = sc.ticker) "
				" where sc.sectype = 'F' and sc.idate = %i ", idate);
	}
	else if( model02 == "EU" || model02 == "CA" ) // EU and CJ.
	{
		sprintf(cmd, "select distinct(b.symbol) from hfbalanceposition b where b.pos != 0 "
				" union select distinct(h.symbol) from hfliveposition h where h.pos != 0 ");
	}
	else
	{
		string selex = select_exchange(markets); // Asia, South Africa, and South America.
		sprintf(cmd, "select distinct(b.symbol) from hfbalanceposition b where b.pos != 0 and b.exchange %s "
				"union select distinct(h.symbol) from hfliveposition h where h.pos != 0 and h.exchange %s ",
				selex.c_str(), selex.c_str());
	}

	vector<vector<string> > vvPort;
	GODBC::Instance()->read(mto::hf(markets[0]), cmd, vvPort);
	portSyms_.clear();
	for( vector<vector<string> >::iterator it = vvPort.begin(); it != vvPort.end(); ++it )
	{
		string ticker = trim((*it)[0]);
		if( !ticker.empty() )
			portSyms_.insert(ticker);
	}

	// Delete.
	char chk3[1000];
	char cmd3[1000];
	if( model02 == "US" || model02 == "UF" ) // Two jobs (US & UF) write on the same db. Do not delete.
	{
		string symbol_list;
		string selsec;
		if( model02 == "US" )
			selsec = " secType != 'F' ";
		else if( model02 == "UF" )
			selsec = " secType = 'F' ";

		// Do nothing.
		if( 0 ) // test
		{
			sprintf(chk3, " select count(*) from hfUniverse where idate = %d and %s ", dbdate_, selsec.c_str());
			sprintf(cmd3, " delete from hfUniverse where idate = %d and %s ", dbdate_, selsec.c_str());
			check_and_delete(dbname_, chk3, cmd3);
		}
	}
	else if( model02 == "CA" )
	{
		sprintf(chk3, " select count(*) from hfUniverse where idate = %d ", dbdate_);
		sprintf(cmd3, " delete from hfUniverse where idate = %d ", dbdate_);
		check_and_delete(dbname_, chk3, cmd3);
	}
	else
	{
		string selex = select_exchange(markets);
		sprintf(chk3, " select count(*) from hfUniverse where idate = %d and exchange %s ", dbdate_, selex.c_str());
		sprintf(cmd3, " delete from hfUniverse where idate = %d and exchange %s ", dbdate_, selex.c_str());
		check_and_delete(dbname_, chk3, cmd3);
	}

	qa_.clear();
}

void MWriteHfuniv::write_ticker(const string& model, const string& market, const string& uid, const string& ticker, hff::SigC& sig)
{
	string model02 = model.substr(0, 2);

	// Define exchange.
	string exchange;
	if( model02 == "US" || model02 == "UF" || model02 == "CA" )
		exchange = sig.exchangeChar;
	else
		exchange = market.substr(1, 1);
	string compSymbol = (exchange.empty()) ? ticker : (exchange + ":" + ticker);

	// check variables.
	if( sig.avgDlyVolume <= 1 || sig.adjPrevClose <= hff::min_price_ || sig.avgDlyVolat <= 0.00001 || sig.medSprd <= .00001 ) // Should write zeros?
		return;
	if( ticker.empty() )
		return;

	// inUnivDB.
	int inUnivDB = -1;
	if( sig.inUnivChara == 1 )
		inUnivDB = 1;
	else
	{
		if( portSyms_.count(ticker) )
			inUnivDB = 0;
		else
			inUnivDB = -1;
	}

	// Collect symbols.
	if( inUnivDB > 0 )
	{
		vSymbolInuniv_.push_back(compSymbol);
	}

	// Write hfuniverse.
	// exhcnage = 'N', 'Q', etc. in US.
	// 'T', 'H', 'S', 'K', 'Q', etc. in Asia.
	// if and else can be combined if US hfUniv are deleted in beginDay().
	if( model02 == "US" || model02 == "UF" )
	{
		char chk[1000];
		char ins[1000];
		char upt[1000];

		sprintf(chk, " select count(*) from hfUniverse "
				" where idate = %d and symbol = '%s' ",
				dbdate_, ticker.c_str());

		sprintf(ins, " insert into hfUniverse (idate, symbol, inUniverse, exchange, "
				" volatility, volume, close_, medsprd, nFFundRST, nFFundTRD, nFFundBP, secType) "
				" VALUES (%d, '%s', %d, '%s', %f, %f, %f, %f, %f, %f, %f, '%s')",
				dbdate_, ticker.c_str(), inUnivDB, exchange.c_str(),
				sig.avgDlyVolat, sig.avgDlyVolume, sig.adjPrevClose, sig.medSprd, sig.northRST, sig.northTRD, sig.northBP, sig.sectype.c_str());

		sprintf(upt, " update hfUniverse set "
				" inUniverse = %d, exchange = '%s', volatility = %f, volume = %f, close_ = %f, "
				" medsprd = %f, nFFundRST = %f, nFFundTRD = %f, nFFundBP = %f, secType = '%s' "
				" where idate = %d and symbol = '%s' ",
				inUnivDB, exchange.c_str(), sig.avgDlyVolat, sig.avgDlyVolume, sig.adjPrevClose,
				sig.medSprd, sig.northRST, sig.northTRD, sig.northBP, sig.sectype.c_str(),
				dbdate_, ticker.c_str()
			   );

		check_and_insert_or_update(dbname_, chk, ins, upt);
	}
	else
	{
		char cmd[1000];
		sprintf(cmd, " insert into hfUniverse (idate, symbol, inUniverse, exchange, "
				" volatility, volume, close_, medsprd, nFFundRST, nFFundTRD, nFFundBP, secType) "
				" VALUES (%d, '%s', %d, '%s', %f, %f, %f, %f, %f, %f, %f, '%s')",
				dbdate_, ticker.c_str(), inUnivDB, exchange.c_str(),
				sig.avgDlyVolat, sig.avgDlyVolume, sig.adjPrevClose, sig.medSprd, sig.northRST, sig.northTRD, sig.northBP, sig.sectype.c_str());
		if( efficient_write_ )
			qa_.add(cmd);
		else
			GODBC::Instance()->get(dbname_)->ExecDirect(cmd);
	}
}

void MWriteHfuniv::write_end(const string& model, const string& market, char mCode, bool freeze_univ)
{
	qa_.exec(dbname_);

	string model02 = model.substr(0, 2);







	// Max from last 5 days.
	int n5daymax = 0;
	{
		int pdate = dbdate_;
		for( int i = 0; i < 5; ++i )
			pdate = prevClose(market, pdate);

		char cmd[1000];
		if( model02 == "US" )
			sprintf(cmd, " select idate, Count(symbol) from hfuniverse "
					" where idate >= %d and idate < %d and sectype != 'F' group by idate ",
					pdate, dbdate_);
		else if( model02 == "UF" )
			sprintf(cmd, " select idate, Count(symbol) from hfuniverse "
					" where idate >= %d and idate < %d and sectype = 'F' group by idate ",
					pdate, dbdate_);
		else if( model02 == "CA" )
			sprintf(cmd, " select idate, Count(symbol) from stockParams1bySymbol "
					" where idate >= %d and idate < %d group by idate ",
					pdate, dbdate_);
		else if( model02 == "KR" ) // stockParams1bySymbol.exchange can be 'K' or 'Q', etc.
			sprintf(cmd, " select idate, Count(symbol) from stockParams1bySymbol "
					" where idate >= %d and idate < %d and (exchange = 'K' or exchange = 'Q') group by idate ",
					pdate, dbdate_);
		else if( model02 == "EU" )
			sprintf(cmd, " select idate, Count(symbol) from stockParams1bySymbol "
					" where idate >= %d and idate < %d "
					" and exchange in ('A','B','F','I','P','Z','X','C','W','Y','M','D','O','L') group by idate ",
					pdate, dbdate_);
		else
			sprintf(cmd, " select idate, Count(symbol) from stockParams1bySymbol "
					" where idate >= %d and idate < %d and exchange = '%c' group by idate ",
					pdate, dbdate_, mCode);

		vector<vector<string> > vv;
		GODBC::Instance()->get(dbname_)->ReadTable(cmd, &vv);
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			int n = atoi((*it)[1].c_str());
			if( n > n5daymax )
				n5daymax = n;
		}
	}

	// Calculate paramsOK.
	int paramsOK = 0;
	int nrows = 0;
	{
		char cmd1[400];
		if( model02 == "US" )
			sprintf(cmd1, " select Count(symbol) from hfuniverse where idate = %d and sectype != 'F' ", dbdate_);
		else if( model02 == "UF" )
			sprintf(cmd1, " select Count(symbol) from hfuniverse where idate = %d and sectype = 'F' ", dbdate_);
		else if( model02 == "CA" )
			sprintf(cmd1, " select Count(symbol) from stockParams1bySymbol where idate = %d ", dbdate_);
		else if( model02 == "KR" ) // stockParams1bySymbol.exchange can be 'K' or 'Q', etc.
			sprintf(cmd1, " select Count(symbol) from stockParams1bySymbol where idate = %d and (exchange = 'K' or exchange = 'Q') ", dbdate_);
		else if( model02 == "EU" )
			sprintf(cmd1, " select Count(symbol) from stockParams1bySymbol where idate = %d "
					" and exchange in ('A','B','F','I','P','Z','X','C','W','Y','M','D','O','L') ",
					dbdate_);
		else
			sprintf(cmd1, " select Count(symbol) from stockParams1bySymbol where idate = %d and exchange = '%c' ", dbdate_, mCode);

		vector<vector<string> > vv;
		GODBC::Instance()->get(dbname_)->ReadTable(cmd1, &vv);
		if( vv.size() == 1 )
			nrows = atoi(vv[0][0].c_str());
		if( nrows > 0.7 * n5daymax )
			paramsOK = 1;
	}

	// Write stockParamsOK.
	char chk2[400];
	char cmd2[400];
	char cmd3[800];
	if( model02 == "CA" )
	{
		sprintf(chk2, " select count(*) from stockParamsOK where idate = %d ", dbdate_);
		sprintf(cmd2, " delete from stockParamsOK where idate = %d ", dbdate_);
		sprintf(cmd3,"insert into stockParamsOK (idate, expectedRows, actualRows, stockParams10OK, stockParams10NDOK, paramsGoodToGo) "
				"VALUES (%d, %d, %d, %d, %d ,%d)",
				dbdate_, nrows, nrows, 1, 1, paramsOK);
	}
	else
	{
		// In US, stockParamsOK.exchagne = 'U' or 'F'.
		// stockParamsOK.exchange = 'T', 'H', 'S', 'K', etc. but not 'Q'.
		sprintf(chk2, " select count(*) from stockParamsOK where idate = %d and exchange = '%c' ", dbdate_, mCode);
		sprintf(cmd2, " delete from stockParamsOK where idate = %d and exchange = '%c' ", dbdate_, mCode);
		sprintf(cmd3,"insert into stockParamsOK (idate, expectedRows, actualRows, stockParams10OK, stockParams10NDOK ,paramsGoodToGo, exchange) "
				"VALUES (%d, %d, %d, %d, %d, %d, '%c')",
				dbdate_, nrows, nrows, 1, 1, paramsOK, mCode);
	}
	check_and_delete(dbname_, chk2, cmd2);
	GODBC::Instance()->exec(dbname_, cmd3);







	// Write random inUniverse field.
	char cmd[200];
	int nSymbol = vSymbolInuniv_.size();
	vector<double> randomNumbers(nSymbol);

	// Generate the random numbers.
	for( int i = 0; i < nSymbol; ++i )
		randomNumbers[i] = Random::Instance()->next();

	// Sort the random numbers.
	vector<int> vIndx(nSymbol);
	if( nSymbol > 0 )
		gsl_rank<int, double>(vIndx, randomNumbers);

	// inUniverse field from the previous day.
	map<string, int> mCompsymUniv;
	if( freeze_univ && !dbdate_monday_ )
	{
		sprintf(cmd, " select exchange + ':' + symbol, inUniverse from hfUniverse "
				" where idate = %d ",
				dbdate_p_);

		vector<vector<string> > vv;
		GODBC::Instance()->read(dbname_, cmd, vv);
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			if( it->size() == 2 )
			{
				string symbol = (*it)[0];
				int univ = atoi((*it)[1].c_str());
				mCompsymUniv[symbol] = univ;
			}
		}
	}

	// Write to db.
	QueryAggregator qa_univ;
	for( int i = 0; i < nSymbol; ++i )
	{
		int iUniv = vIndx[i] % 16 + 1;
		string compSymbol = vSymbolInuniv_[i];
		string exchange = compSymbol.substr(0, 1);
		string ticker = base_name(compSymbol);

		if( freeze_univ && !dbdate_monday_ )
		{
			if( iUniv > 0 )
			{
				map<string, int>::iterator itu = mCompsymUniv.find(compSymbol);
				if( itu != mCompsymUniv.end() && itu->second > 0 )
					iUniv = itu->second;
			}
		}

		sprintf(cmd, " update hfUniverse set inUniverse = %d "
				" where idate = %d and symbol = '%s' and exchange = '%s' ",
				iUniv, dbdate_, ticker.c_str(), exchange.c_str());
		if( efficient_write_ )
			qa_univ.add(cmd);
		else
			GODBC::Instance()->exec(dbname_, cmd);
	}
	if( efficient_write_ )
		qa_univ.exec(dbname_);
}

