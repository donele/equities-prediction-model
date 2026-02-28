#include <jl_lib/mto.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/Arg.h>
#include <string>
using namespace std;

void exit_usage()
{
	cout << "Usage: db -d <dbname> -q \"<query>\" -f <field-separator>\n";
	cout << "Usage: db -d <dbname> -t <tablename>\n";
	exit(1);
}

void print(vector<vector<string> >& vv, string sep)
{
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		for(vector<string>::iterator it2 = it->begin(); it2 != it->end(); ++it2 )
		{
			if( it2 == it->begin() )
				cout << trim(*it2);
			else
				cout << sep << trim(*it2);
		}
		cout << "\n";
	}
}

int main(int argc, char* argv[])
{
	if( argc < 2 )
		exit_usage();
	Arg arg(argc, argv);

	string db = arg.getVal("d");
	string table = arg.getVal("t");
	string sep = ",";
	if( arg.isGiven("f") )
		sep = arg.getVal("f");

	string query = "";
	if( arg.isGiven("q") )
	{
		vector<string> vq = arg.getVals("q");
		for( string q : vq )
			query += " " + q;
	}
	else if( arg.isGiven("f") )
	{
		string filename = arg.getVal("f");
		ifstream ifs(filename.c_str());
		if( ifs.is_open() )
		{
			string piece = "";
			while( ifs >> piece )
			{
				query += " " + piece;
			}
		}
	}

	if( !db.empty() && !query.empty() )
	{
		vector<vector<string> > vv;
		GODBC::Instance()->read(db, query, vv);
		print(vv, sep);
	}
	else if( !db.empty() && !table.empty() )
	{
		vector<vector<string> > vv;
		string query = "select column_name from information_schema.columns where table_name = '" + table + "'";
		GODBC::Instance()->read(db, query, vv);
		print(vv, sep);
	}
	return 0;
}
