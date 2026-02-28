#include <gtlib_linmod/BiLinearModelWeights.h>
#include <gtlib_model/hff.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/Random.h>
#include <jl_lib/sort_util.h>
#include <boost/filesystem.hpp>
using namespace std;
using namespace hff;

BiLinearModelWeights::BiLinearModelWeights()
{
}

BiLinearModelWeights::BiLinearModelWeights(int nHorizon, int num_time_slices, int om_num_sig, int om_num_err_sig)
:dbdate_monday_(false),
	efficient_write_(true),
	use_sigmoid_(false),
	nHorizon_(nHorizon),
	num_time_slices_(num_time_slices),
	om_num_sig_(om_num_sig),
	om_num_err_sig_(om_num_err_sig)
{
}

void BiLinearModelWeights::set_time_slices(int gpts)
{
	// timeSliceStart
	timeSliceStart_.clear();
	int timeFac = ceil( float(gpts - 1) / num_time_slices_ );
	for( int i = 0; i < num_time_slices_; ++i )
		timeSliceStart_.push_back( i * timeFac * 30 );
	int cnt = 0;
	for( vector<int>::iterator it = timeSliceStart_.begin(); it != timeSliceStart_.end(); ++it )
		mItime_[*it] = cnt++;
}

void BiLinearModelWeights::useSigmoid()
{
	use_sigmoid_ = true;
}

void BiLinearModelWeights::useSigmoid(const vector<float>& range, const vector<float>& rangeErr)
{
	use_sigmoid_ = true;
	range_ = range;
	rangeErr_ = rangeErr;
}

void BiLinearModelWeights::read_range(int idate, const string& weight_dir)
{
	ifstream ifs(get_aggregated_range_file(idate, weight_dir).c_str());
	double x;
	vector<float> v;
	while(ifs >> x)
		v.push_back(x);
	if( v.size() == om_num_sig_ )
	{
		range_ = v;
		rangeErr_ = vector<float>(v.begin(), v.begin() + om_num_err_sig_);
	}
}

void BiLinearModelWeights::read_weights(int idate, const string& weight_dir)
{
	weights_.clear();
	weightsErr_.clear();

	weights_ = vector<vector<vector<float> > >(nHorizon_, vector<vector<float> >(num_time_slices_, vector<float>(om_num_sig_)));
	weightsErr_ = vector<map<string, vector<float> > >(nHorizon_, map<string, vector<float> >());

	string filename = get_weight_path(idate, weight_dir);
	ifstream ifs(filename.c_str());

	int nHorizonRead = 0;
	int nTimeSliceRead = 0;
	int lengthRead = 0;
	ifs >> nHorizonRead >> nTimeSliceRead >> lengthRead;

	if( nHorizon_ != nHorizonRead || num_time_slices_ != nTimeSliceRead || (num_time_slices_ > 0 && om_num_sig_ != lengthRead) )
	{
		cerr << "BiLinearModelWeights::read_weights() ERROR\n";
		exit(3);
	}

	for( int ih = 0; ih < nHorizon_; ++ih )
	{
		for( int it = 0; it < num_time_slices_; ++it )
		{
			for( int is = 0; is < om_num_sig_; ++is )
				ifs >> weights_[ih][it][is];
		}
	}

	int nTickerRead = 0;
	int lengthErrRead = 0;
	ifs >> nTickerRead >> lengthErrRead;

	if( om_num_err_sig_ != lengthErrRead )
	{
		cerr << "BiLinearModelWeights::read_weights() ERROR\n";
		exit(3);
	}

	for( int ih = 0; ih < nHorizon_; ++ih )
	{
		for( int it = 0; it < nTickerRead; ++it )
		{
			string uid;
			ifs >> uid;

			vector<float> vw(om_num_err_sig_);
			for( int is = 0; is < om_num_err_sig_; ++is )
				ifs >> vw[is];
			weightsErr_[ih][uid] = vw;
		}
	}
}

void BiLinearModelWeights::read_db_weights(const string& model, const string& market, int idate, char mCode,
		unordered_map<string, string>& mTickerUid, int gpts, const string& dbname)
{
	//string dbname = mto::hfpar(market, idate);
	string model02 = model.substr(0, 2);
	string region = model.substr(0, 1);

	// Universal (except the sprd part) and stock specific models are merged into the table stockParams1bySymbol.
	// The sprd part of universal model is kept separately in stockParams1bySymbolSprd.
	// Since it is impossible to recover the original model, read the content of stockParams1bySymbol into the
	// stock specific model (weightsErr_), and the stockParams1bySymbolSprd into weights_.
	weightsDB_.clear();
	weightsDBsprd_.clear();
	weightsDB_ = vector<map<string, vector<float> > >(num_time_slices_, map<string, vector<float> >());
	weightsDBsprd_ = vector<vector<float> >(num_time_slices_, vector<float>(om_num_basic_sig_, 0.));

	// idate to read from.
	int dbdate = 0;
	{
		char cmd[1000];
		string select_exchange;
		if( model02 == "CA" )
			;
		else
		{
			char exbuf[100];
			sprintf(exbuf, " and exchange = '%c' ", mCode);
			select_exchange = exbuf;
		}
		sprintf(cmd, "select max(idate) from stockparamsok where idate <= %d and paramsGoodToGo > 0 %s", idate, select_exchange.c_str());

		vector<vector<string> > vv;
		GODBC::Instance()->read(dbname, cmd, vv);
		dbdate = atoi(vv[0][0].c_str());
	}

	string col_list;
	if( model02 == "US" || model02 == "UF" )
		col_list = "qutImb, fillImb, ret, hilo, retMkt, retFut, lowcapIdx, lowcapFut, qutImb1, qutImb2, midOff1, midOff2, russIdx, russLowcap, mret300, mret300lag";
	else if( model02 == "CA" )
	{
		if( dbdate < 20081201 )
			col_list = "qutImb, fillImb, ret, hilo, tsxPtsx, sxfPtsx, 0, 0, 0, 0, 0, 0, 0, 0, mret300, mret300lag";
		else
			col_list = "qutImb, fillImb, ret, hilo, tsxPtsx, sxfPtsx, 0, 0, qutImb1, qutImb2, midOff1, midOff2, 0, 0, mret300, mret300lag";
	}
	else
		col_list = "qutImb, fillImb, ret, hilo, euxPeux, 0, 0, 0, qutImb1, qutImb2, midOff1, midOff2, 0, 0, mret300, mret300lag";

	string selExch;
	if( model02 != "US" && model02 != "UF" && model02 != "CA" )
	{
		if( model02 == "KR" )
			selExch = "and (exchange = 'K' or exchange = 'Q')";
		else if( model02 == "EU" )
			selExch = "and exchange in ('A','B','F','I','P','Z','X','C','W','Y','M','D','O','L')";
		else
			selExch = "and exchange = '" + market.substr(1, 1) + "'";
	}

	// Read the 'Sprd' part of the universal model from stockParams1bySymbolSprd.
	// The value of sprd for each stock is not included in the weights.
	mItime_.clear();
	map<int, int>::iterator mItimeEnd;
	vector<vector<string> > vv2;
	{
		// Pick a symbol to read the model from. Result should be independant of the choice of symbol.
		char cmd1[400];
		if( model02 == "US" )
		{
			if( dbdate < 20130701 ) // MSSQL
				sprintf(cmd1, "select top 1 symbol from hfuniverse where idate = %d and sectype != 'F'", dbdate );
			else
				sprintf(cmd1, "select symbol from hfuniverse where idate = %d and sectype != 'F' limit 1", dbdate );
		}
		else if( model02 == "UF" )
		{
			if( dbdate < 20130701 ) // MSSQL
				sprintf(cmd1, "select top 1 symbol from hfuniverse where idate = %d and sectype = 'F'", dbdate );
			else
				sprintf(cmd1, "select symbol from hfuniverse where idate = %d and sectype = 'F' limit 1", dbdate );
		}
		else if( model02 == "CA" )
		{
			if( dbdate < 20081201 )
				sprintf(cmd1, "select top 1 symbol from hfuniverse where idate = %d", dbdate );
			else
				sprintf(cmd1, "select symbol from hfuniverse where idate = %d limit 1", dbdate );
		}
		else if( model02 == "KR" )
			sprintf(cmd1, "select symbol from hfuniverse where idate = %d %s limit 1", dbdate, selExch.c_str() );
		else if( model02 == "EU" )
			sprintf(cmd1, "select symbol from hfuniverse where idate = %d %s limit 1", dbdate, selExch.c_str() );
		else
			sprintf(cmd1, "select symbol from hfuniverse where idate = %d %s limit 1", dbdate, selExch.c_str() );
		vector<vector<string> > vv1;
		GODBC::Instance()->read(dbname, cmd1, vv1);
		string symbol = trim(vv1[0][0]);

		// Read the universal model.
		char cmd2[2000];
		sprintf(cmd2, "select itime, %s "
				" from stockParams1bySymbolSprd "
				" where idate = %d and symbol = '%s' %s",
				col_list.c_str(), dbdate, symbol.c_str(), selExch.c_str());
		GODBC::Instance()->read(dbname, cmd2, vv2);

		set<int> sTimeStart;
		for( vector<vector<string> >::iterator it = vv2.begin(); it != vv2.end(); ++it )
			sTimeStart.insert(atoi((*it)[0].c_str()));
		int cnt = 0;
		for( set<int>::iterator it = sTimeStart.begin(); it != sTimeStart.end(); ++it )
			mItime_[*it] = cnt++;
		mItimeEnd = mItime_.end();

		for( vector<vector<string> >::iterator it = vv2.begin(); it != vv2.end(); ++it )
		{
			int timeStart = atoi((*it)[0].c_str());
			map<int, int>::iterator itt = mItime_.find(timeStart);
			if( itt != mItimeEnd )
			{
				int iTime = itt->second;
				for( int i = 0; i < om_num_basic_sig_; ++i )
					weightsDBsprd_[iTime][i] = atof((*it)[i + 1].c_str());
			}
			else
				exit(7);
		}
	}

	// Read the rest of the linear model from stockParams1bySymbol.
	// This is a combination of the first 3/4 of the universal model and the stock specific model.
	// logvolu and logprice are already included in the weights.
	vector<vector<string> > vv1;
	{
		string exCol = (model02 == "US" || model02 == "UF" || model02 == "CA") ? "'TEST'" : "exchange";
		char cmd1[2000];
		sprintf(cmd1, "select symbol, %s, itime, %s "
				" from stockParams1bySymbol "
				" where idate = %d %s ",
				exCol.c_str(), col_list.c_str(), dbdate, selExch.c_str());
		GODBC::Instance()->read(dbname, cmd1, vv1);
	}

	// Loop the contents of stockParams1bySymbol and copy stock specific model.
	vector<float> temp_weights(om_num_basic_sig_);
	for( vector<vector<string> >::iterator it = vv1.begin(); it != vv1.end(); ++it )
	{
		string ticker = trim((*it)[0]);
		string exchange = trim((*it)[1]);
		string compTicker = (region == "U" || region == "C") ? ticker : exchange + ":" + ticker;
		string uid = mTickerUid[compTicker];

		for( int i = 0; i < om_num_basic_sig_; ++i )
			temp_weights[i] = atof((*it)[i + 3].c_str());

		int timeStart = atoi((*it)[2].c_str());
		map<int, int>::iterator itt = mItime_.find(timeStart);
		if( itt != mItimeEnd )
		{
			int iTime = itt->second;
			if( !uid.empty() )
				weightsDB_[iTime][uid] = temp_weights;
		}
		else
			exit(8);
	}
}

string BiLinearModelWeights::get_aggregated_range_file(int idate, const string& weight_dir)
{
	char filename[200];
	sprintf(filename, "%s/range%d.txt", weight_dir.c_str(), idate);
	return filename;
}

string BiLinearModelWeights::get_weight_path(int idate, const string& weight_dir)
{
	namespace fs = boost::filesystem;
	fs::path dir(weight_dir);

	int weight_date = 0;
	if( fs::exists(dir) )
	{
		fs::directory_iterator end_iter;
		for( fs::directory_iterator itd(dir); itd != end_iter; ++itd )
		{
			string filename = itd->path().filename().string();
			if( filename.size() == 18 )
			{
				int fdate = atoi(filename.substr(6, 8).c_str());
				if( fdate > weight_date && fdate < idate )
					weight_date = fdate;
			}
		}
	}
	else
	{
		cerr << "BiLinearModelWeights::get_weight_path() ERROR: Cannot open " << weight_dir << "\n";
		exit(9);
	}

	char filename[200];
	if( weight_date > 0 )
		sprintf(filename, "%s\\weight%d.txt", weight_dir.c_str(), weight_date);
	return xpf(filename);
}

//
// Write weights.
//

void BiLinearModelWeights::write_weights_beginDay(int idate, int dbdate, const string& model, const vector<string>& markets, bool db_debug, int gpts)
{
	set_time_slices(gpts);

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
		//sprintf(cmd, "select hfp.symbol from "
		//	" (select distinct(b.symbol) as symbol from hfbalanceposition b where b.pos != 0 "
		//	" union select distinct(h.symbol) as symbol from hfliveposition h where h.pos != 0) "
		//	" hfp join stockcharacteristics sc "
		//	" on ( (hfp.symbol collate sql_latin1_general_cp1_ci_as) = sc.ticker ) "
		//	" where sc.sectype != 'F' and sc.idate = %i ", idate);
		sprintf(cmd, "select hfp.symbol from "
				" (select distinct(b.symbol) from hfbalanceposition b where b.pos != 0 "
				" union select distinct(h.symbol) from hfliveposition h where h.pos != 0) "
				" hfp join stockcharacteristics sc "
				" on (hfp.symbol = sc.ticker) "
				" where sc.sectype != 'F' and sc.idate = %i ", idate);
	}
	else if( model02 == "UF" )
	{
		//sprintf(cmd, "select hfp.symbol from "
		//	" (select distinct(b.symbol) as symbol from hfbalanceposition b where b.pos != 0 "
		//	" union select distinct(h.symbol) as symbol from hfliveposition h where h.pos != 0) "
		//	" hfp join stockcharacteristics sc "
		//	" on ( (hfp.symbol collate sql_latin1_general_cp1_ci_as) = sc.ticker ) "
		//	" where sc.sectype = 'F' and sc.idate = %i ", idate);
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
	char chk1[1000];
	char chk2[1000];
	char chk3[1000];
	char cmd1[1000];
	char cmd2[1000];
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
			sprintf(chk1, " select count(*) from stockParams1bySymbol where idate = %d "
					" and symbol in (select symbol from hfuniverse where idate = %d and %s ) ", dbdate_, dbdate_, selsec.c_str());
			sprintf(chk2, " select count(*) from stockParams1bySymbolSprd where idate = %d "
					" and symbol in (select symbol from hfuniverse where idate = %d and %s ) ", dbdate_, dbdate_, selsec.c_str());
			sprintf(chk3, " select count(*) from hfUniverse where idate = %d and %s ", dbdate_, selsec.c_str());
			sprintf(cmd1, " delete from stockParams1bySymbol where idate = %d "
					" and symbol in (select symbol from hfuniverse where idate = %d and %s ) ", dbdate_, dbdate_, selsec.c_str());
			sprintf(cmd2, " delete from stockParams1bySymbolSprd where idate = %d "
					" and symbol in (select symbol from hfuniverse where idate = %d and %s ) ", dbdate_, dbdate_, selsec.c_str());
			sprintf(cmd3, " delete from hfUniverse where idate = %d and %s ", dbdate_, selsec.c_str());

			check_and_delete(dbname_, chk1, cmd1);
			check_and_delete(dbname_, chk2, cmd2);
			check_and_delete(dbname_, chk3, cmd3);
		}
	}
	else if( model02 == "CA" )
	{
		sprintf(chk1, " select count(*) from stockParams1bySymbol where idate = %d ", dbdate_);
		sprintf(chk2, " select count(*) from stockParams1bySymbolSprd where idate = %d ", dbdate_);
		sprintf(chk3, " select count(*) from hfUniverse where idate = %d ", dbdate_);
		sprintf(cmd1, " delete from stockParams1bySymbol where idate = %d ", dbdate_);
		sprintf(cmd2, " delete from stockParams1bySymbolSprd where idate = %d ", dbdate_);
		sprintf(cmd3, " delete from hfUniverse where idate = %d ", dbdate_);

		check_and_delete(dbname_, chk1, cmd1);
		check_and_delete(dbname_, chk2, cmd2);
		check_and_delete(dbname_, chk3, cmd3);
	}
	else
	{
		string selex = select_exchange(markets);
		sprintf(chk1, " select count(*) from stockParams1bySymbol where idate = %d and exchange %s", dbdate_, selex.c_str());
		sprintf(chk2, " select count(*) from stockParams1bySymbolSprd where idate = %d and exchange %s ", dbdate_, selex.c_str());
		sprintf(chk3, " select count(*) from hfUniverse where idate = %d and exchange %s ", dbdate_, selex.c_str());
		sprintf(cmd1, " delete from stockParams1bySymbol where idate = %d and exchange %s", dbdate_, selex.c_str());
		sprintf(cmd2, " delete from stockParams1bySymbolSprd where idate = %d and exchange %s ", dbdate_, selex.c_str());
		sprintf(cmd3, " delete from hfUniverse where idate = %d and exchange %s ", dbdate_, selex.c_str());

		check_and_delete(dbname_, chk1, cmd1);
		check_and_delete(dbname_, chk2, cmd2);
		check_and_delete(dbname_, chk3, cmd3);
	}

	qa_.clear();
}

string BiLinearModelWeights::select_exchange(const vector<string>& markets)
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

void BiLinearModelWeights::write_weights_ticker(const string& model, const string& market, const string& uid, const string& ticker, int iHorizon,
		double volume, double close, double volat, double medSprd, int inUniv, double nFFundRST, double nFFundTRD, double nFFundBP,
		const string& secType, const string& exchangeChar, bool debug_noss )
{
	string model02 = model.substr(0, 2);

	// Define exchange.
	string exchange;
	if( model02 == "US" || model02 == "UF" || model02 == "CA" )
		exchange = exchangeChar;
	else
		exchange = market.substr(1, 1);
	string compSymbol = (exchange.empty()) ? ticker : (exchange + ":" + ticker);

	// check variables.
	if( volume <= 1 || close <= hff::min_price_ || volat <= 0.00001 || medSprd <= .00001 ) // Should write zeros?
		return;
	if( ticker.empty() )
		return;

	// inUnivDB.
	int inUnivDB = -1;
	if( inUniv == 1 )
		inUnivDB = 1;
	else
	{
		if( portSyms_.count(ticker) )
			inUnivDB = 0;
		else
			inUnivDB = -1;
	}

	// Ticker specific linear model.
	vector<float> wgt_stock = vector<float>(hff::om_num_basic_sig_, 0.);
	bool noLunchBreak = market != "AT" && market != "AH" && market != "AG" && market != "AA";

	bool do_write = true;
	if( weightsErr_[iHorizon].find(uid) != weightsErr_[iHorizon].end() )
	{
		wgt_stock = weightsErr_[iHorizon][uid];

		// Stock specific model is not good.
		if( wgt_stock[2] <= -100 )
		{
			if( inUnivDB < 0 )
				do_write = false;
			else
			{
				wgt_stock = vector<float>(hff::om_num_basic_sig_, 0.);
				cout << "Ticker-specific signal for " << compSymbol << " (inuniv " << inUnivDB << ") wgt[2] <= -100. Resetting to zeros.\n";
				do_write = true;
			}
		}

		// Stock specific model is not fitted.
		else if( noLunchBreak && wgt_stock[0] == 0. )
		{
			if( inUnivDB < 0 )
				return;
			else
			{
				wgt_stock = vector<float>(hff::om_num_basic_sig_, 0.);
				cout << "Ticker-specific signal for " << compSymbol << " (inuniv " << inUnivDB << ") wgt[0] == 0. Resetting to zeros.\n";
				do_write = true;
			}
		}
	}
	else // Stock specific model is not found.
	{
		if( inUnivDB < 0 )
			return;
		else
		{
			cout << "Ticker-specific signal for " << compSymbol << " (inuniv " << inUnivDB << ") not found.\n";
			do_write = true;
		}
	}

	if( !do_write )
		return;

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
				volat, volume, close, medSprd, nFFundRST, nFFundTRD, nFFundBP, secType.c_str());

		sprintf(upt, " update hfUniverse set "
				" inUniverse = %d, exchange = '%s', volatility = %f, volume = %f, close_ = %f, "
				" medsprd = %f, nFFundRST = %f, nFFundTRD = %f, nFFundBP = %f, secType = '%s' "
				" where idate = %d and symbol = '%s' ",
				inUnivDB, exchange.c_str(), volat, volume, close,
				medSprd, nFFundRST, nFFundTRD, nFFundBP, secType.c_str(),
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
				volat, volume, close, medSprd, nFFundRST, nFFundTRD, nFFundBP, secType.c_str());
		if( efficient_write_ )
			qa_.add(cmd);
		else
			GODBC::Instance()->get(dbname_)->ExecDirect(cmd);
	}

	// Write weights.
	if( num_time_slices_ == 0 )
	{
		vector<float> omwgts(hff::om_num_basic_sig_);

		for( int i = 0; i < hff::om_num_basic_sig_; ++i )
		{
			// stock specific model.
			if( noLunchBreak || wgt_stock[0] > -99 )
				omwgts[i] += wgt_stock[i];
		}

		// write weights.
		if( model02 == "US" || model02 == "UF" )
		{
			char chk[2000];
			char ins[2000];
			char upt[2000];

			sprintf(chk, " select count(*) from stockParams1bySymbol "
					" where symbol = '%s' and itime = %d and idate = %d ",
					ticker.c_str(), 0, dbdate_);

			sprintf(ins, " insert into stockParams1bySymbol (symbol, itime, idate, "
					" qutImb, fillImb, ret, hilo, retMkt, retFut, lowcapIdx, lowcapFut, inUniverse, "
					" qutImb1, qutImb2, midOff1, midOff2, russIdx, russLowcap, mret300, mret300lag) "
					" VALUES ('%s', %d, %d, %f, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %f, %f, %f, %f, %f, %f) ",
					ticker.c_str(), 0, dbdate_,
					omwgts[0], omwgts[1], omwgts[2], omwgts[3], omwgts[4], omwgts[5], omwgts[6], omwgts[7], inUnivDB,
					omwgts[8], omwgts[9], omwgts[10], omwgts[11], omwgts[12], omwgts[13], omwgts[14], omwgts[15]);

			sprintf(upt, " update stockParams1bySymbol set "
					" qutImb = %f, fillImb = %f, ret = %f, hilo = %f, retMkt = %f, retFut = %f, lowcapIdx = %f, lowcapFut = %f, inUniverse = %d, "
					" qutImb1 = %f, qutImb2 = %f, midOff1 = %f, midOff2 = %f, russIdx = %f, russLowcap = %f, mret300 = %f, mret300lag = %f "
					" where symbol = '%s' and itime = %d and idate = %d ",
					omwgts[0], omwgts[1], omwgts[2], omwgts[3], omwgts[4], omwgts[5], omwgts[6], omwgts[7], inUnivDB,
					omwgts[8], omwgts[9], omwgts[10], omwgts[11], omwgts[12], omwgts[13], omwgts[14], omwgts[15],
					ticker.c_str(), 0, dbdate_);

			check_and_insert_or_update(dbname_, chk, ins, upt);
		}
		else
		{
			char cmd[2000];
			if( model02 == "CA" )
			{
				sprintf(cmd, " insert into stockParams1bySymbol (symbol,itime, idate, "
						" qutImb, fillImb, ret, hilo, tsxPtsx, sxfPtsx, volatility, volume, inUniverse, "
						" qutImb1, qutImb2, midOff1, midOff2, close_, mret300, mret300lag, medSprd) "
						" VALUES ('%s', %d, %d, %f, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %f, %f, %f, %f, %f, %f)",
						ticker.c_str(), 0, dbdate_,
						omwgts[0], omwgts[1], omwgts[2], omwgts[3], omwgts[4], omwgts[5], volat, volume, inUnivDB,
						omwgts[8], omwgts[9], omwgts[10], omwgts[11], close, omwgts[14], omwgts[15], medSprd);
			}
			else
			{
				sprintf(cmd, "insert into stockParams1bySymbol (symbol, itime, idate, "
						" qutImb, fillImb, ret, hilo, euxPeux, volatility, volume, inUniverse, "
						" close_, qutImb1, qutImb2, midOff1, midOff2, mret300, mret300lag, medSprd, exchange) "
						" VALUES ('%s', %d, %d, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %d, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, '%s')",
						ticker.c_str(), timeSliceStart_[0], dbdate_,
						omwgts[0], omwgts[1], omwgts[2], omwgts[3], omwgts[4], volat, volume, inUnivDB,
						close, omwgts[8], omwgts[9], omwgts[10], omwgts[11], omwgts[14], omwgts[15], medSprd, exchange.c_str());
			}
			if( efficient_write_ )
				qa_.add(cmd);
			else
				GODBC::Instance()->get(dbname_)->ExecDirect(cmd);
		}
	}
	else
	{
		double logV = log10(volume);
		double logS = log10(close);
		vector<float> omwgts(hff::om_num_basic_sig_);
		vector<float> omwgtsSprd(hff::om_num_basic_sig_);
		for( int iTime = 0; iTime < num_time_slices_; ++iTime )
		{
			vector<float>& wgt_univ = weights_[iHorizon][iTime];

			for( int i = 0; i < hff::om_num_basic_sig_; ++i )
			{
				// universal model.
				omwgts[i] = wgt_univ[i] + logV * wgt_univ[i + hff::om_num_basic_sig_] + logS * wgt_univ[i + hff::om_num_basic_sig_ * 2];

				// stock specific model.
				if( noLunchBreak || wgt_stock[0] > -99 )
					omwgts[i] += wgt_stock[i];

				// sprd model.
				omwgtsSprd[i] = wgt_univ[i + hff::om_num_basic_sig_ * 3];
			}

			// write weights.
			if( model02 == "US" || model02 == "UF" )
			{
				char chk[2000];
				char ins[2000];
				char upt[2000];

				sprintf(chk, " select count(*) from stockParams1bySymbol "
						" where symbol = '%s' and itime = %d and idate = %d ",
						ticker.c_str(), iTime * 60 * 30, dbdate_);

				sprintf(ins, " insert into stockParams1bySymbol (symbol, itime, idate, "
						" qutImb, fillImb, ret, hilo, retMkt, retFut, lowcapIdx, lowcapFut, inUniverse, "
						" qutImb1, qutImb2, midOff1, midOff2, russIdx, russLowcap, mret300, mret300lag) "
						" VALUES ('%s', %d, %d, %f, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %f, %f, %f, %f, %f, %f) ",
						ticker.c_str(), iTime * 60 * 30, dbdate_,
						omwgts[0], omwgts[1], omwgts[2], omwgts[3], omwgts[4], omwgts[5], omwgts[6], omwgts[7], inUnivDB,
						omwgts[8], omwgts[9], omwgts[10], omwgts[11], omwgts[12], omwgts[13], omwgts[14], omwgts[15]);

				sprintf(upt, " update stockParams1bySymbol set "
						" qutImb = %f, fillImb = %f, ret = %f, hilo = %f, retMkt = %f, retFut = %f, lowcapIdx = %f, lowcapFut = %f, inUniverse = %d, "
						" qutImb1 = %f, qutImb2 = %f, midOff1 = %f, midOff2 = %f, russIdx = %f, russLowcap = %f, mret300 = %f, mret300lag = %f "
						" where symbol = '%s' and itime = %d and idate = %d ",
						omwgts[0], omwgts[1], omwgts[2], omwgts[3], omwgts[4], omwgts[5], omwgts[6], omwgts[7], inUnivDB,
						omwgts[8], omwgts[9], omwgts[10], omwgts[11], omwgts[12], omwgts[13], omwgts[14], omwgts[15],
						ticker.c_str(), iTime * 60 * 30, dbdate_);

				check_and_insert_or_update(dbname_, chk, ins, upt);
			}
			else
			{
				char cmd[2000];
				if( model02 == "CA" )
				{
					sprintf(cmd, " insert into stockParams1bySymbol (symbol,itime, idate, "
							" qutImb, fillImb, ret, hilo, tsxPtsx, sxfPtsx, volatility, volume, inUniverse, "
							" qutImb1, qutImb2, midOff1, midOff2, close_, mret300, mret300lag, medSprd) "
							" VALUES ('%s', %d, %d, %f, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %f, %f, %f, %f, %f, %f)",
							ticker.c_str(), iTime * 60 * 30, dbdate_,
							omwgts[0], omwgts[1], omwgts[2], omwgts[3], omwgts[4], omwgts[5], volat, volume, inUnivDB,
							omwgts[8], omwgts[9], omwgts[10], omwgts[11], close, omwgts[14], omwgts[15], medSprd);
				}
				else
				{
					sprintf(cmd, "insert into stockParams1bySymbol (symbol, itime, idate, "
							" qutImb, fillImb, ret, hilo, euxPeux, volatility, volume, inUniverse, "
							" close_, qutImb1, qutImb2, midOff1, midOff2, mret300, mret300lag, medSprd, exchange) "
							" VALUES ('%s', %d, %d, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %d, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, '%s')",
							ticker.c_str(), timeSliceStart_[iTime], dbdate_,
							omwgts[0], omwgts[1], omwgts[2], omwgts[3], omwgts[4], volat, volume, inUnivDB,
							close, omwgts[8], omwgts[9], omwgts[10], omwgts[11], omwgts[14], omwgts[15], medSprd, exchange.c_str());
				}
				if( efficient_write_ )
					qa_.add(cmd);
				else
					GODBC::Instance()->get(dbname_)->ExecDirect(cmd);
			}

			// write sprd weights.
			if( model02 == "US" || model02 == "UF" )
			{
				char chk[2000];
				char ins[2000];
				char upt[2000];

				sprintf(chk, " select count(*) from stockParams1bySymbolSprd "
						" where symbol = '%s' and itime = %d and idate = %d ",
						ticker.c_str(), iTime * 60 * 30, dbdate_);

				sprintf(ins, " insert into stockParams1bySymbolSprd (symbol, itime, idate, "
						" qutImb, fillImb, ret, hilo, retMkt, retFut, lowcapIdx, lowcapFut, inUniverse, "
						" qutImb1, qutImb2, midOff1, midOff2, russIdx, russLowcap, mret300, mret300lag) "
						" VALUES ('%s', %d, %d, %f, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %f, %f, %f, %f, %f, %f) ",
						ticker.c_str(), iTime * 60 * 30, dbdate_,
						omwgtsSprd[0], omwgtsSprd[1], omwgtsSprd[2], omwgtsSprd[3], omwgtsSprd[4], omwgtsSprd[5], omwgtsSprd[6], omwgtsSprd[7], inUnivDB,
						omwgtsSprd[8], omwgtsSprd[9], omwgtsSprd[10], omwgtsSprd[11], omwgtsSprd[12], omwgtsSprd[13], omwgtsSprd[14], omwgtsSprd[15]);

				sprintf(upt, " update stockParams1bySymbolSprd set "
						" qutImb = %f, fillImb = %f, ret = %f, hilo = %f, retMkt = %f, retFut = %f, lowcapIdx = %f, lowcapFut = %f, inUniverse = %d, "
						" qutImb1 = %f, qutImb2 = %f, midOff1 = %f, midOff2 = %f, russIdx = %f, russLowcap = %f, mret300 = %f, mret300lag = %f "
						" where symbol = '%s' and itime = %d and idate = %d ",
						omwgtsSprd[0], omwgtsSprd[1], omwgtsSprd[2], omwgtsSprd[3], omwgtsSprd[4], omwgtsSprd[5], omwgtsSprd[6], omwgtsSprd[7], inUnivDB,
						omwgtsSprd[8], omwgtsSprd[9], omwgtsSprd[10], omwgtsSprd[11], omwgtsSprd[12], omwgtsSprd[13], omwgtsSprd[14], omwgtsSprd[15],
						ticker.c_str(), iTime * 60 * 30, dbdate_);

				check_and_insert_or_update(dbname_, chk, ins, upt);
			}
			else
			{
				char cmd[2000];
				if( model02 == "CA" )
				{
					sprintf(cmd, " insert into stockParams1bySymbolSprd (symbol,itime, idate, "
							" qutImb, fillImb, ret, hilo, tsxPtsx, sxfPtsx, volatility, volume, inUniverse, "
							" qutImb1, qutImb2, midOff1, midOff2, close_, mret300, mret300lag) "
							" VALUES ('%s', %d, %d, %f, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %f, %f, %f, %f, %f)",
							ticker.c_str(), iTime * 60 * 30, dbdate_,
							omwgtsSprd[0], omwgtsSprd[1], omwgtsSprd[2], omwgtsSprd[3], omwgtsSprd[4], omwgtsSprd[5], volat, volume, inUnivDB,
							omwgtsSprd[8], omwgtsSprd[9], omwgtsSprd[10], omwgtsSprd[11], close, omwgtsSprd[14], omwgtsSprd[15]);
				}
				else
				{
					sprintf(cmd, " insert into stockParams1bySymbolSprd (symbol, itime, idate, "
							" qutImb, fillImb, ret, hilo, euxPeux, volatility, volume, inUniverse, "
							" close_, qutImb1, qutImb2, midOff1, midOff2, mret300, mret300lag, exchange) "
							" VALUES ('%s', %d, %d, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %f, %f, %f, %f, %f, '%s')",
							ticker.c_str(), timeSliceStart_[iTime], dbdate_,
							omwgtsSprd[0], omwgtsSprd[1], omwgtsSprd[2], omwgtsSprd[3], omwgtsSprd[4], volat, volume, inUnivDB,
							close, omwgtsSprd[8], omwgtsSprd[9], omwgtsSprd[10], omwgtsSprd[11], omwgtsSprd[14], omwgtsSprd[15], exchange.c_str());
				}
				if( efficient_write_ )
					qa_.add(cmd);
				else
					GODBC::Instance()->get(dbname_)->ExecDirect(cmd);
			}
		}
	}
}

void BiLinearModelWeights::write_weights_endDay(const string& model, const string& market, char mCode, bool freeze_univ)
{
	qa_.exec(dbname_);

	endDay_paramsOK(model, market, mCode);
	endDay_random_univ(model, freeze_univ);
}

void BiLinearModelWeights::endDay_paramsOK(const string& model, const string& market, char mCode)
{
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
}	

void BiLinearModelWeights::endDay_random_univ(const string& model, bool freeze_univ)
{
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
	string model02 = model.substr(0, 2);
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

		// Write the same inUniverse values to stockParams1bySymbol and stockParams1bySymbolSprd.
		char cmd1[400];
		char cmd2[400];
		if( model02 == "US" || model02 == "UF" || model02 == "CA" )
		{
			sprintf(cmd1, " update stockParams1bySymbol set inUniverse = %d where idate = %d and symbol = '%s' ",
					iUniv, dbdate_, ticker.c_str(), exchange.c_str());
			sprintf(cmd2, " update stockParams1bySymbolSprd set inUniverse = %d where idate = %d and symbol = '%s' ",
					iUniv, dbdate_, ticker.c_str(), exchange.c_str());
		}
		else
		{
			sprintf(cmd1, " update stockParams1bySymbol set inUniverse = %d where idate = %d and symbol = '%s' and exchange = '%s' ",
					iUniv, dbdate_, ticker.c_str(), exchange.c_str());
			sprintf(cmd2, " update stockParams1bySymbolSprd set inUniverse = %d where idate = %d and symbol = '%s' and exchange = '%s' ",
					iUniv, dbdate_, ticker.c_str(), exchange.c_str());
		}
		if( efficient_write_ )
			qa_univ.add(cmd1);
		else
			GODBC::Instance()->exec(dbname_, cmd1);
		if( num_time_slices_ > 0 )
		{
			if( efficient_write_ )
				qa_univ.add(cmd2);
			else
				GODBC::Instance()->exec(dbname_, cmd2);
		}
	}
	if( efficient_write_ )
		qa_univ.exec(dbname_);
}

//
// Prediction.
//

double BiLinearModelWeights::pred(int iT, int timeIdx, const vector<float>& v)
{
	if( num_time_slices_ == 0 )
		return 0;

	double pred = 0.;

	vector<float>& wgts = weights_[iT][timeIdx];
	int Ni = v.size();
	int Nw = wgts.size();
	if( Ni == Nw )
	{
		for( int i = 0; i < Ni; ++i )
		{
			if( wgts[i] > -99 )
			{
				float partial = getPartialPred(i, wgts, v);
				pred += partial;
			}
		}
	}
	else
	{
		cerr << "BiLinearModelWeights::pred() ERROR\n";
		cout << Ni << "\t" << Nw << endl;
		copy(wgts.begin(), wgts.end(), ostream_iterator<float>(cout, "\t"));
		cout << endl;
		copy(v.begin(), v.end(), ostream_iterator<float>(cout, "\t"));
		cout << endl;
		exit(3);
	}
	return pred;
}

double BiLinearModelWeights::predIndex(int iT, int timeIdx, const vector<float>& v)
{
	vector<float> vLocal = v;
	for( int i = 0; i < hff::om_num_basic_sig_; ++i )
	{
		if( i != 4 && i != 5 && i != 6 && i != 7 && i != 12 && i != 13 )
		{
			int indx1 = i;
			int indx2 = i + hff::om_num_basic_sig_;
			int indx3 = i + 2 * hff::om_num_basic_sig_;
			int indx4 = i + 3 * hff::om_num_basic_sig_;

			vLocal[indx1] = 0.;
			vLocal[indx2] = 0.;
			vLocal[indx3] = 0.;
			vLocal[indx4] = 0.;
		}
	}

	double ret = pred(iT, timeIdx, vLocal);
	return ret;
}

double BiLinearModelWeights::predErr(int iT, const string& uid, const vector<float>& v)
{
	double pred = 0.;

	vector<float>& wgts = weightsErr_[iT][uid];
	int Ni = v.size();
	int Nw = wgts.size();
	if( Nw == 0 )
		;
	else if( Ni == Nw )
	{
		for( int i = 0; i < Ni; ++i )
		{
			if( wgts[i] > -99 )
			{
				float partial = getPartialPredErr(i, wgts, v);
				pred += partial;
			}
		}
	}
	else
	{
		cerr << "BiLinearModelWeights::pred() ERROR\n";
		cout << Ni << "\t" << Nw << endl;
		copy(wgts.begin(), wgts.end(), ostream_iterator<float>(cout, "\t"));
		cout << endl;
		copy(v.begin(), v.end(), ostream_iterator<float>(cout, "\t"));
		cout << endl;
		exit(3);
	}
	return pred;
}

double BiLinearModelWeights::predErrIndex(int iT, const string& uid, const vector<float>& v)
{
	vector<float> vLocal = v;
	for( int i = 0; i < hff::om_num_basic_sig_; ++i )
	{
		if( i != 4 && i != 5 && i != 6 && i != 7 && i != 12 && i != 13 )
			vLocal[i] = 0.;
	}

	double ret = predErr(iT, uid, vLocal);
	return ret;
}

double BiLinearModelWeights::predDB(int timeIdx, const string& uid, const vector<float>& v)
{
	double pred = 0.;

	vector<float>& wgts = weightsDB_[timeIdx][uid];
	vector<float>& wgtsSprd = weightsDBsprd_[timeIdx];
	if( wgts.size() >= om_num_basic_sig_ && wgtsSprd.size() >= om_num_basic_sig_ )
	{
		for( int i = 0; i < om_num_basic_sig_; ++i )
		{
			if( wgts[i] > -99 )
				pred += wgts[i] * v[i];
			if( wgtsSprd[i] > -99 )
				pred += wgtsSprd[i] * v[i + 3 * om_num_basic_sig_];
		}
	}
	return pred;
}

double BiLinearModelWeights::getPartialPred(int i, const vector<float>& wgts, const vector<float>& v)
{
	if( use_sigmoid_ )
	{
		if( !range_.empty() && range_[i] != 0. )
			return range_[i] * wgts[i] * tanh(v[i] / range_[i]);
		else
			return 0.;
	}
	return wgts[i] * v[i];
}

double BiLinearModelWeights::getPartialPredErr(int i, const vector<float>& wgts, const vector<float>& v)
{
	if( use_sigmoid_ )
	{
		if( !rangeErr_.empty() && rangeErr_[i] != 0. )
			return rangeErr_[i] * wgts[i] * tanh(v[i] / rangeErr_[i]);
		else
			return 0.;
	}
	return wgts[i] * v[i];
}
