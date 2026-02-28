#include <MSignal/MWriteFilters.h>
#include <MSignal/SigSet.h>
#include <MSignal.h>
#include <gtlib_linmod/ARMulti.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <boost/thread.hpp>
using namespace std;
using namespace hff;
using namespace sig;
using namespace gtlib;

MWriteFilters::MWriteFilters(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
write_filters_debug_(true),
write_filters_horizon_(0),
write_filters_lag_(0),
db_offset_(3)
{
	if( conf.count("writeFiltersDebug") )
		write_filters_debug_ = conf.find("writeFiltersDebug")->second == "true";
	if( conf.count("writeFiltersHorizon") )
		write_filters_horizon_ = atoi(conf.find("writeFiltersHorizon")->second.c_str());
	if( conf.count("writeFiltersLag") )
		write_filters_lag_ = atoi(conf.find("writeFiltersLag")->second.c_str());
	if( conf.count("dbOffset") )
		db_offset_ = atoi(conf.find("dbOffset")->second.c_str());
}

MWriteFilters::~MWriteFilters()
{}

void MWriteFilters::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	vector<string> markets = MEnv::Instance()->markets;
	dbname_ = write_filters_debug_ ? mto::hfdbg(markets[0]) : mto::hf(markets[0]);
}

void MWriteFilters::beginDay()
{
	int idate = MEnv::Instance()->idate;
	vector<string> markets = MEnv::Instance()->markets;
	const hff::LinearModel linMod = MEnv::Instance()->linearModel;
	string model = MEnv::Instance()->model;
	string model02 = (model.size() >= 2) ? model.substr(0, 2) : "";

	string filter_horiz_to_write = (string)";" + itos(write_filters_horizon_) + ";0;300";

	// Read the filters.
	ar_.clear();
	vlen_.clear();
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	for( auto it = filters.begin(); it != filters.end(); ++it )
	{
		const hff::IndexFilter& filter = *it;
		if( horizon_match(filter.title(), write_filters_horizon_) )
		{
			string model_path = flt::get_filter_path(filter, model, idate);
			if( !model_path.empty() )
			{
				ar_.push_back(ARMulti(model_path));
				vlen_.push_back(filter.length);
			}
		}
	}

	int nfilters = ar_.size();
	int nfilters_model = filters.size() / linMod.nHorizon;
	if( nfilters != nfilters_model )
	{
		cout << "MWriteFilters::beginDay(): " << nfilters << " filters are found. " << nfilters_model << " expected.\n";
		exit(6);
	}
	else if( nfilters == 0 )
	{
		cout << "MWriteFilters::beginDay(): Filter not found.\n";
		exit(6);
	}

	if( !markets.empty() )
	{
		int dbdate = idate;
		for( int i = 0; i < db_offset_; ++i )
			dbdate = nextClose( markets[0], dbdate );
		cout << "Writing filters for " << dbdate << endl;

		// hffiltersOK.
		char chk1[1000];
		char cmd1[1000];
		char cmd2[1000];
		if( "US" == model02 || "CA" == model02 )
		{
			sprintf(chk1, " select count(*) from hfFiltersOK where idate = %d", dbdate);
			sprintf(cmd1, " delete from hfFiltersOK where idate = %d", dbdate);
			sprintf(cmd2, " insert into hfFiltersOK (idate, numOKFilts) "
				" values (%d, %d)", dbdate, nfilters);
		}
		else
		{
			sprintf(chk1, " select count(*) from hfFiltersOK where idate = %d and exchange = '%s'", dbdate, linMod.mCode.c_str());
			sprintf(cmd1, " delete from hfFiltersOK where idate = %d and exchange = '%s'", dbdate, linMod.mCode.c_str());
			sprintf(cmd2, " insert into hfFiltersOK (idate, numOKFilts, exchange) "
				" values (%d, %d, '%s')",
				dbdate, nfilters, linMod.mCode.c_str());
		}
		check_and_delete(dbname_, chk1, cmd1);
		GODBC::Instance()->get(dbname_)->ExecDirect(cmd2);

		// Wirte filters.
		char cmd3[1000];
		if( "US" == model02 )
		{
			vector<vector<float> >& coeff0 = ar_[0].coeffs();
			vector<vector<float> >& coeff1 = ar_[1].coeffs();
			vector<vector<float> >& coeff2 = ar_[2].coeffs();
			vector<vector<float> >& coeff3 = ar_[3].coeffs();
			vector<vector<float> >& coeff4 = ar_[4].coeffs();
			vector<vector<float> >& coeff5 = ar_[5].coeffs();
			int N = vlen_[0];

			string colname0 = "spxPspx";
			string colname1 = "spfPspx";
			string colname2 = "lcPlc";
			string colname3 = "spfPlc";
			string colname4 = "rusfPspx";
			string colname5 = "rusfPlc";

			print_coeff(colname0, coeff0);
			print_coeff(colname1, coeff1);
			print_coeff(colname2, coeff2);
			print_coeff(colname3, coeff3);
			print_coeff(colname4, coeff4);
			print_coeff(colname5, coeff5);

			// 100
			for( int i = 0; i < 100; ++i )
			{
				char cmdChk[1000];
				sprintf(cmdChk, " select * from hfFilters100 "
					" where idate = %d and idx = %d ",
					dbdate, i);

				vector<vector<string> > vvChk;
				GODBC::Instance()->get(dbname_)->ReadTable(cmdChk, &vvChk);

				if( !vvChk.empty() )
				{
					sprintf(cmd3, "update hfFilters100 set %s = %f, %s = %f, %s = %f, %s = %f "
						" where idate = %d and idx = %d ",
						colname0.c_str(), coeff0[0][i],
						colname1.c_str(), coeff1[0][i],
						colname4.c_str(), coeff4[0][i],
						colname2.c_str(), coeff2[0][i],
						dbdate, i);
				}
				else
				{
					sprintf(cmd3, " insert into hfFilters100 (idate, idx, %s, %s, %s, %s) "
						" values (%d, %d, %f, %f, %f, %f) ",
						colname0.c_str(), colname1.c_str(), colname4.c_str(), colname2.c_str(),
						dbdate, i,
						coeff0[0][i], coeff1[0][i], coeff4[0][i], coeff2[0][i]);
				}
				GODBC::Instance()->get(dbname_)->ExecDirect(cmd3);
			}

			// 300
			for( int i = 0; i < 300; ++i )
			{
				char cmdChk[1000];
				sprintf(cmdChk, " select * from hfFilters300 "
					" where idate = %d and idx = %d ",
					dbdate, i);

				vector<vector<string> > vvChk;
				GODBC::Instance()->get(dbname_)->ReadTable(cmdChk, &vvChk);

				if( !vvChk.empty() )
				{
					sprintf(cmd3, "update hfFilters300 set %s = %f, %s = %f "
						" where idate = %d and idx = %d ",
						colname3.c_str(), coeff3[0][i],
						colname5.c_str(), coeff5[0][i],
						dbdate, i);
				}
				else
				{
					sprintf(cmd3, " insert into hfFilters300 (idate, idx, %s, %s) "
						" values (%d, %d, %f, %f) ",
						colname3.c_str(), colname5.c_str(),
						dbdate, i,
						coeff3[0][i], coeff5[0][i]);
				}
				GODBC::Instance()->get(dbname_)->ExecDirect(cmd3);
			}
		}
		else if( "CA" == model02 )
		{
			vector<vector<float> >& coeff0 = ar_[0].coeffs();
			vector<vector<float> >& coeff1 = ar_[1].coeffs();
			int N = vlen_[0];

			string colname0 = "tsxPtsx";
			string colname1 = "sxfPtsx";
			print_coeff(colname0, coeff0);
			print_coeff(colname1, coeff1);
			for( int i = 0; i < N; ++i )
			{
				char cmdChk[1000];
				sprintf(cmdChk, " select * from hfFilters300 "
					" where idate = %d and idx = %d ",
					dbdate, i);

				vector<vector<string> > vvChk;
				GODBC::Instance()->get(dbname_)->ReadTable(cmdChk, &vvChk);

				if( !vvChk.empty() )
				{
					sprintf(cmd3, "update hfFilters300 set %s = %f, %s = %f "
						" where idate = %d and idx = %d ",
						colname0.c_str(), coeff0[0][i],
						colname1.c_str(), coeff1[0][i],
						dbdate, i);
				}
				else
				{
					sprintf(cmd3, " insert into hfFilters300 (idate, idx, %s, %s) "
						" values (%d, %d, %f, %f) ",
						colname0.c_str(), colname1.c_str(), dbdate, i, coeff0[0][i], coeff1[0][i]);
				}
				GODBC::Instance()->get(dbname_)->ExecDirect(cmd3);
			}
		}
		else
		{
			int iF = 0;
			vector<vector<float> >& coeff = ar_[iF].coeffs();
			int N = vlen_[iF];

			string colname = get_colname(model02);
			print_coeff(colname, coeff);
			for( int i = 0; i < N; ++i )
			{
				char cmdChk[1000];
				sprintf(cmdChk, " select * from hfFilters300 "
					" where idate = %d and idx = %d ",
					dbdate, i);

				vector<vector<string> > vvChk;
				GODBC::Instance()->get(dbname_)->ReadTable(cmdChk, &vvChk);

				if( !vvChk.empty() )
				{
					sprintf(cmd3, "update hfFilters300 set %s = %f "
						" where idate = %d and idx = %d ",
						colname.c_str(), coeff[0][i],
						dbdate, i);
				}
				else
				{
					sprintf(cmd3, " insert into hfFilters300 (idate, idx, %s) "
						" values (%d, %d, %f) ",
						colname.c_str(), dbdate, i, coeff[0][i]);
				}
				GODBC::Instance()->get(dbname_)->ExecDirect(cmd3);
			}
		}
	}
}

void MWriteFilters::endDay()
{
}

void MWriteFilters::endJob()
{
}

string MWriteFilters::get_colname(const string& model02)
{
	string colname = "";
	if( "AT" == model02 )
		colname = "tsexPtsex";
	else if( "AH" == model02 )
		colname = "hkxPhkx";
	else if( "AS" == model02 )
		colname = "auxPaux";
	else if( "KR" == model02 )
		colname = "kqsxPkqsx";
	else if( "AW" == model02 )
		colname = "twxPtwx";
	else if( "AG" == model02 )
		colname = "sgxPsgx";
	else if( "SS" == model02 )
		colname = "brxPbrx";
	else if( "MJ" == model02 )
		colname = "jnbpjnb";
	else if( "EU" == model02 )
		colname = "euxPeux";
	else if( "AA" == model02 )
		colname = "shgxPshgx";

	return colname;
}

void MWriteFilters::print_coeff(const string& colname, const vector<vector<float> >& coeff)
{
	int N = coeff[0].size();
	if( N > 6 )
		printf("%s %.3f, %.3f, %.3f, ..., %.3f, %.3f\n", colname.c_str(), coeff[0][0], coeff[0][1], coeff[0][2], coeff[0][N - 2], coeff[0][N - 1]);
}


bool MWriteFilters::horizon_match(const string& title, int horizon)
{
	int pos0 = title.find(";");
	int pos1 = title.find(";", pos0 + 1);
	int pos2 = title.find(";", pos1 + 1);

	bool match = false;
	if( pos1 > 0 && pos2 > pos0 )
	{
		string shorizon = title.substr(pos1 + 1, pos2 - pos1 - 1);
		if( horizon == atoi(shorizon.c_str()) )
			match = true;
	}
	return match;
}
