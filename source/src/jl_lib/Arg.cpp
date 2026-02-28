#include <jl_lib/Arg.h>
using namespace std;

Arg::Arg(int argc, char* argv[])
{
	for( int i=0; i<argc; ++i )
	{
		// find a key.
		string this_arg = argv[i];
		int lenArg = this_arg.size();
		if( lenArg > 1 && this_arg[0] == '-' )
		{
			// count the number of values for the key.
			int nval = 0;
			int j = i;
			while( ++j < argc )
			{
				if( argv[j][0] == '-' )
					break;
				else
					++nval;
			}

			// insert the values for the key.
			if( nval == 0 )
				amm_.insert(make_pair(this_arg.substr(1, lenArg-1), ""));
			else
			{
				for( int k=1; k<=nval; ++k )
					amm_.insert(make_pair(this_arg.substr(1, lenArg-1), argv[i+k]));
			}
		}
	}
}

Arg::~Arg()
{}

bool Arg::isGiven(const string& opt) const
{
	return amm_.count(opt);
}

string Arg::getVal(const string& opt) const
{
	string ret = "";
	if( isGiven(opt) )
		ret = amm_.find(opt)->second;
	return ret;
}

vector<string> Arg::getVals(const string& opt) const
{
	vector<string> ret;
	if( isGiven(opt) )
	{
		auto range = amm_.equal_range(opt);
		for( auto mi = range.first; mi != range.second; ++mi )
		{
			string val = mi->second;
			ret.push_back(val);
		}
	}

	return ret;
}
