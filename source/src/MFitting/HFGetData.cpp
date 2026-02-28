#include <MFitting/HFSTree.h>
#include <algorithm>

using namespace std;
//void getIdxRetsSQL(int *dts, int ndts,  hfData** ret, string what)
//{
//	float dbret;
//	char sqlCmd[LARGEBUFFSIZE];
//	ODBCConnection odbcEQ("equityData","mercator1","DBacc101");
//
//	double clip = MAX_IDX_1SEC_RET;
//
//	int day = 0;
//	for(int u=0;u<ndts;++u)
//	{
//		sprintf(sqlCmd,"select ret from locapIdx where idate=%d order by isec",dts[u]);
//		odbcEQ.ExecDirect(sqlCmd);
//
//		odbcEQ.BindColFloat(0,&dbret);
//		int npt=0;
//		while(odbcEQ.NextFetch())
//		{	if(npt>=23400)
//			break;		
//			ret[day][npt+1].x = BASIS_PTS * dbret;
//			ret[day][npt+1].ok = 2;	// make sure this off-set of +1 is right 
//			if(fabs(ret[day][npt+1].x) > clip)
//				ret[day][npt+1].x=clip*ret[day][npt+1].x/fabs(ret[day][npt+1].x); // clip outliers
//			++npt;
//		}
//		odbcEQ.EndFetch();
//		day++;
//
//		if(npt<23400)
//		{
//			char buf[BUFFSIZE];
//			sprintf(buf,"number of locapIdx points does not equal 23400 date = %d",dts[u]);
//			ERROR_HANDLER(buf,EL_WARNING);
//		}
//	}
//	
//	return; 
//}
//
//void CALL_CONVENTION getIdxReturns(int *dts, int ndts,  hfData** ret, string what, int n1sec)
//{
//	double clip;
//	if(what == "SPX_RET" || what == "TSX_RET" || what == "LSE_RET" || what == "PANEU_RET" ||
//		what == "PANEU_HC_RET" || what == "PANEU_LC_RET" ||
//		what == "LCX_RET" || what == "T_RET" || what == "H_RET" || what == "S_RET" || what == "K_RET" || 
//		what == "Q_RET" || what == "KR_RET" || what == "J_RET" || what == "G_RET" || what == "W_RET" ||
//		what == "A_RET" || what == "B_RET" || what == "P_RET" || what == "F_RET" || what == "L_RET" || 
//		what == "D_RET" || what == "M_RET" || what == "Z_RET" 
//		)
//		clip = MAX_IDX_1SEC_RET;
//	else if( what == "PANEU_SIG" )
//		clip = 2500.;
//	else
//		clip = MAX_FUT_1SEC_RET;
//
//	Exchange ex;
//	ODBCConnection eqData;
//	char sqlCmd[LARGEBUFFSIZE];
//	string sqlstring=" SELECT switchDate, stocksDirectory, futuresDirectory, statsTable, oktable FROM tickDataSources ";
// 	int sgPts = n1sec; 
//	bool longISIN = false;
//	if(what == "TSX_RET" || what == "SXF_RET" || what == "USO_RET" || what == "GLD_RET" || what == "TLT_RET")
//	{
//		ex.Set("CA");
//		eqData.Set("hfstockTsx","mercator1","DBacc101");
//		sqlstring += (string) " where market = 'J' ";
//	}
//	else if(what == "LSE_RET")
//	{
//		ex.Set("GB");
//		eqData.Set("hfStockEU","mercator1","DBacc101");
//		sqlstring += (string) " where market = 'L' ";
//		longISIN = true;
//	}
////WIST
//	else if(what == "PANEU_RET" ||	what == "PANEU_SIG" || what == "PANEU_HC_RET" || what == "PANEU_LC_RET" || what == "E5_FUT"
//	 ||	what == "A_RET" ||	what == "EA_FUT"
//	 ||	what == "B_RET" ||	what == "EB_FUT"
//	 ||	what == "P_RET" ||	what == "EP_FUT"
//	 ||	what == "F_RET" ||	what == "EF_FUT"
//	 ||	what == "L_RET"	||	what == "EL_FUT"
//	 ||	what == "D_RET"	||	what == "ED_FUT"
//	 ||	what == "M_RET"	||	what == "EM_FUT"
//	 ||	what == "Z_RET"	||	what == "EZ_FUT"
//	)
//	{
//		ex.Set("GB");
//		eqData.Set("hfStockEU","mercator1","DBacc101");  // There is a copy of PAN_EU ret in here 
//		if( what.substr(1,4) == "_RET" )
//			sqlstring += string(" where market = '") + what.substr(0,1) + string("' ");
//		else
//			sqlstring += (string) " where market = 'L' ";
//	
//		longISIN = true;
//	}
//	else if(what == "T_RET")
//	{
//		ex.Set("JP");
//		eqData.Set("hfstockAsia","mercator1","DBacc101");
//		sqlstring += (string) " where market = 'T' ";
//		longISIN = false; 
//	}
//	else if(what == "H_RET")
//	{
//		ex.Set("HK");
//		eqData.Set("hfstockAsia","mercator1","DBacc101");
//		sqlstring += (string) " where market = 'H' ";
//		longISIN = false; 
//	}
//	else if(what == "S_RET")
//	{
//		ex.Set("AU");
//		eqData.Set("hfstockAsia","mercator1","DBacc101");
//		sqlstring += (string) " where market = 'S' ";
//		longISIN = false; 
//	}
//	//else if(what == "K_RET")
//	//{
//	//	ex.Set("KR");
//	//	eqData.Set("hfstockAsia","mercator1","DBacc101");
//	//	sqlstring += (string) " where market = 'K' ";
//	//	longISIN = false; 
//	//}
//	//else if(what == "Q_RET")
//	//{
//	//	ex.Set("KR");
//	//	eqData.Set("hfstockAsia","mercator1","DBacc101");
//	//	sqlstring += (string) " where market = 'Q' ";
//	//	longISIN = false; 
//	//}
//	else if(what == "KR_RET")
//	{
//		ex.Set("KR");
//		eqData.Set("hfstockAsia","mercator1","DBacc101");
//// WIST - not sure about this
//		sqlstring += (string) " where market = 'K' ";
//		longISIN = false; 
//	}
//	else if(what == "J_RET")
//	{
//		ex.Set("ZA");
//		eqData.Set("hfstockMA","mercator1","DBacc101");
//		sqlstring += (string) " where market = 'J' ";
//		longISIN = false; 
//	}
//	else if(what == "G_RET")
//	{
//		ex.Set("SG");
//		eqData.Set("hfstockAsia","mercator1","DBacc101");
//		sqlstring += (string) " where market = 'G' ";
//		longISIN = false; 
//	}
//	else if(what == "W_RET")
//	{
//		ex.Set("TW");
//		eqData.Set("hfstockAsia","mercator1","DBacc101");
//		sqlstring += (string) " where market = 'W' ";
//		longISIN = false; 
//	}
//	else
//	{
//		ex.Set("US");
//		eqData.Set("equityData","mercator1","DBacc101");
//		if(USE_MIRROR_DATA) // if not in production, then use Mirror data
//			sqlstring=" SELECT switchDate, stocksDirectory, futuresDirectory, statsTable, oktable FROM tickDataSourcesMirror order by switchdate";
//	}
//	//20091111
//	sqlstring += (string) " order by switchdate ";
//	sprintf(sqlCmd,sqlstring.c_str());
//	eqData.Open();
//
//	////////////////////   get the proper directories for the data 
//	vector<vector<string> >   tDSources;
//	eqData.ReadTable(sqlCmd,&tDSources);
//	int ntDSources =(int)tDSources.size();	
//	
//	int day = 0;
//	int msecsOpen=ROUND(ex.OpenTime().Days()*86400000.);
//	for(int u=0;u<ndts;++u)
//	{
//		// figure out where FUTURE AND IDX data is 
//		int tdataidx = -1;
//		for(int m=0; m< ntDSources; m++)
//			if(dts[u] >= atof(tDSources[m][0].c_str()) )
//				tdataidx++;
//
//		if(tdataidx == -1)
//			continue;
//		
//		string tdataDir = tDSources[tdataidx][1];
//
//		if(what == "USO_RET" || what == "GLD_RET" || what == "TLT_RET")
//			tdataDir = "\\\\smrc-ltc-mrct40\\L\\tickC\\us_test\\commodity";
////WIST		
//		else if(what == "KR_RET")
//			tdataDir = "\\\\smrc-ltc-mrct16\\data00\\tickC_test\\asia\\return\\kr";
////WIST
//		else if(what.substr(0,1) == "E" && what.substr(2,4) == "_FUT" && what.substr(1,1) != "5")
//		{
//			tdataDir = string("\\\\smrc-ltc-mrct16\\data00\\tickC\\eu\\futureHist\\") + 
//				what.substr(1,1); 
//		}
//		else if(what == "E5_FUT")
//		{
//			tdataDir = string("\\\\smrc-ltc-mrct16\\data00\\tickC\\eu\\futureHist\\A"); 
//		}
//		else if( what.substr(0, 5) == "PANEU" )
//		{
//			tdataDir = string("\\\\smrc-ltc-mrct16\\data00\\tickC\\eu\\return");
//		}
//
//
//	    TickAccess<ReturnData> taR(tdataDir,longISIN);
//		TickSeries<ReturnData> tsR;
//		taR.GetTickSeries(what,dts[u],&tsR);
//		
//		ReturnData *trades = NULL;
//		int nTrades = 0;
//		tsR.ReadSeries(&trades,&nTrades);
//
//		if (nTrades >= 11001) // changed this to much smaller number for TW
//		{	
//			int firsttime = ROUND(0.001*(trades[0].msecs - msecsOpen));
//			if((firsttime + nTrades) == sgPts) 
//			{	
//				for(int i=firsttime; i<sgPts; i++)
//				{	
//					ret[day][i].x = BASIS_PTS * trades[i-firsttime].ret; 
//					ret[day][i].ok = 2;
//					if(fabs(ret[day][i].x) > clip)
//						ret[day][i].x=clip*ret[day][i].x/fabs(ret[day][i].x); // clip outliers
//				}
//			}
//			else
//				cout << "Starttime + ntrades does not add up to 23401 for "<< what << ", " << "date = " << dts[u] << endl;
//
//		}
//	
//		if(trades) delete trades; 
//		day++;  
//	}	
//	return; 
//}
//
//void CALL_CONVENTION SgetIdxReturns(char** what, long *date, double* ret)
//{
//	int numDays = 1;
//	int numTs = 1;
//	int dts = (int) *date;
//	int i;
//
//	tsmodel mod;
//	mod.numTs = 1;
//	mod.tsNames[0] = what[0];
//	
//int n1sec = 23401;
//
//	hfData*** idxdat;
//	idxdat = callochfDataTensor(numTs,numDays,n1sec);
//	
//	for(i=0;i<numTs;i++)
//		getIdxReturns(&dts,1,idxdat[i],mod.tsNames[i],n1sec);
//	
//	for(i=0;i<n1sec; i++) 	ret[i] = idxdat[0][0][i].x;
//		
//	freehfDataTensor(idxdat, numTs, numDays, n1sec);
//	return;
//}
//void CALL_CONVENTION writeIdxReturns(string ticker, int date)
//{
//	int numDays = 1;
//	int numTs = 1;
//	int dts = date;
//	int i;
//
//	tsmodel mod;
//	mod.numTs = 1;
//	mod.tsNames[0] = ticker;
//	
//int n1sec=23401; 
//
//	hfData*** idxdat;
//	idxdat = callochfDataTensor(numTs,numDays,n1sec);
//	
//	for(i=0;i<numTs;i++)
//		getIdxReturns(&dts,1,idxdat[i],mod.tsNames[i],n1sec);
//	
//	char filename[BUFFSIZE];
//	sprintf(filename,"%s%s%dsignal.txt",DEBUG_DIR,ticker.c_str(),date);
//	fstream rets(filename,ios::out);
//	rets.precision(6);
//	
//	for(i=0;i<n1sec; i++)
//		rets << idxdat[0][0][i].x << endl;
//		
//	freehfDataTensor(idxdat, numTs, numDays, n1sec);
//	return;
//}
//
//void makeSQLCommand::getUniverse(ExchangeInfo ei,int sdate, int edate)
//{
//	string mkt=(string)" market = '" + ei.market + (string)"'";
//	if(ei.model==101 || ei.model == 105) // use all listing markets
//		mkt = "(market = 'Q' or market != 'Q') ";
//	else if(ei.model==107)
//	{
//		mkt = " market IN ('A','B','F','I','P','Z','X','C','W','Y','M','D','O','L')  ";	
//	}
//	else if(ei.model==114)
//	{
//		mkt = " market IN ('K','Q')  ";
//	}
//	else if(ei.model==115)
//	{
//		mkt = " secType = 'F' ";
//	}
//
//	QuoteTime xfDate(edate,120000,ei.timeZone);    
//	QuoteTime xpDate=ei.ex->PrevClose(xfDate);  
//	QuoteTime sfDate(sdate,120000,ei.timeZone);    
//	QuoteTime spDate=ei.ex->PrevClose(sfDate);  
//
//	if(ei.model==101)   
//		sprintf(sqlCmd,"SELECT DISTINCT(uniqueID) "
//				"FROM stockCharacteristics "
//				"WHERE idate <= %d and idate >= %d and "
//				//"uniqueID > 'T' and uniqueID < 'U' and " 
//				"( inUniverse = 1  OR  " 
//				" ( close_ > .5 and "
//				"close_ * medVolume > 60000 and "
//				"medVolatility > .005 and close_ < 900 and medmedsprd >= .00008 and medmedsprd<.04) ) "
//				"and uniqueID IS NOT NULL " 
//				"and sectype != 'P' "
//				"and sectype != 'F' " 
//				"and sectype != 'X' " 
//				" ORDER BY uniqueID ",
//				(int)xpDate.Date(), (int)spDate.Date()); 
//
//	else if(ei.model==115)   
//		sprintf(sqlCmd,"SELECT DISTINCT(uniqueID) "
//				"FROM stockCharacteristics "
//				"WHERE idate <= %d and idate >= %d and "
//				" ( close_ > .5 and "
//				"close_ * medVolume > 60000 and "
//				"medVolatility > .005 and close_ < 900 and medmedsprd >= .00008 and medmedsprd<.04)  "
//				"and uniqueID IS NOT NULL " 
//				"and sectype = 'F' " 
//				" ORDER BY uniqueID ",
//				(int)xpDate.Date(), (int)spDate.Date()); 
//
//	else if(ei.model==116 )
//		sprintf(sqlCmd,"SELECT DISTINCT(uniqueID) "
//				"FROM stockCharacteristics "
//				"WHERE idate <= %d and idate >= %d and "
//				" ( close_ > .5 and "
//				"close_ * medVolume > 36000000 and market = 'N' and "
//				"medVolatility > .005 and close_ < 900 and medmedsprd >= .00008 and medmedsprd<.04)  "
//				"and uniqueID IS NOT NULL " 
//				"and sectype != 'P' "
//				"and sectype != 'F' " 
//				"and sectype != 'X' " 
//				"union "
//				"SELECT DISTINCT(uniqueID) "
//				"FROM stockCharacteristics "
//				"WHERE idate <= %d and idate >= %d and "
//				" ( close_ > .5 and "
//				"close_ * medVolume > 15000000 and market != 'N' and "
//				"medVolatility > .005 and close_ < 900 and medmedsprd >= .00008 and medmedsprd<.04)  "
//				"and uniqueID IS NOT NULL " 
//				"and sectype != 'P' "
//				"and sectype != 'F' " 
//				"and sectype != 'X' " 
//				" ORDER BY uniqueID ",
//				(int)xpDate.Date(), (int)spDate.Date()); 
//
//	else if(ei.model==130)
//		sprintf(sqlCmd,"SELECT DISTINCT(uniqueID) "
//				"FROM stockCharacteristics "
//				"WHERE idate <= %d and idate >= %d "
//				"and close_ >= 0.1 and close_ * medVolume >= 50000 and medNTrades >= 20 "
//				"and medMedSprd <= .03 and medvolatility > 0 and secType = 'W' "
//				"and uniqueID IS NOT NULL "
//				"and ( %s ) "
//				" ORDER BY uniqueID ",
//				(int)xpDate.Date(), (int)spDate.Date(),mkt.c_str());
//	else 
//		sprintf(sqlCmd,"SELECT DISTINCT(uniqueID) "
//				"FROM stockCharacteristics "
//				"WHERE idate <= %d and idate >= %d and inUniverse = 1  and uniqueID IS NOT NULL "
//				"and ( %s ) "
//				" ORDER BY uniqueID ",
//				(int)xpDate.Date(), (int)spDate.Date(),mkt.c_str());
//
//
//}
//
//void makeSQLCommand::getUniverse(ExchangeInfo ei,int edate)
//{
//	string mkt=(string)" market = '" + ei.market + (string)"'";
//	if(ei.model==101 || ei.model==115 || ei.model==116)
//		mkt = "(market = 'Q' or market != 'Q') ";
//	else if(ei.model == 107)
//	{
//		mkt = " market IN ('A','B','F','I','P','Z','X','C','W','Y','M','D','O','L')  ";	
//	}
//	else if(ei.model==114)
//	{
//		mkt = " market IN ('K','Q')  ";
//	}
//
//	QuoteTime fDate(edate,120000,ei.timeZone);    // fit date; date of tick data, signals etc  
//	QuoteTime pDate=ei.ex->PrevClose(fDate);    // previous trade date; from which get volume, volat, universe, close, etc
//
//	if(ei.model==101) 
//		sprintf(sqlCmd,"SELECT uniqueID, medVolume, medVolatility, close_, "
//				"inUniverse, medmedSprd, nTrades, nQuotes , volume, volatility, market, ticker, secType "
//				"FROM stockCharacteristics "
//				"WHERE idate = %d and %s and uniqueID IS NOT NULL "
//				"ORDER BY uniqueID ",
//				(int)pDate.Date(), mkt.c_str());
//
//	else if(ei.model==115) 
//		sprintf(sqlCmd,"SELECT uniqueID, medVolume, medVolatility, close_, "
//				"inUniverse, medmedSprd, nTrades, nQuotes , volume, volatility, market, ticker, secType "
//				"FROM stockCharacteristics "
//				"WHERE idate = %d and %s and uniqueID IS NOT NULL and secType = 'F' "
//				"ORDER BY uniqueID ",
//				(int)pDate.Date(), mkt.c_str());
//
//	else if(ei.model==116) 
//		sprintf(sqlCmd,"SELECT uniqueID, medVolume, medVolatility, close_, "
//				"inUniverse, medmedSprd, nTrades, nQuotes , volume, volatility, market, ticker, secType "
//				"FROM stockCharacteristics "
//				"WHERE idate = %d and %s and uniqueID IS NOT NULL "
//				"ORDER BY uniqueID ",
//				(int)pDate.Date(), mkt.c_str());
//
//	else if(ei.model==105)   // Toronto; has "ticker" rather than "symbol"
//		sprintf(sqlCmd,"SELECT uniqueID, medVolume, medVolatility, close_,  "
//				" inUniverse, medmedSprd, nTrades, nQuotes, volume, volatility, market, ticker "  // 20080109
//				"FROM stockCharacteristics "
//				"WHERE idate = %d  and uniqueID IS NOT NULL "
//				" ORDER BY uniqueID ",
//				 (int)pDate.Date());
//
//	else
//		sprintf(sqlCmd,"SELECT uniqueID, medVolume, medVolatility, close_,  "
//					" inUniverse, medmedSprd, nTrades, nQuotes, volume, volatility, market, symbol  " // 20080109
//					"FROM stockCharacteristics "
//					"WHERE idate = %d  and uniqueID IS NOT NULL "
//					"and ( %s ) "
//					" ORDER BY uniqueID ",
//					(int)pDate.Date(),mkt.c_str());
//}
//
//void makeSQLCommand::getStCharHLAdj(ExchangeInfo ei,int idate)
//{
//	string mkt = " (market = 'Q' or market != 'Q'  ) ";
//	if(ei.model == 109 || ei.model == 110 || ei.model == 112 || ei.model == 113 || ei.model == 117
//		|| ei.model == 118 || ei.model == 119 || ei.model == 130)
//		mkt = (string)"market = '" + ei.market + (string)"'";
//	else if(ei.model == 107)
//	{
//		mkt = " market IN ('A','B','F','I','P','Z','X','C','W','Y','M','D','O','L')  ";	
//	}
//	else if(ei.model == 114)
//	{
//		mkt = " market IN ('K','Q')  ";
//	}
//
//	if(ei.model<=105 || ei.model == 116)
//		sprintf(sqlCmd,"SELECT uniqueID, adjust, close_ , open_, high, low "
//			"FROM stockCharacteristics "
//			"WHERE idate = %d and %s and uniqueID IS NOT NULL "
//			"ORDER BY uniqueID ",
//			idate, mkt.c_str());
//	else if(ei.model==115)
//		sprintf(sqlCmd,"SELECT uniqueID, adjust, close_ , open_, high, low "
//			"FROM stockCharacteristics "
//			"WHERE idate = %d and %s and uniqueID IS NOT NULL and secType = 'F' "
//			"ORDER BY uniqueID ",
//			idate, mkt.c_str());
//	else 
//		sprintf(sqlCmd,"SELECT uniqueID, adjust, close_ , open_, high, low, tickValid "
//			"FROM stockCharacteristics "
//			"WHERE idate = %d and %s and uniqueID IS NOT NULL "
//			"ORDER BY uniqueID ",
//			idate, mkt.c_str());
//}
//
//void makeSQLCommand::getStCharHLAdj(ExchangeInfo ei,vector<int> & idate)
//{
//	if(ei.country.size()!=idate.size())
//	{
//		cout << "ei.country.size()!=idate.size()" << endl;
//		return;
//	}
//
//	string cmd; 
//	for(int u=0;u<idate.size();u++)
//	{
//			sprintf(sqlCmd,"SELECT uniqueID, adjust, close_ , open_, high, low, tickValid "
//			"FROM stockCharacteristics "
//			"WHERE idate = %d and market = '%s' and uniqueID IS NOT NULL ",
//			idate[u], ei.exchanges[u].c_str());
//
//		cmd += (string)sqlCmd;
//		if(u < idate.size()-1)
//			cmd += (string)" UNION ALL "; 
//	}
//	cmd += (string)" ORDER BY uniqueID "; 
//	sprintf(sqlCmd,"%s",
//		cmd.c_str());
////	cout << sqlCmd << endl; 
//}
////// dataHandler 
//void dataHandler::initMatrixShort(MATRIXSHORT &data)
//{
//	data.cols=0;
//	data.elem=NULL;
//	data.elemF=NULL;
//	data.rowlabel=NULL;
//	data.rows=0;
//	data.rpartSorts=NULL;
//	data.rpartWhich=NULL;
//	data.target=NULL;
//}

void dataHandler::getDataDirs(string binmodel,string bindir,string wmodel,int imarket)
{
	binModel=binmodel;
	binDir=bindir;
	wModel=wmodel;

	if(imarket==101||imarket==116) market = market2 = "U";
	else if(imarket==115) market = market2 = "F";
	else if(imarket==105) market=market2 ="J";
	else if(imarket==107) market =market2 = "E";
	else if(imarket==109) 
	{
		string mkt  = bindir.substr(bindir.length()-2,1);
		string mkt2 = bindir.substr(bindir.length()-3,2);
		market=mkt;
		market2 =mkt2;
	}
	else if(imarket==110) market =market2 = "T"; 
	else if(imarket==112) market =market2 = "H";
	else if(imarket==113) market =market2 = "S";
	else if(imarket==114) market =market2 = "K";
	else if(imarket==117) { market ="J"; market2 = "MJ";}
	else if(imarket==118) { market ="G"; market2 = "G";}
	else if(imarket==119) { market ="W"; market2 = "W";}
	else if(imarket==130) { market ="H"; market2 = "H";}
}

void dataHandler::getFitDates(int udate, int nfitdays)
{
	uDate=udate;
	nFitDays=nfitdays;
	fitDates.resize(nFitDays);
//	getValidDates(uDate,nFitDays,&(fitDates[0]),market);
	//JL getValidDates(uDate,nFitDays,&(fitDates[0]),market2);

	return;
}

void dataHandler::getTestDates(int lasttestdate)
{
	// get dates >= nextTrDay and < lastTestDate to do the prediction
	//lastTestDate=lasttestdate;

	//if(lastTestDate < uDate) 
	//	lastTestDate = uDate; // (int)ex.NextOpen(QuoteTime(uDate,120000,timeZone)).Date();

	//getValidDatesSdEd(lastTestDate,uDate,&nTestDays,&nTestDays,0,market2);
	//if(nTestDays>0)
	//{
	//	testDates.resize(nTestDays);
	//	getValidDatesSdEd(lastTestDate,uDate,&nTestDays,&(testDates[0]),1,market2);
	//}
	return;
}

void dataHandler::getNormDates(int nnormdays)
{
	if(fitDates.size()>0)
	{
		nNormDays=nnormdays;
		normDates.resize(nNormDays);
		//getValidDates(fitDates[0],nNormDays,&(normDates[0]),0);
	}
	return;
}

int dataHandler::getNRowsData(vector<int> &dates)
{
	//sigFInOut sigWr;
	int nrows=0;
	//for(int n=0;n<(int)dates.size();n++)
	//{
	//	int fok=0,fileRows=0,tfileCols=0,fileRowsSub=0; 
	//	fok = sigWr.gSigBinaryFileSize(dates[n],binModel,market,binDir,&fileRows,&tfileCols);
	//	if(fok==0)
	//		continue;
	//	if(fileColCount==0)
	//	{
	//		fileCols=tfileCols;
	//		++fileColCount; 
	//	}
	//	nrows += fileRows;
	//	if( fileRows > maxFileRows )
	//		maxFileRows = fileRows; 
	//}
	return nrows;
}

void dataHandler::selectDataRows(int nsigs,vector<long> &Sflags)
{	// this is a bad method 
	s.resize(nsigs+3);
	if((binModel=="om" || binModel=="ompb") )
	{
		for(int i=0;i<9;i++)
			s[i]=i;
		s[nsigs]=9; // residual target

		s[9]  = 18;
		s[10] = 19;
		s[11] = 20;

if(Sflags[13]==12 || Sflags[13]==21 || Sflags[13]==41|| Sflags[13]==42 ||  Sflags[13]==43 
    || Sflags[13]==44  || Sflags[13]==45 || Sflags[13]==46 || Sflags[13]==47|| Sflags[13]==48|| Sflags[13]==49 
	|| Sflags[13]==51 || Sflags[13]==54|| Sflags[13]==55) // 23 signal model
{
			s[12]=11;
			s[13]= 13; // 12; //
			s[14]=14;
			s[15]=15;
			s[16]= 17;
			s[17]= 21; // 16; //
			s[18]=22;

			s[19]=29;
			s[20]=30;
			s[21]= 31; // 23; //
			s[22]= 32; // 24; //
}
if(Sflags[13]==41)
{
			s[23]=37;
			s[24]= 38; // 12; //
			s[25]=39;
			s[26]=40;
			s[27]= 41; // 16; //
			s[28]=42;
			s[29]=43;
			s[30]=44;
			s[31]= 45; // 23; //
			s[32]= 46; 
			s[33]= 47; 
			s[34]= 48; 
}
if(Sflags[13]==42 || Sflags[13]==43 || Sflags[13]==44 || Sflags[13]==45 || 
   Sflags[13]==45|| Sflags[13]==46|| Sflags[13]==47|| Sflags[13]==48 || Sflags[13]==49 
   || Sflags[13]==51 || Sflags[13]==54|| Sflags[13]==55)
{
			s[8] =  37; // 8   ret5
			s[19]=  38; // 29  ret15
			s[12] = 39;  // 11  ret30

			s[23]= 40;
			s[24]= 41; 
			s[25] = 44;
			s[26] =45;
			s[27] = 47; // ret
			s[28]= 48; // g
}
if(Sflags[13]==44|| Sflags[13]==45) // ETF model
{
			s[29]= 49;
			s[30]= 50; 
			s[31] = 51;
			s[32] =52;
			s[33] = 53; // ret
			s[34]= 54; // g
			s[35]= 55; // g
}
if(Sflags[13]==46)
{
			s[29]= 62;
			s[30]= 63; 		
}
if(Sflags[13]==47 || Sflags[13]==48)
{
			s[29]= 61;  // fillImb
			s[30]= 62;  // Idx
			s[31] = 63;  // fut
			s[nsigs]=68; // real target
}
if(Sflags[13]==49)
{
			s[29]= 61;  // fillImb
			s[30]= 62;  // Idx
			s[31] = 63;  // fut

}
if(Sflags[13]==51)
{
			s[29]= 11;  // fillImb
			s[30]= 12;  // Idx
			s[31] = 16;
}
if(Sflags[13]==54)
{
			s[29]= 62;  // vwap
}
if(Sflags[13]==55)
{
			s[29]= 64;  // vwap
			s[30]= 65;  // vwap
			s[31]= 66;  // vwap
			s[32]= 67;  // vwap
			s[33]= 68;  // vwap
}
	}
	else if(binModel=="opt" && wModel=="treeBoost") // last three are target, bm pred, rbf pred
	{
		for(int i=0;i<nsigs+1;i++)
			s[i]=i;    
//		s[nsigs+1]=nsigs+1; // sprd 
		// no bm pred yet  
	}
	else if( binModel.substr(0,2)=="tm" && (wModel=="nNet" || wModel == "linMod"))
	{
		for(int i=0;i<21;i++)
			s[i]=i;
		s[nsigs-1]= 23;// sprd 
		s[nsigs]=21;   // target 
 		s[nsigs+1]=23; // sprd; 
		s[nsigs+2]=22; // bm pred;
	}
	else if(research==27 && binModel=="tm" && wModel=="treeBoost")
	{
		s[0]=14;
		s[1]=17;
		s[2]=18;
		s[3]=19;
		s[4]=20;   // tob

		s[5]=30;
		s[6]=31;
		s[7]=32;
		s[8]=33;   // qrat

		for(int z=9;z<34;z++)
			s[z]=42+z;

		s[nsigs]=22;
		s[nsigs+1]=21; // sprd; 
		s[nsigs+2]=23; // bm pred;
	}
	else if(binModel=="tm" && wModel=="treeBoost")
	{
		for(int i=0;i<22;i++) // 0:21 are our standard 22 sigs; 22 is the target
			s[i]=i;

		s[22]=34; // ex 
		s[23]=35;  // nf1
		s[24]=36; //nf2 
		s[25]=37; //nf3
		s[26]=38; 
		s[27]=39; 
		s[28]=40; 
		s[29]=41; 
		s[30]=42; 
		s[31]=43; 
		s[32]=44; 
		s[33]=45; 

		s[34]=47;  // is F
		s[35]=50; // sprd open

		if(Sflags[13] >= 30 )
		{
			s[36]=55; // 
			s[37]=61;
			s[38]=62;
			s[39]=63; //
			s[40]=64;
			s[41]=65;
			
			s[42]=30;
			s[43]=31;
			s[44]=32;
			s[45]=33;
			s[46]=51; // 
			s[47]=52;
			s[48]=53;
			s[49]=54;

			s[50]=66;
			s[51]=67;
			s[52]=68;
			s[53]=69;
			s[54]=70;
			s[55]=71;
			s[56]=72;
			s[57]=24; // 
			s[58]=25;
			s[59]=26;
		}	
		if(Sflags[13] >= 30 && nsigs >=70 )
		{
			s[60]=78;
			s[61]=79;
			s[62]=80;
			s[63]=81;
			s[64]=82;
			s[65]=83;
			s[66]=84;
			s[67]=85;
			s[68]=86;
			s[69]=87;
	}
	if(Sflags[13] == 36) // get rid of TOB and weak signals  
	{
		s[17]=78;
		s[18]=79;
		s[19]=80;
		s[20]=81;
		s[34]=82;
		s[41]=83;
		s[47]=84;
		s[50]=85;
		s[60]=86;
		s[61]=87;
	}
	if(Sflags[13] == 37)  // dollar volume
	{
		s[1] = 76;
		s[2] = 77;
	}
	if(Sflags[13] == 38)
	{
		s[70] = 95;
		s[71] = 96;
		s[72] = 97;
		s[73] = 98;
		s[74] = 99;
		s[75] = 100;
		s[76] = 101;
		s[77] = 29;
	}
	
	if(Sflags[13] == 39) // dollar volume + mintick 
	{
		s[1] = 76;
		s[2] = 77;
		s[70] = 75;
	}
	if(Sflags[13] == 40)
	{
		s[70]=97;
		s[71]=98;
		s[72]=99;
		s[73]=101;
	}	
	if(Sflags[13] == 41)
	{
		s[70]=97;
		s[71]=98;
		s[72]=99;
		s[73]=101;
		s[74]=102;
		s[75]=103;
		s[76]=104;
		s[77]=105;
		s[78]=106;
	}	
	if(Sflags[13] == 42)
	{
		s[70]=90;
	}
	if(Sflags[13] == 43)
	{
		s[70]=91;
	}
	if(Sflags[13] == 44)
	{
		s[70]=92;
	}
	if(Sflags[13] == 45)
	{
		s[70]=91;s[71]=92;
	}
	if(Sflags[13] == 50)
	{
		s[70] = 88;
		s[71] = 89;
	}
	if(Sflags[13] == 52)
	{
		s[70] = 29;
	}

		s[nsigs]=22;
		s[nsigs+1]=21; // sprd; 
		s[nsigs+2]=23; // bm pred;

		if(Sflags[13] == 34) 
			s[nsigs]=49;
		if(Sflags[13] == 35)
			s[nsigs]=60;		
	}

	else if( binModel=="pnl" && wModel=="nNet")
	{
		for(int i=0;i<nsigs+1;i++) // 0:21 are our standard 22 sigs; 22 is the target
			 s[i]=i;
	}
	else if( binModel=="vol")
	{
		for(int i=0;i<nsigs;i++) // 0:21 are our standard 22 sigs; 22 is the target
			 s[i]=i+2;
		s[nsigs]=22;
	}
	else return;
}

void dataHandler::selectDataRowsAux(int nsigs,vector<long> &Sflags)
{
	nAuxCols=nsigs;	
	saux.resize(nAuxCols);
	if(binModel=="opt" && wModel=="treeBoost")
	{
		saux[0]=32;saux[1]=26;saux[2]=26;saux[3]=27; // tar, bmpred, treepred, sprd; 	
	}
	else if(binModel == "om" && wModel=="treeBoost")
	{
		saux[0]=9;saux[1]=10;saux[2]=10;saux[3]=1; 
		if(nAuxCols==5)
			saux[4]=18; // exchange
		if(Sflags[13] == 47 || Sflags[13]==48) // pure t b 
			saux[0]=68;
// WIST
		if(Sflags[13]==43 && Sflags[22]==1)
			saux[0]=58;
	}
	else if(binModel == "ompb" && wModel=="treeBoost")
	{
		saux[0]=21;saux[1]=22;saux[2]=22;saux[3]=1; 			
	}
	else if( binModel.substr(0,2)=="tm" && wModel=="treeBoost")
	{
		saux[0]=22;saux[1]=23;saux[2]=23;saux[3]=21; // for the new files hedged
		if(Sflags[13] == 34) 
			saux[0]=49;
		if(Sflags[13] == 35)
			saux[0]=60;
		if(saux.size()>5)
		{
			saux[4]=48; saux[5]=49; // the two psuedotargets 
			saux[6]=34;
			saux[7]=47;  // sectype 
		}
		if(research == 17) //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		{
			saux[0]=74;saux[1]=73;saux[2]=73;saux[3]=21; // tar, bmpred, treepred, sprd; 			
			
		}
	}
	else if( binModel.substr(0,2)=="ex" && wModel=="treeBoost")
	{
		saux[0]=22;saux[1]=23;saux[2]=23;saux[3]=21; // for the new files hedged
		saux[4]=48; saux[5]=49; // the two psuedotargets 
		saux[6]=34;
		saux[7]=47; // is F
	}
	else if( binModel.substr(0,2)=="tm" && wModel=="linMod")
	{
		saux[0]=21;saux[1]=22;saux[2]=22;saux[3]=23; 	
	}
	else if( binModel=="vol")
	{
		saux[0]=22;saux[1]=22;saux[2]=22;saux[3]=22; // everything equal to target 		
	}
	else return;
}

void dataHandler::getDataTranspose(MATRIXSHORT &data, int nsigs, int npts, vector<int> &dates, vector<int> &ws)
{
	//sigFInOut sigWr;

	//data.rows=nsigs+1; // Notice: we want columns to be observations, not rows
	//data.elemF = fmatcalloc(data.rows, npts);
 //	data.cols = npts;
	//data.rpartSorts = NULL;  // memory allocated in sorting routine 
	//data.rpartWhich = lvcalloc(npts);
	//data.target = NULL;   // if this is not NULL, then it is used in the tree fit
	//data.elem   = NULL;    // if this is NULL, use the elemF
	//data.rowlabel = (char **) calloc(data.rows, sizeof(char *));
	//for(int i=0;i<data.rows;i++)
	//	data.rowlabel[i]=(char*)calloc(20,sizeof(char)); 

	//int countRows=0;
	//float *tstv;
	//tstv = fvcalloc(maxFileRows*(fileCols));	// corresponding to the largest data set
	//for(int n=0;n<(int)dates.size();n++) 
	//{
	//	int fok=0,tfrows=0,tfcols=0;
	//	fok = sigWr.gSigBinaryFileSize(dates[n],binModel,market,binDir,&tfrows,&tfcols);
	//	if(fok == 0 || tfrows==0 || tfcols != fileCols)
	//		continue; 
	//	// switch rows and columns for reading  
	//	sigWr.rSigBinary(tstv,colLabels,tfrows,fileCols,dates[n],binModel,market,binDir);
	//	if(n==0)
	//	{
	//		for(int j=0;j<data.rows;j++)
	//			strncpy(data.rowlabel[j],colLabels[ws[j]].c_str(),19);
	//	}
	//	for(int u=0;u<data.rows;u++)
	//		for(int v=0;v<tfrows;v++)
	//		{
	//			data.elemF[u][v+countRows] = tstv[v*fileCols + ws[u]];
	//		}
	//	countRows+=tfrows; 
	//}
	//free(tstv); 
	return;
}
// simple conditional case - just use data where the value in row conditionRow equals conditionValue;
void dataHandler::getDataTransposeCond(MATRIXSHORT &data,int nsigs,int npts,vector<int> &dates,vector<int> &ws,
			int conditionRow, float conditionValue)
{
	//sigFInOut sigWr;

	//data.rows=nsigs+1; // Notice: we want columns to be observations, not rows
	//data.elemF = fmatcalloc(data.rows, npts);
 //	data.cols = npts;
	//data.rpartSorts = NULL;  // memory allocated in sorting routine 
	//data.rpartWhich = lvcalloc(npts);
	//data.target = NULL;   // if this is not NULL, then it is used in the tree fit
	//data.elem   = NULL;    // if this is NULL, use the elemF
	//data.rowlabel = (char **) calloc(data.rows, sizeof(char *));
	//for(int i=0;i<data.rows;i++)
	//	data.rowlabel[i]=(char*)calloc(20,sizeof(char)); 

	//int countRows=0;
	//float *tstv;
	//tstv = fvcalloc(maxFileRows*(fileCols));	// corresponding to the largest data set

	//vector<float> condCol;
	//condCol.resize(maxFileRows);

	//for(int n=0;n<(int)dates.size();n++) 
	//{
	//	int fok=0,tfrows=0,tfcols=0;
	//	fok = sigWr.gSigBinaryFileSize(dates[n],binModel,market,binDir,&tfrows,&tfcols);
	//	if(fok == 0 || tfrows==0 || tfcols != fileCols)
	//		continue; 
	//	
	//	// switch rows and columns for reading  
	//	sigWr.rSigBinary(tstv,colLabels,tfrows,fileCols,dates[n],binModel,market,binDir);
	//	if(n==0)
	//	{
	//		for(int j=0;j<data.rows;j++)
	//			strncpy(data.rowlabel[j],colLabels[ws[j]].c_str(),19);
	//	}
	//	for(int u=0;u<data.rows;u++)
	//		for(int v=0;v<tfrows;v++)
	//		{
	//			data.elemF[u][v+countRows] = tstv[v*fileCols + ws[u]];
	//		}
	//	countRows+=tfrows; 
	//}
	//free(tstv); 
	return;
}


// this is for cases when we want to combine several exchanges like P and A 
void dataHandler::getDataTranspose(MATRIXSHORT &data, bool allocMem, int nsigs, int npts, vector<int> &dates, vector<int> &ws, int &countRows)
{
//	sigFInOut sigWr;
//	if(allocMem)
//	{
//		data.rows=nsigs+1; // Notice: we want columns to be observations, not rows
//		data.elemF = fmatcalloc(data.rows, npts);
// 		data.cols = npts;
//		data.rpartSorts = NULL;  // memory allocated in sorting routine 
//		data.rpartWhich = lvcalloc(npts);
//		data.target = NULL;   // if this is not NULL, then it is used in the tree fit
//		data.elem   = NULL;    // if this is NULL, use the elemF
//		data.rowlabel = (char **) calloc(data.rows, sizeof(char *));
//		for(int i=0;i<data.rows;i++)
//			data.rowlabel[i]=(char*)calloc(20,sizeof(char)); 
//	}
//
////	int countRows=0;
//	float *tstv;
//	tstv = fvcalloc(maxFileRows*(fileCols));	// corresponding to the largest data set
//	for(int n=0;n<(int)dates.size();n++) 
//	{
//		int fok=0,tfrows=0,tfcols=0;
//		fok = sigWr.gSigBinaryFileSize(dates[n],binModel,market,binDir,&tfrows,&tfcols);
//		if(fok == 0 || tfrows==0 || tfcols != fileCols)
//			continue; 
//		// switch rows and columns for reading  
//		sigWr.rSigBinary(tstv,colLabels,tfrows,fileCols,dates[n],binModel,market,binDir);
//		if(n==0)
//		{
//			for(int j=0;j<data.rows;j++)
//				strncpy(data.rowlabel[j],colLabels[ws[j]].c_str(),19);
//		}
//		for(int u=0;u<data.rows;u++)
//			for(int v=0;v<tfrows;v++)
//				data.elemF[u][v+countRows] = tstv[v*fileCols + ws[u]];
//
//		countRows+=tfrows; 
//	}
//	free(tstv); 
	return;
}

void dataHandler::getData(MATRIXSHORT &data, int nsigs,int npts,vector<int> &dates)
{
	//sigFInOut sigWr;

	//data.cols = nsigs+3;  // getting sprd info as well 
	//data.elemF = fmatcalloc(npts,(int)data.cols); // nsigs+fulltarget+
	//data.rows = npts;
	//data.rpartSorts = NULL;  // not needed except by trees 
	//data.rpartWhich = NULL;
	//data.target = NULL;    // if this is not NULL, then it is used in the tree fit;
	//data.elem   = NULL;    // if this is NULL, use the elemF
	//data.rowlabel = NULL;

	//int countRows=0;
	//float *tstv;
	//tstv = fvcalloc(maxFileRows*(fileCols));	// corresponding to the largest file
	//for(int n=0;n<(int)dates.size();n++) 
	//{
	//	int fok=0,tfrows=0,tfcols=0;
	//	fok = sigWr.gSigBinaryFileSize(dates[n],binModel,market,binDir,&tfrows,&tfcols);
	//	if(fok == 0 || tfrows==0 || tfcols != fileCols)
	//		continue;

	//	sigWr.rSigBinary(tstv,colLabels,tfrows,fileCols,dates[n],binModel,market,binDir);
	//	for(int u=0;u<tfrows;u++)
	//	{
	//		for(int v=0;v<(int)data.cols;v++)
	//			data.elemF[u+countRows][v] = tstv[u*fileCols + s[v]];	
	//	}
	//	countRows+=tfrows; 
	//}
	//free(tstv); 
	return;
}

void dataHandler::getDataBootStrap(MATRIXSHORT &data, long seed)
{
	MATRIXSHORT bdata;
	int N=data.rows;
	bdata.elemF = fmatcalloc(N,data.cols); // nsigs+fulltarget+bmpred+treePred
	bdata.cols = data.cols;    
	bdata.rows = N;
	bdata.rpartSorts = NULL;  // not needed except by trees 
	bdata.rpartWhich = NULL;
	bdata.target = NULL;    // if this is not NULL, then it is used in the tree fit;
	bdata.elem   = NULL;    // if this is NULL, use the elemF
	bdata.rowlabel = NULL;	
	
	long tseed=seed+1;
	vector<int> sample;
	sample.resize(N);
	for(int i=0;i<N;i++)
	{
		sample[i]= floor(N*ran0(&tseed));
		if(sample[i]>=N)
			sample[i]=N-1;
		if(sample[i]<=0)
			sample[i]=0;
	}

	for(int i=0;i<N;i++)
		for(int j=0;j<data.cols;j++)
			bdata.elemF[i][j]=data.elemF[sample[i]][j];
	
	for(int i=0;i<N;i++)
		for(int j=0;j<data.cols;j++)
			data.elemF[i][j]=bdata.elemF[i][j];

	freeData(bdata);
	return;
}

void dataHandler::sortDataRows(MATRIXSHORT &data, long nThreads)
{	// also allocates memory for dataFit.rpartSorts
	if(nThreads<2)
		MatrixShortSort(&data);
	else
		MatrixShortSortMT(&data, nThreads);
}

void dataHandler::freeData(MATRIXSHORT &data) // should clean this up and consolidate 
{
	if(data.elemF != NULL)
		fmatfree(data.elemF,data.rows);
	if(data.elem != NULL)
		shortmatfree(data.elem,data.rows);
	if(data.rpartSorts != NULL)
		longmatfree(data.rpartSorts,data.rows-1); //the target is not sorted ; should check to see that this is allocated 
	if(data.rpartWhich != NULL)
		free(data.rpartWhich);
	if(data.rowlabel!= NULL)
	{
		// if(binModel!="opt")   
		if(binModel.substr(0,3)!="opt" && binModel.substr(0,3)!="vol")
			for(int i=0;i<data.rows;i++)
				free(data.rowlabel[i]);
		free(data.rowlabel); 
	}
	if(data.target != NULL)
		free(data.target);
	
}

//void dataHandler::selectBySprd(const MATRIXSHORT &rdata,bool isTranspose, const int sprdRow, const int minSprd, const int maxSprd,MATRIXSHORT &data)
void dataHandler::getDataSubsetTranspose(MATRIXSHORT &dataFit, MATRIXSHORT &dataIdx, MATRIXSHORT &dataSub, int dataIdxValue)
{
	// if we want to do tight and narrow sprds, figure out tight and narrow rows 
//	if(dataAux.rows == 0 || dataAux.cols == 0)
//		return;

//	int nrows=0;
//	int ncols=data.rows;

	//for(int i=0;i<data.cols;i++)
	//{
	//	if(data.elemF[subCol][i]==subColValue)
	//		nrows++;
	//}

//	if(nrows==0)
//		return;
//
////	data.cols=ncols-1; // don't want to keep sprd info
//	data.cols=ncols; // don't want to keep sprd info
//	data.rows=nrows;
//	data.elemF=fmatcalloc(nrows,ncols);
//	
//	if(isTranspose)
//	{
//		int count=0;
//		for(int i=0;i<rdata.cols;i++)
//		{
//			if(rdata.elemF[sprdRow][i]>=minSprd && rdata.elemF[sprdRow][i] < maxSprd)
//			{
//				for(int j=0;j<data.cols;j++)		
//					data.elemF[count][j]=rdata.elemF[j][i];
//				++count;
//			}
//		}
//	}
//	else
//	{
//		int count=0;
//		for(int i=0;i<rdata.rows;i++)
//		{
//			if(rdata.elemF[i][sprdRow]>=minSprd && rdata.elemF[i][sprdRow] < maxSprd)
//			{
//				for(int j=0;j<data.cols;j++)		
//					data.elemF[count][j]=rdata.elemF[i][j];
//				++count;
//			}
//		}
//	}
	return;
}


void dataHandler::selectBySprd(const MATRIXSHORT &rdata,bool isTranspose, const int sprdRow, const int minSprd, const int maxSprd,MATRIXSHORT &data)
{
	// if we want to do tight and narrow sprds, figure out tight and narrow rows 
	int nrows=0;
	int ncols=0;
	if(isTranspose)
	{
		ncols  = rdata.rows;
		for(int i=0;i<rdata.cols;i++)
		{
			if(rdata.elemF[sprdRow][i]>=minSprd  && rdata.elemF[sprdRow][i] < maxSprd)
				nrows++;
		}
	}
	else
	{
		ncols  = rdata.cols;
		for(int i=0;i<rdata.rows;i++)
		{
			if(rdata.elemF[i][sprdRow]>=minSprd  && rdata.elemF[i][sprdRow] < maxSprd)
				nrows++;
		}		
	}
	if(nrows==0)
		return;

//	data.cols=ncols-1; // don't want to keep sprd info
	data.cols=ncols; // don't want to keep sprd info
	data.rows=nrows;
	data.elemF=fmatcalloc(nrows,ncols);
	
	if(isTranspose)
	{
		int count=0;
		for(int i=0;i<rdata.cols;i++)
		{
			if(rdata.elemF[sprdRow][i]>=minSprd && rdata.elemF[sprdRow][i] < maxSprd)
			{
				for(int j=0;j<data.cols;j++)		
					data.elemF[count][j]=rdata.elemF[j][i];
				++count;
			}
		}
	}
	else
	{
		int count=0;
		for(int i=0;i<rdata.rows;i++)
		{
			if(rdata.elemF[i][sprdRow]>=minSprd && rdata.elemF[i][sprdRow] < maxSprd)
			{
				for(int j=0;j<data.cols;j++)		
					data.elemF[count][j]=rdata.elemF[i][j];
				++count;
			}
		}
	}
	return;
}


void dataHandler::selectBySprdNQ(const MATRIXSHORT &rdata,bool isTranspose,const int sprdRow, 
		const int minSprd, const int maxSprd,MATRIXSHORT &data, int nqrow, float nynasd)
{
	// if we want to do tight and narrow sprds, figure out tight and narrow rows 
	int nrows=0;
	int ncols=0;
	if(isTranspose)
	{
		ncols  = rdata.rows;
		for(int i=0;i<rdata.cols;i++)
		{
			if( (rdata.elemF[sprdRow][i]>=minSprd  && rdata.elemF[sprdRow][i] < maxSprd) && rdata.elemF[nqrow][i]==nynasd )
				nrows++;
		}
	}
	else
	{
		ncols  = rdata.cols;
		for(int i=0;i<rdata.rows;i++)
		{
			if((rdata.elemF[i][sprdRow]>=minSprd  && rdata.elemF[i][sprdRow] < maxSprd) && rdata.elemF[i][nqrow]==nynasd)
				nrows++;
		}		
	}
	if(nrows==0)
		return;

//	data.cols=ncols-1; // don't want to keep sprd info
	data.cols=ncols; // don't want to keep sprd info
	data.rows=nrows;
	data.elemF=fmatcalloc(nrows,ncols);
	
	if(isTranspose)
	{
		int count=0;
		for(int i=0;i<rdata.cols;i++)
		{
			if((rdata.elemF[sprdRow][i]>=minSprd && rdata.elemF[sprdRow][i] < maxSprd) && rdata.elemF[nqrow][i]==nynasd)
			{
				for(int j=0;j<data.cols;j++)		
					data.elemF[count][j]=rdata.elemF[j][i];
				++count;
			}
		}
	}
	else
	{
		int count=0;
		for(int i=0;i<rdata.rows;i++)
		{
			if((rdata.elemF[i][sprdRow]>=minSprd && rdata.elemF[i][sprdRow] < maxSprd) && rdata.elemF[i][nqrow]==nynasd)
			{
				for(int j=0;j<data.cols;j++)		
					data.elemF[count][j]=rdata.elemF[i][j];
				++count;
			}
		}
	}
	return;
}

void dataHandler::bin(const MATRIXSHORT &rdata,bool isTranspose, int col, vector<double> &bins,vector<dataObject> &data)
{
	// col is column to bin on
	// bins are limits, length is n+1
	// data are binned objects
//	int ncols = 3 ; // number of columns that we want to keep: should make this dynamic 
	assert(bins.size()==data.size()+1);
	int ncols = 0;

	if(isTranspose)
	{
ncols = rdata.rows;		
		for(int i=0;i<rdata.cols;i++)
		{
			for(int j=0;j<(int)data.size();j++)
			{
				if(rdata.elemF[col][i]>=bins[j] && rdata.elemF[col][i] < bins[j+1] )
					++data[j].mat.rows;
			}
		}
	}
	else
	{
ncols = rdata.cols;		
		for(int i=0;i<rdata.rows;i++)
		{
			for(int j=0;j<(int)data.size();j++)
			{
				if(rdata.elemF[i][col]>=bins[j] && rdata.elemF[i][col] < bins[j+1] )
					++data[j].mat.rows;
			}
		}		
	}

	// allocate memory
	for(int j=0;j<(int)data.size();j++)
	{
		data[j].mat.cols=ncols;
		if(data[j].mat.rows>0)
			data[j].mat.elemF=fmatcalloc(data[j].mat.rows,ncols);
	}

	vector<int> count;
	count.resize(data.size());
	if(isTranspose)
	{
		for(int i=0;i<rdata.cols;i++)
		{
			for(int j=0;j<(int)data.size();j++)
			{
				if(rdata.elemF[col][i]>=bins[j] && rdata.elemF[col][i] < bins[j+1] )
				{
					for(int k=0;k<ncols;k++)
						data[j].mat.elemF[count[j]][k]=rdata.elemF[k][i];
					++count[j];
				}
			}				
		}
	}
	else
	{
		for(int i=0;i<rdata.rows;i++)
		{
			for(int j=0;j<(int)data.size();j++)
			{
				if(rdata.elemF[i][col]>=bins[j] && rdata.elemF[i][col]< bins[j+1] )
				{
					for(int k=0;k<ncols;k++)
						data[j].mat.elemF[count[j]][k]=rdata.elemF[i][k];
					++count[j];
				}
			}				
		}
	}

	return;
}

int dataHandler::getDataAscii(MATRIXSHORT &data, int nsigs, string filename)
{
	vector<vector<string> > items;
	int maxrows = 10000000;
//	int maxrows = 1000;
	ReadTable(filename,&items,DEFAULT_COMMENTS,DEFAULT_SEPARATORS,DEFAULT_WHITESPACE,maxrows);		
	int npts=(int)items.size()-1;  // Header takes 1 line : - actually, it doesn't // 2/23/07; actually it seems to depend? 
//	int npts=(int)items.size();
	if(npts<=0)
		return 0; 
	
	data.cols = nsigs+1;  // 
	data.elemF = fmatcalloc(npts,(int)data.cols); // nsigs+target
	data.rows = npts;
	data.rpartSorts = NULL;  // not needed except by trees 
	data.rpartWhich = NULL;
	data.target = NULL;    // if this is not NULL, then it is used in the tree fit;
	data.elem   = NULL;    // if this is NULL, use the elemF
	data.rowlabel = NULL;

	int idx=0;
	if(s.size()>0)
	{
		for(int i=0;i<npts;i++)
			for(int j=0;j<data.cols;j++)
			//	data.elemF[i][j]  = atof(items[i][s[j]].c_str()); 
				 data.elemF[i][j]  = atof(items[i+1][s[j]].c_str());  // header takes one line: NO!; well sometimes it does 
	}
	else
		for(int i=0;i<npts;i++)
			for(int j=0;j<data.cols;j++)
			//	data.elemF[i][j]  = atof(items[i][j].c_str());
				data.elemF[i][j]  = atof(items[i+1][j].c_str());

	return npts;
}


int dataHandler::getDataTransposeAscii(MATRIXSHORT &data, int nsigs, string filename)
{
	vector<vector<string> > items;
	int maxrows = 10000000;
	ReadTable(filename,&items,DEFAULT_COMMENTS,DEFAULT_SEPARATORS,DEFAULT_WHITESPACE,maxrows);		
	int npts=(int)items.size()-1;  // Header takes 1 line : - actually, it doesn't // 2/23/07; actually it seems to depend? 
//	int npts=(int)items.size();
	if(npts<=0)
		return 0; 
	
	data.rows = nsigs+1;  // 
	data.cols = npts;
	data.elemF = fmatcalloc((int)data.rows,npts); // nsigs+target
	data.rpartSorts = NULL;  // not needed except by trees 
	data.rpartWhich = NULL;
	data.target = NULL;    // if this is not NULL, then it is used in the tree fit;
	data.elem   = NULL;    // if this is NULL, use the elemF
	data.rowlabel = (char **) calloc(data.rows, sizeof(char *));

	if(nsigs > 4)
	{
		data.rowlabel[0] = "vL1";		
		data.rowlabel[1] = "vL2";		
		data.rowlabel[2] = "vL3";    
		data.rowlabel[3] = "vL4";
		data.rowlabel[4] = "vL5";
		data.rowlabel[5] = "vL6";
		data.rowlabel[6] = "vL7";
		data.rowlabel[7] = "vL8";

		data.rowlabel[8]  = "rL1";  
		data.rowlabel[9]  = "rL2";
		data.rowlabel[10] = "rL3";
		data.rowlabel[11] = "rL4";
	/*	data.rowlabel[12] = "coL1";
		data.rowlabel[13] = "coL2";

		data.rowlabel[14] = "coL3"; 
		data.rowlabel[15] = "coL4";
		data.rowlabel[16] = "coL5";
		data.rowlabel[17] = "coL6";
		data.rowlabel[18] = "coL7";
		data.rowlabel[19] = "coL8";   */
		data.rowlabel[nsigs] = "target";
	}
	else
	{
		data.rowlabel[0] = "target";		
		data.rowlabel[1] = "bmpred";		
		data.rowlabel[2] = "pred";    
		data.rowlabel[3] = "sprd";
	}
	int idx=0;
	if(s.size()>0)
	{
		for(int i=0;i<data.rows;i++)
			for(int j=0;j<data.cols;j++)
				 data.elemF[i][j]  = atof(items[j+1][s[i]].c_str());	}
	else
		for(int i=0;i<data.rows;i++)
			for(int j=0;j<data.cols;j++)
				 data.elemF[i][j]  = atof(items[j+1][i].c_str());

	return npts;
}

void dataHandler::makeCatTarget(MATRIXSHORT &data, int taridx, int sprdidx, int midpriceidx, double thresh)
{
	// 0 = sell;
	// 1 = do nothing;
	// 2 = buy; 
	
	// copy to target
	data.cst.resize(data.cols);
	data.ret.resize(data.cols);
	data.wt.resize(data.cols); // for adaboost
	double twt = 1./data.cols;

	for(int i=0;i<data.cols;i++)
	{
		data.ret[i] = data.elemF[taridx][i];
		data.wt[i] = twt; 
		double gain     = data.elemF[taridx][i];
		double halfsprd = .5*data.elemF[sprdidx][i];
		double tthresh   = halfsprd;
		double midprice = data.elemF[midpriceidx][i];
if(midpriceidx==2 && sprdidx == 21)
		midprice = pow(10,midprice);
		if(midprice>0)
			tthresh += BASIS_PTS * thresh/midprice;
		data.cst[i] = tthresh;
		if( gain >= 0 ) // buy;
		{
			if(gain > tthresh)
				data.elemF[taridx][i]=2;
			else
				data.elemF[taridx][i]=1;
		}
		else            // sell
		{
			if(-gain > tthresh)
				data.elemF[taridx][i]=0;
			else
				data.elemF[taridx][i]=1;
		}
	}	

	return;		
}

void dataHandler::makeBinaryCatTarget(MATRIXSHORT &data, int taridx, int sprdidx, int midpriceidx, double thresh, int buySell)
{
	// -1 = do nothing
	//  1 = buy or sell
	
	// copy to target
	data.cst.resize(data.cols);
	data.ret.resize(data.cols);
	data.wt.resize(data.cols); // for adaboost
	double twt = 1./data.cols;

	for(int i=0;i<data.cols;i++)
	{
		data.ret[i] = data.elemF[taridx][i];
		data.wt[i] = twt;
		double gain     = data.elemF[taridx][i];
		double halfsprd = .5*data.elemF[sprdidx][i];
		double tthresh   = halfsprd;
		double midprice = data.elemF[midpriceidx][i];
if(midpriceidx==2 && sprdidx == 21)
		midprice = pow(10,midprice);
		if(midprice>0)
			tthresh += BASIS_PTS * thresh/midprice;
		data.cst[i] = tthresh;
	
		if(buySell == 1)
		{
			if( gain >= 0 && gain > tthresh) // buy;
				data.elemF[taridx][i]=1;
			else
				data.elemF[taridx][i]=-1;
		}
		else if(buySell == -1)
		{
			if(-gain >0 && -gain > tthresh)
				data.elemF[taridx][i]=1;
			else
				data.elemF[taridx][i]=-1;
		}
	}	

	return;		
}


void dataHandler::writeMatrixShort(const MATRIXSHORT &data, string file)
{
	fstream fsWrite(file.c_str(),ios::out);
	fsWrite.precision(6);
	
	if(data.rowlabel != NULL)
	{
		for(int j=0;j<data.rows-1;j++)	
			fsWrite << data.rowlabel[j] << "\t";  
		fsWrite << data.rowlabel[data.rows-1] << endl;
	}
	if(data.elemF!=NULL)
	{
		for(int i=0; i<data.cols; i++)
		{
			for(int j=0;j<data.rows-1;j++)	
				fsWrite << data.elemF[j][i] << "\t";  
			fsWrite << data.elemF[data.rows-1][i] << endl;  
		}
	}
	return;
}


void dataHandler::getTestDatesOpt(int){}
void dataHandler::getFitDatesOpt(int, int){}

void dataHandler::getDataTransposeOpt2(MATRIXSHORT &data, int nsigs, int &npts, vector<int> &dates, vector<int> &whichIdx)
{
//	// if npts = 0, count how many points there are; otherwise fill the data
//	if(npts>0)
//	{
//		data.rows = nsigs+1;
//		data.elemF = fmatcalloc(data.rows, npts);
// 		data.cols = npts;
//		data.rpartSorts = NULL;   
//		data.rpartWhich = lvcalloc(npts);
//		data.target = NULL;   
//		data.elem   = NULL;    
//		data.rowlabel = (char **) calloc(data.rows, sizeof(char *));
//
//		if(nsigs == 25) 
//		{
//			data.rowlabel[0] = "ufc";		
//			data.rowlabel[1] = "quim";		
//
//			data.rowlabel[2] = "msecs";    
//			data.rowlabel[3] = "spot";     
//			data.rowlabel[4] = "midPrice";  
//			data.rowlabel[5] = "spread";  
//			data.rowlabel[6] = "daysToExp";   
//			data.rowlabel[7] = "delta"; 
//			data.rowlabel[8] = "realVol"; 
//			data.rowlabel[9] = "spreadopt"; 
//			data.rowlabel[10] = "midoff1";
//			data.rowlabel[11] = "midoff2";
//
//			data.rowlabel[12] = "uretON"; 
//			data.rowlabel[13] = "cretON"; 
//			data.rowlabel[14] = "trImbON"; 
//			data.rowlabel[15] = "vretON"; 
//			data.rowlabel[16] = "ltforec"; 
//			data.rowlabel[17] = "linforec"; 
//
//			data.rowlabel[18] = "uret0"; 
//			data.rowlabel[19] = "uret1";  // underlying ret 
//			data.rowlabel[20] = "uret2";
//			data.rowlabel[21] = "uret3";
//			data.rowlabel[22] = "uret4";
//			data.rowlabel[23] = "uret5";  
//			data.rowlabel[24] = "uVolume";   // filtered  volatility ret
//
//			data.rowlabel[nsigs] = "target"; 
//		}	
//		else if(nsigs == 52) 
//		{
//			data.rowlabel[0] = "ufc";		
//			data.rowlabel[1] = "quim";		
//
//			data.rowlabel[2] = "msecs";    
//			data.rowlabel[3] = "spot";     
//			data.rowlabel[4] = "midPrice";  
//			data.rowlabel[5] = "spread";  
//			data.rowlabel[6] = "daysToExp";   
//			data.rowlabel[7] = "delta"; 
//			data.rowlabel[8] = "realVol"; 
//			data.rowlabel[9] = "spreadopt"; 
//
//			data.rowlabel[10] = "midoff1";
//			data.rowlabel[11] = "midoff2";
//
//			data.rowlabel[12] = "uretON"; 
//			data.rowlabel[13] = "cretON"; 
//			data.rowlabel[14] = "trImbON"; 
//			data.rowlabel[15] = "vretON"; 
//			data.rowlabel[16] = "ltforec"; 
//			data.rowlabel[17] = "linforec"; 
//
//			data.rowlabel[18] = "uret0"; 
//			data.rowlabel[19] = "uret1";  // underlying ret 
//			data.rowlabel[20] = "uret2";
//			data.rowlabel[21] = "uret3";
//			data.rowlabel[22] = "uret4";
//			data.rowlabel[23] = "uret5";  
//
//			data.rowlabel[24] = "cret0"; 
//			data.rowlabel[25] = "cret1";  // contract  ret 
//			data.rowlabel[26] = "cret2";
//			data.rowlabel[27] = "cret3";
//			data.rowlabel[28] = "cret4";
//			data.rowlabel[29] = "cret5";  
//
//			data.rowlabel[30] = "vret0"; 
//			data.rowlabel[31] = "vret1";  // contract  ret 
//			data.rowlabel[32] = "vret2";
//			data.rowlabel[33] = "vret3";
//			data.rowlabel[34] = "vret4";
//			data.rowlabel[35] = "vret5";  
//
//			data.rowlabel[36] = "trImb0"; 
//			data.rowlabel[37] = "trImb1";  // contract  ret 
//			data.rowlabel[38] = "trImb2";
//			data.rowlabel[39] = "trImb3";
//			data.rowlabel[40] = "trImb4";
//			data.rowlabel[41] = "trImb5";  
//
//			data.rowlabel[42] = "trImbW0"; 
//			data.rowlabel[43] = "trImbW1";  // contract  ret 
//			data.rowlabel[44] = "trImbW2";
//			data.rowlabel[45] = "trImbW3";
//			data.rowlabel[46] = "trImbW4";
//			data.rowlabel[47] = "trImbW5"; 
//
//			data.rowlabel[48] = "uVolume";   // filtered  volatility ret
//
//			data.rowlabel[49] = "quim1";
//			data.rowlabel[50] = "quim2";
//			data.rowlabel[51] = "vega";
//
//			data.rowlabel[nsigs] = "target"; 
//
//	//		data.rowlabel[21] = "uVolat";   // filtered  volatility ret
//		}	
//		else if(nsigs==6)
//		{
//			data.rowlabel[0] = "target";
//			data.rowlabel[1] = "bmpred";
//			data.rowlabel[2] = "tbpred";	
//			data.rowlabel[3] = "spread";
//			data.rowlabel[4] = "term";
//			data.rowlabel[5] = "delta";
//			data.rowlabel[6] = "spot";
//		}
//	
//	}
//
//	Exchange ex("US");
//	char fname[LARGEBUFFSIZE];
// 
//	//get all days that have valid data 
//	int idx=0;
//	int firstLinParamsDate=olsParams.ValidDates(nDaysFit).front(); // first date on which we have params
//	for(unsigned dt=0;dt<dates.size();++dt)
//	{	
//		int idate=dates[dt];
//
//		//load today's index returns
//		MarketReturn mRet(idate);
//
//		//extract parameters
//		hfoLinFrc<BilinInput41Trd> lfc(BilinInput41Trd(),10);
//		bool paramsLoaded=false;
//		if(idate<firstLinParamsDate)//before oosStartDate use in-sample forecast
//			paramsLoaded=olsParams.ExtractParams(firstLinParamsDate,nDaysFit,&lfc);
//		else
//			paramsLoaded=olsParams.ExtractParams(idate,nDaysFit,&lfc);
//
//		if(!paramsLoaded)
//			continue;
//
//		//unit forecast with input limit
//		lfc.InitForecast(inpLimit);
//
//		// read in stockparams to get volume, volatility etc
//// used for 2009-opt-1 not opt-2
//		char sqlCmd[LARGEBUFFSIZE];
//		sprintf(sqlCmd,"SELECT uniqueID, medVolume, medVolatility, medmedSprd "
//					"FROM stockCharacteristics "
//					"WHERE idate = %d and uniqueID IS NOT NULL "
//					"ORDER BY uniqueID ",
//					idate);
//		ODBCConnection odbcHF("hfStock","mercator1","DBacc101"); 
//		vector<vector<string> > stchar;
//		odbcHF.ReadTable(sqlCmd,&stchar);
//		int Nids =(int)stchar.size();
//		if(Nids < 2000)
//			continue;
//		map<string,int> tickerMap;
//		for(int i=0;i<Nids;i++)
//		{
//			string s = FirstWord(stchar[i][0]); 
//			string ticker = th.UniqueToTicker(s,idate);
//			if(ticker == "")
//				continue;
//			tickerMap[ticker]=i; 
//		}
//
//		for(int grp=0;grp < nGroups;grp++)
//		{
//			//open the file with sample data
//			sprintf(fname,"%s\\smpl%d_%d_%d.bin",binDir.c_str(),idate,nUniverse,grp);
//			fstream fs2(fname,ios::in|ios::binary);
//			if(fs2.is_open())
//			{   
//				//load contract mask information for idate
//				sprintf(fname,"%s\\cm%d_%d_%d.txt",binDir.c_str(),idate,nUniverse,grp);
//				ContractMasks cm;
//				cm.LoadFromTextFile(idate,fname,ex);
//
//				//extract universe of underlying stocks
//				vector<string> uni=cm.UnderSyms();
//				if(uni.size()==0)
//				{	ERROR_HANDLER("no cntract mask data",EL_WARNING);
//				}
//
//				//load tomorrow's data (average spot and average mid premium)
//				map<int,pair<double,double> > tomData;
//				int tomDate=(int)ex.NextClose(QuoteTime(idate,200000,ex.TZ())).Date();
//				LoadTomData(tomDate,cm,th,&tomData);
//
//				//load today's stock price data (need for delta hedging)
//				StockReturn sRet(idate);
//				for(unsigned u=0;u<uni.size();++u)
//					sRet.InsertUniverse(uni[u]);
//				sRet.DoneUniverse();
//
//				while(true)
//				{  	
//					OptionDataPoint odp;
//					fs2.read((char*)&odp,sizeof(OptionDataPoint));
//					if(!(fs2.rdstate()==0))
//						break;
//
//					//check for data validity
//					if(odp.inputs.spot<=0.||odp.inputs.midPrice<=0.||odp.fwdPrice120<=0.||odp.fwdPrice180<=0.||odp.closePrice<=0.)
//						continue;
//
//// used in 2009-opt-1
////	if( fabs(odp.inputs.delta) < .1 || fabs(odp.inputs.delta) > .7 || odp.inputs.term>1)
////		continue; 
//
//// 2009-opt-2
//	if( fabs(odp.inputs.delta) < .1 || fabs(odp.inputs.delta) > .7 || odp.inputs.term>3)
//		continue; 
//
//					//retrieve contract mask information for contract ID
//					Option opt;
//					if(!cm.FindContract(odp.inputs.contractID,&opt))
//					{	char msg[LARGEBUFFSIZE];
//						sprintf(msg,"unknown contract ID %d on %d",odp.inputs.contractID,idate);
//						ERROR_HANDLER(msg,EL_WARNING);//this should not happen
//						continue;
//					}
//					//calculate days to expiration
//					int tradeDaysToExp=ROUND(ex.TradeTimeDiff(idate,opt.ExpTime()));
//					int uSymID=HFO_CM_USYM_ID(odp.inputs.contractID);
//
//					// just added this back in on 20080610 to get underlying volume 
//					  string symbol=opt.Under().Symbol();
//// opt-1 stuff
//					  map<string,int>::const_iterator iteru = tickerMap.find(symbol);
//					  int tickRow = 0;
//					  if( iteru != tickerMap.end() )
//							tickRow = iteru->second;
//					  else
//						continue;
//
//					double uVolume = 0;
//					double uVolat  = 0;
//					double uSprd   = 0;
//// opt-1 stuff
//					  uVolume = atof(stchar[tickRow][1].c_str());
//					  uVolat  = atof(stchar[tickRow][2].c_str());
//					  uSprd   = atof(stchar[tickRow][3].c_str());
//					  if(uVolume <= 0 || uVolat<=0)
//							continue;
//
//					//Calculate all the pieces needed for the target:
//					//The over-night target is defined as the price difference between now tomorrow's average price,
//					//market-hedged between 1m and 120m and delta-hedged between 120m and tomorrow.
//					//The different hedging period are used to avoid hedging away the underlying forecast
//
//					//market return between now +1m and now +120m
//					double mRet120=mRet.GetReturn(odp.inputs.msecs+60000,odp.inputs.msecs+7200000);
//					double mRet60 =mRet.GetReturn(odp.inputs.msecs,odp.inputs.msecs+3600000);
//
//					//underlying return between now and now +120m
//					double stockRet120=sRet.GetReturn(uSymID,odp.inputs.msecs,odp.inputs.msecs+7200000);
//					double stockRet60 =sRet.GetReturn(uSymID,odp.inputs.msecs,odp.inputs.msecs+3600000);
//
//					//market hedged underlying return (no hedge in the first minute)
//					double stockRet120H=stockRet120 -mRet120;
//					//underlying return between now and now and the close
//					double stockRetCls=sRet.GetReturn(uSymID,odp.inputs.msecs,NYSE_CLOSE_MSECS);
//					//market hedged 120m target
//					double tgt120H=(odp.fwdPrice120-odp.inputs.midPrice)/odp.inputs.spot -odp.inputs.delta*mRet120;
//					//target to close
//
//					double tgt60=(odp.fwdPrice60-odp.inputs.midPrice)/odp.inputs.spot;
//					double tgt60DeltaH    =tgt60 - odp.inputs.delta*stockRet60;
//					double tgt60MktDeltaH =tgt60 - odp.inputs.delta*mRet60;
//
//					double tgtClose=(odp.closePrice-odp.inputs.midPrice)/odp.inputs.spot;
//					//hedged target to close (no hedge between 0 and 1m, market hedge between 1m and 120m, delta hedge between 120m and close)
//					double tgtCloseHD=tgtClose-odp.inputs.delta*(stockRetCls -stockRet120H);
//					//overnight target defaults to close target
//					double tgtTom=tgtClose;
//					//hedged overnight target defaults to hedged close target
//					double tgtTomHD=tgtCloseHD;
//					map<int,pair<double,double> >::iterator itTom=tomData.find(odp.inputs.contractID);
//					if(itTom!=tomData.end())//calculate overnight target (if not available use close target)
//					{	double fwdSpot=itTom->second.first;
//						double fwdMid=itTom->second.second;
//						double stockRetTom=fwdSpot/odp.inputs.spot -1.;			
//						tgtTom=(fwdMid-odp.inputs.midPrice)/odp.inputs.spot;
//						tgtTomHD=tgtTom -odp.inputs.delta*(stockRetTom -stockRet120H);
//					}
//										
//					//skip outliers
////					if(fabs(tgtTom)>0.15 || fabs(tgtTomHD)>0.15 || fabs(tgt120H)>0.15)
//					if(fabs(tgtTom)>0.15 || fabs(tgtTomHD)>0.15 || fabs(tgt120H)>0.15 || fabs(tgt60)>0.15 )
//						continue;
//				
//					if(npts>0) // we fill in the data
//					{
//						//calculate linear forecast (fit to tgtTomHD)
//						double forec=lfc.Forecast(odp.inputs);
//						if(nsigs==25)
//						{
//							data.elemF[0][idx]  = odp.inputs.ufc; 
//							data.elemF[1][idx]  = odp.inputs.quim; 
//							data.elemF[2][idx]  = odp.inputs.msecs; 
//							data.elemF[3][idx]  = odp.inputs.spot; 
//							data.elemF[4][idx]  = odp.inputs.midPrice; 
//							data.elemF[5][idx]  = odp.inputs.spread/odp.inputs.spot; 
//
//							data.elemF[6][idx]  = tradeDaysToExp; 
//							data.elemF[7][idx]  = odp.inputs.delta; 
//							data.elemF[8][idx]  = odp.inputs.realVol; 
//							data.elemF[9][idx] = odp.inputs.spread/odp.inputs.midPrice;
//
//							data.elemF[10][idx]  = odp.inputs.midOffs1; 
//							data.elemF[11][idx]  = odp.inputs.midOffs2; 
//							data.elemF[12][idx]  = odp.inputs.uRetON;
//							data.elemF[13][idx]  = odp.inputs.cntrRetON; 
//							data.elemF[14][idx]  = odp.inputs.parRetON;
//							data.elemF[15][idx]  = odp.inputs.volRetON;
//							data.elemF[16][idx]  = odp.inputs.ltForec;
//							if(dHmodel==48 || dHmodel==49)
//								data.elemF[17][idx]  = forec;
//							else
//								data.elemF[17][idx]  = 0;
//
//							for(int i=0;i<HFO_RET_NSTEPS;i++)
//							{
//								data.elemF[18+i][idx]  = odp.inputs.uRets[i];// + odp.inputs.cntrRets[i]; 
//							}
//							data.elemF[24][idx] = uVolume;
//
//							// EC
//							if(dHmodel==47)
//								data.elemF[nsigs][idx] = BASIS_PTS*(tgtTomHD - forec);
//							// PURE
//							if(dHmodel==48)
//								data.elemF[nsigs][idx] = BASIS_PTS*(tgtTomHD);
//							if(dHmodel==49)
//								data.elemF[nsigs][idx] = BASIS_PTS*(tgt60);
//							if(dHmodel==50)
//								data.elemF[nsigs][idx] = BASIS_PTS*(tgt60 - forec);
//		//					data.elemF[39][idx] = uVolat;
//						}
//						else if(nsigs==6)
//						{
//							if(dHmodel==47)
//								data.elemF[0][idx]  = BASIS_PTS*(tgtTomHD - forec); 
//							if(dHmodel==48)
//								data.elemF[0][idx]  = BASIS_PTS*(tgtTomHD); 
//							if(dHmodel==49 ||dHmodel==51)
//								data.elemF[0][idx] = BASIS_PTS*(tgt60);
//							if(dHmodel==50)
//								data.elemF[0][idx] = BASIS_PTS*(tgt60 - forec);
//
//							if(dHmodel==52)
//								data.elemF[0][idx] = BASIS_PTS*(tgt60DeltaH); 
//							if(dHmodel==53)
//								data.elemF[0][idx] = BASIS_PTS*(tgt60MktDeltaH);
//
//							data.elemF[1][idx]  = BASIS_PTS*forec; // BASIS_PTS*ftd.linForec; 
//							data.elemF[2][idx]  = 0; 
//							data.elemF[3][idx]  =  BASIS_PTS * odp.inputs.spread/odp.inputs.spot;  
//							data.elemF[4][idx]  = odp.inputs.term; 				
//							data.elemF[5][idx]  = odp.inputs.delta;  
//							data.elemF[6][idx]  = odp.inputs.spot;  
//						} // end n == 6 or n == nsigs
//						else if(nsigs == 52 )
//						{
//							data.elemF[0][idx]  = odp.inputs.ufc; 
//							data.elemF[1][idx]  = odp.inputs.quim; 
//							data.elemF[2][idx]  = odp.inputs.msecs; 
//							data.elemF[3][idx]  = odp.inputs.spot; 
//							data.elemF[4][idx]  = odp.inputs.midPrice; 
//							data.elemF[5][idx]  = odp.inputs.spread/odp.inputs.spot; 
//							data.elemF[6][idx]  = tradeDaysToExp; 
//							data.elemF[7][idx]  = odp.inputs.delta; 
//							data.elemF[8][idx]  = odp.inputs.realVol; 
//							data.elemF[9][idx] = odp.inputs.spread/odp.inputs.midPrice;
//
//							data.elemF[10][idx]  = odp.inputs.midOffs1; 
//							data.elemF[11][idx]  = odp.inputs.midOffs2; 
//							data.elemF[12][idx]  = odp.inputs.uRetON;
//							data.elemF[13][idx]  = odp.inputs.cntrRetON; 
//							data.elemF[14][idx]  = odp.inputs.parRetON;
//							data.elemF[15][idx]  = odp.inputs.volRetON;
//							data.elemF[16][idx]  = odp.inputs.ltForec;
//							data.elemF[17][idx]  = 0; // forec
//
//							for(int i=0;i<HFO_RET_NSTEPS;i++)
//							{
//								data.elemF[18+i][idx]  = odp.inputs.uRets[i];// + odp.inputs.cntrRets[i]; 
//								data.elemF[24+i][idx]  = odp.inputs.cntrRets[i];//odp.inputs.uRets[i] - odp.inputs.cntrRets[i]; 
//							    data.elemF[30+i][idx]  = odp.inputs.volRets[i];  
//								data.elemF[36+i][idx]  = odp.inputs.trimRets[i];  // trade
//								data.elemF[42+i][idx]  = odp.inputs.trimVolRets[i]; //trade
//							}
//
//							data.elemF[48][idx] = uVolume;   // filtered  volatility ret
//							data.elemF[49][idx]  = odp.inputs.quim1; 
//							data.elemF[50][idx]  = odp.inputs.quim2; 
//							data.elemF[51][idx] = odp.inputs.vega;											
//
//							// EC
//							if(dHmodel==47)
//								data.elemF[nsigs][idx] = BASIS_PTS*(tgtTomHD - forec);
//							// PURE
//							if(dHmodel==48)
//								data.elemF[nsigs][idx] = BASIS_PTS*(tgtTomHD);
//							if(dHmodel==51)
//								data.elemF[nsigs][idx] = BASIS_PTS*(tgt60);
//
//							if(dHmodel==52)
//								data.elemF[nsigs][idx] = BASIS_PTS*(tgt60DeltaH); 
//
//							if(dHmodel==53)
//								data.elemF[nsigs][idx] = BASIS_PTS*(tgt60MktDeltaH);
//		
//						}
//					}     // end if npts == 0 
//					++idx; // because we are in the while loop, we are reading a new data point and must increase idx by 1
//				} // close the while true loop ==> all the data has been read
//			} // close the is file if statement
//		} // close the loop over ngroup
//		cout << "date = " << "\t" << idate <<  "\t" << "idx = " << idx << endl; 
//	} // close the loop over dates 
//	if(npts == 0)
//		npts = idx;
	return;
}



//
//void LoadTRMatrix(int idate,const std::string& dir,std::vector<std::string>* uidVect,double*** riskMatrix)
//{	char fname[LARGEBUFFSIZE];
//	bool readSuccess=false;
//	uidVect->clear();
//
//	if(*riskMatrix!=NULL)
//	{	ERROR_HANDLER("riskMatrix must be NULL",EL_WARNING);
//		return;
//	}
//
//
//	struct _finddata_t fileinfo;
//	char fpath[LARGEBUFFSIZE];
//	sprintf(fpath,"%s\\cr*",dir.c_str());
//	intptr_t fileFindHandle=_findfirst(fpath,&fileinfo);
//	int lastDate=20010101;
//	if(fileFindHandle!=-1)
//	{	do
//		{	if(fileinfo.name[0]>='A'&&fileinfo.name[0]<='z')
//			{	int fDate=-1;
//				sscanf(fileinfo.name,"cr%d.bin",&fDate);
//				if(fDate>lastDate && fDate<=idate)
//					lastDate=fDate;
//			}
//		}while(_findnext(fileFindHandle,&fileinfo)!=-1);
//	}
//	_findclose(fileFindHandle);
//	if(lastDate>20010101)
//	{	
//		vector<vector<string> > records;
//		sprintf(fname,"%s\\uid%d.txt",dir.c_str(),lastDate);
//		ReadTable(fname,&records);
//		for(unsigned u=0;u<records.size();++u)
//			uidVect->push_back(records[u][0]);
//		
//		sprintf(fname,"%s\\cr%d.bin",dir.c_str(),lastDate);
//		fstream fsRM(fname,ios::in|ios::binary);
//		*riskMatrix=new double*[uidVect->size()];
//		float* tmpVect=new float[uidVect->size()];
//
//		for(unsigned u=0;u<uidVect->size();++u)
//		{	(*riskMatrix)[u]=new double[uidVect->size()];
//		}
//		
//		readSuccess=true;
//		for(unsigned u=0;u<uidVect->size();++u)
//		{	fsRM.read((char*)tmpVect,(u+1)*sizeof(float));
//				for(unsigned v=0;v<=u;++v)
//					(*riskMatrix)[u][v]=tmpVect[v];
//			if(fsRM.rdstate()!=0)
//			{	readSuccess=false;
//			}
//		}
//		char ch;
//		fsRM.read(&ch,1);
//		if(fsRM.rdstate()==0)
//		{	ERROR_HANDLER("file not read completely",EL_WARNING);
//			readSuccess=false;
//		}
//
//		delete tmpVect;
//	}
//
//	if(!readSuccess)
//	{	ERROR_HANDLER("risk matrix read failed",EL_WARNING);
//		if(*riskMatrix!=NULL)
//		{	for(unsigned v=0;v<=uidVect->size();++v)
//				delete (*riskMatrix)[v];
//			delete *riskMatrix;
//			*riskMatrix=NULL;
//		}
//		uidVect->clear();
//	}
//	else
//	{	for(unsigned u=0;u<uidVect->size();++u)
//			for(unsigned v=0;v<u;++v)
//				(*riskMatrix)[v][u]=(*riskMatrix)[u][v];
//	}
//}
