#include <string>
#include <map>
#include <set>
#include <vector>
#include <jl_lib.h>
#include <gtlib_model/hff.h>
using namespace std;

void usage()
{
	cout << "usage:\n";
	cout << " checkParams.exe -u -m AT\n";
	cout << " checkParams.exe -u -l -om -tm -m US EU CA\n";
	exit(0);
}

void check_nonlinear(vector<string>& models, string type);
void check_linear(vector<string>& models);

void check_nonlinear(string model, string market, string mCode, string type);
void check_linear(string model, string market, string mCode);
void check_universe(string model, int idate, vector<string>& markets, set<int>& idates, string mCode);
void check_params1(string model, vector<string>& markets, set<int>& idates, string mCode);
void get_univ_size(map<int, int>& mUniv, string model, string market, int mindate, int maxdate);
void get_univ_size_market(map<string, map<int, int> >& mMarketUniv, string model, vector<string>& markets, int mindate, int maxdate);
void get_paramsOK(map<int, int>& mParamsOK, string model, string market, int mindate, int maxdate);
void print_line_univ(int idate, int this_date, int nuniv, vector<int>& muniv, int ok);
set<int> get_dates(string market, int idate, int N);
string modelCode(string model);

int main(int argc, char* argv[])
{
	if( argc < 2 )
		usage();
	Arg arg(argc, argv);

	vector<string> models;
	map<string, vector<string> > mmarkets;
	map<string, string > mmCode;

	if( arg.isGiven("m") )
	{
		models = arg.getVals("m");

		for( vector<string>::iterator it = models.begin(); it != models.end(); ++it )
		{
			string model = *it;
			vector<string> markets = hff::markets(model);
			string market = markets[0];
			string mCode = modelCode(model);

			cout << "\n" << model << endl;

			// Nonlinear om
			if( arg.isGiven("om") )
				check_nonlinear(model, market, mCode, "om");

			// Nonlinear tm
			if( arg.isGiven("tm") )
				check_nonlinear(model, market, mCode, "tm");

			// Nonlinear om trade event model
			if( arg.isGiven("ome") )
				check_nonlinear(model, market, mCode, "ome");

			// Linear model.
			if( arg.isGiven("l") )
				check_linear(model, market, mCode);

			int idate = itoday();
			set<int> idates = get_dates(market, idate, 5);

			// Universe.
			if( arg.isGiven("u") )
				check_universe(model, idate, markets, idates, mCode);

			// stockParams1bySymbol
			if( arg.isGiven("s") )
				check_params1(model, markets, idates, mCode);
		}
	}
	else // All markets.
	{
		models.push_back("US");
		models.push_back("UF");
		models.push_back("CA");
		models.push_back("EU");
		models.push_back("AT");
		models.push_back("AH");
		models.push_back("AS");
		models.push_back("KR");
		models.push_back("MJ");
		models.push_back("SS");

		// Nonlinear om
		if( arg.isGiven("om") )
			check_nonlinear(models, "om");

		// Nonlinear tm
		if( arg.isGiven("tm") )
			check_nonlinear(models, "tm");

		// Nonlinear om trade event model
		if( arg.isGiven("ome") )
			check_nonlinear(models, "ome");

		// Linear model.
		if( arg.isGiven("l") )
			check_linear(models);
	}
}

set<int> get_dates(string market, int idate, int N)
{
	set<int> idates;

	int pdate = idate;
	for( int i = 0; i < N; ++i )
	{
		pdate = prevClose(market, pdate);
		idates.insert(pdate);
	}

	int fdate = idate;
	for( int i = 0; i < N; ++i )
	{
		idates.insert(fdate);
		fdate = nextClose(market, fdate);
	}

	return idates;
}

// --------------------------------------------------------------------------------
// Nonlinear model.
// --------------------------------------------------------------------------------

void check_nonlinear(vector<string>& models, string type)
{
}

void check_nonlinear(string model, string market, string mCode, string type)
{
	cout << type << endl;
	string tablename = (type == "om") ? "hftree" : "hfpuretreeboost";
	if( type == "om" )
		tablename = "hftree";
	else if( type == "tm" )
		tablename = "hfpuretreeboost";
	else if( type == "ome" )
		tablename = "hftreetradeevent";

	char buf[1000];
	char cmd[2000];
	sprintf(cmd, "select distinct idate from %s where market = '%s' order by idate",
		tablename.c_str(), mCode.c_str());
	vector<vector<string> > vv;
	GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);
	int nModels = vv.size();

	if( nModels > 0 )
	{
		int idate = atoi(vv[nModels - 1][0].c_str());
		int prev_idate = 0;
		if( vv.size() > 1 )
			prev_idate = atoi(vv[nModels - 2][0].c_str());

		sprintf(buf, "Recent model: %d, previous model: %d\n", idate, prev_idate);
		cout << buf;

		if( idate > 0 )
		{
			int nTrees = 0;
			{
				sprintf(cmd, "select count(distinct treenumber) from %s"
					" where market = '%s' and idate = %d",
					tablename.c_str(), mCode.c_str(), idate);
				vector<vector<string> > vv;
				GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);
				nTrees = atoi(vv[0][0].c_str());
			}

			sprintf(cmd, "select cutvariable, cutvalue, npts, mu, deviance from %s"
				" where market = '%s' and idate = %d order by treenumber, idx",
				tablename.c_str(), mCode.c_str(), idate);
			vector<vector<string> > vv;
			GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);

			int nNodes = vv.size();
			sprintf(buf, "Number of trees: %d Number of nodes: %d\n", nTrees, nNodes);
			cout << buf;

			for( int i = 0; i < 5; ++i )
			{
				sprintf(buf, "%2d %10.4f %10d %8.4f %14.1f\n",
					atoi(vv[i][0].c_str()), atof(vv[i][1].c_str()), atoi(vv[i][2].c_str()), atof(vv[i][3].c_str()), atof(vv[i][4].c_str()));
				cout << buf;
			}

			cout << "..." << endl;
			for( int i = nNodes - 5; i < nNodes; ++i )
			{
				sprintf(buf, "%2d %10.4f %10d %8.4f %14.1f\n",
					atoi(vv[i][0].c_str()), atof(vv[i][1].c_str()), atoi(vv[i][2].c_str()), atof(vv[i][3].c_str()), atof(vv[i][4].c_str()));
				cout << buf;
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Linear Model.
// --------------------------------------------------------------------------------

void check_linear(vector<string>& models)
{
}

void check_linear(string model, string market, string mCode)
{
}

// --------------------------------------------------------------------------------
// hfUniverse.
// --------------------------------------------------------------------------------

void check_universe(string model, int idate, vector<string>& markets, set<int>& idates, string mCode)
{
	string market = markets[0];
	int mindate = *(idates.begin());
	int maxdate = *(idates.rbegin());

	map<int, int> mUniv;
	get_univ_size(mUniv, model, market, mindate, maxdate);

	map<string, map<int, int> > mMarketUniv;
	if( markets.size() > 1 )
		get_univ_size_market(mMarketUniv, model, markets, mindate, maxdate);

	map<int, int> mParamsOK;
	get_paramsOK(mParamsOK, model, market, mindate, maxdate);

	for( set<int>::iterator it = idates.begin(); it != idates.end(); ++it )
	{
		int this_date = *it;
		int nuniv = mUniv[this_date];
		vector<int> muniv;
		if( markets.size() > 1 )
		{
			for( vector<string>::iterator itm = markets.begin(); itm != markets.end(); ++itm )
				muniv.push_back(mMarketUniv[*itm][this_date]);
		}
		int ok = mParamsOK[this_date];
		print_line_univ(idate, this_date, nuniv, muniv, ok);
	}
}

void print_line_univ(int idate, int this_date, int nuniv, vector<int>& muniv, int ok)
{
	if( idate == this_date )
		printf("->%8d", this_date);
	else
		printf("%10d", this_date);

	printf("%5d", nuniv);

	int nmarket = muniv.size();
	if( nmarket > 0 )
		for( vector<int>::iterator it = muniv.begin(); it != muniv.end(); ++it )
			if( nmarket > 5 )
				printf("%4d", *it);
			else
				printf("%5d", *it);

	printf("%4d", ok);
	printf("\n");
}

void get_univ_size(map<int, int>& mUniv, string model, string market, int mindate, int maxdate)
{
	// Size of universe.
	char cmd[2000];
	string model2 = model.substr(0, 2);
	string model1 = model.substr(0, 1);
	if( "US" == model2 )
	{
		sprintf(cmd, " select idate, count(*) from hfuniverse "
		" where idate >= %d and idate <= %d "
		" and sectype != 'F' "
		" and inuniverse > 0 group by idate ", 
		mindate, maxdate);
	}
	else if( "UF" == model2 )
	{
		sprintf(cmd, " select idate, count(*) from hfuniverse "
		" where idate >= %d and idate <= %d "
		" and sectype = 'F' "
		" and inuniverse > 0 group by idate ", 
		mindate, maxdate);
	}
	else if( "CA" == model2 || "EU" == model2)
	{
		sprintf(cmd, " select idate, count(*) from hfuniverse "
		" where idate >= %d and idate <= %d "
		" and inuniverse > 0 group by idate ", 
		mindate, maxdate);
	}
	else if( "KR" == model2 )
	{
		sprintf(cmd, " select idate, count(*) from hfuniverse "
		" where idate >= %d and idate <= %d "
		" and exchange in ('K', 'Q') "
		" and inuniverse > 0 group by idate ", 
		mindate, maxdate);
	}
	else if( mto::isInternational(market) )
	{
		sprintf(cmd, " select idate, count(*) from hfuniverse "
		" where idate >= %d and idate <= %d "
		" and exchange = '%s' "
		" and inuniverse > 0 group by idate ", 
		mindate, maxdate, market.substr(1, 1).c_str());
	}

	vector<vector<string> > vv;
	GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		int idate = atoi((*it)[0].c_str());
		int n = atoi((*it)[1].c_str());
		mUniv[idate] = n;
	}
}

void get_univ_size_market(map<string, map<int, int> >& mMarketUniv, string model, vector<string>& markets, int mindate, int maxdate)
{
	for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it )
	{
		string market = *it;

		// Size of universe.
		char cmd[2000];
		string model2 = model.substr(0, 2);
		string model1 = model.substr(0, 1);
		if( mto::isInternational(market) )
		{
			sprintf(cmd, " select idate, count(*) from hfuniverse "
			" where idate >= %d and idate <= %d "
			" and exchange = '%s' "
			" and inuniverse > 0 group by idate ", 
			mindate, maxdate, market.substr(1, 1).c_str());
		}

		vector<vector<string> > vv;
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			int idate = atoi((*it)[0].c_str());
			int n = atoi((*it)[1].c_str());
			mMarketUniv[market][idate] = n;
		}
	}
}

// --------------------------------------------------------------------------------
// stockParams1bySymbol(Sprd).
// --------------------------------------------------------------------------------

void check_params1(string model, vector<string>& markets, set<int>& idates, string mCode)
{
}

// --------------------------------------------------------------------------------
// stockParamsOK.
// --------------------------------------------------------------------------------

void get_paramsOK(map<int, int>& mParamsOK, string model, string market, int mindate, int maxdate)
{
	// stockParamsOK.
	char cmd[2000];
	string model2 = model.substr(0, 2);
	string model1 = model.substr(0, 1);
	if( "US" == model2 )
	{
		sprintf(cmd, " select idate, paramsgoodtogo from stockParamsOK "
		" where idate >= %d and idate <= %d "
		" and exchange = 'U' ",
		mindate, maxdate);
	}
	else if( "UF" == model2 )
	{
		sprintf(cmd, " select idate, paramsgoodtogo from stockParamsOK "
		" where idate >= %d and idate <= %d "
		" and exchange = 'F' ",
		mindate, maxdate);
	}
	else if( "CA" == model2 || "EU" == model2)
	{
		sprintf(cmd, " select idate, paramsgoodtogo from stockParamsOK "
		" where idate >= %d and idate <= %d ",
		mindate, maxdate);
	}
	else if( mto::isInternational(market) )
	{
		sprintf(cmd, " select idate, paramsgoodtogo from stockParamsOK "
		" where idate >= %d and idate <= %d "
		" and exchange = '%s' ",
		mindate, maxdate, market.substr(1, 1).c_str());
	}

	vector<vector<string> > vv;
	GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		int idate = atoi((*it)[0].c_str());
		int ok = atoi((*it)[1].c_str());
		mParamsOK[idate] = ok;
	}
}

// --------------------------------------------------------------------------------
// Other functions.
// --------------------------------------------------------------------------------

string modelCode(string model)
{
	string mCode = model.substr(1, 1);
	if( model.substr(0, 1) == "E" )
		mCode = "E";
	else if( model.substr(0, 1) == "U" )
	{
		if( model.substr(0, 2) == "US" )
			mCode = "U";
		else if( model.substr(0, 2) == "UF" )
			mCode = "F";
	}
	else if( model.substr(0, 1) == "C" )
		mCode = "J";
	else if( "KR" == model )
		mCode = "K";
	return mCode;
}
