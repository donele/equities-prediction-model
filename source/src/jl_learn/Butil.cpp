#include <jl_learn/Butil.h>
#include <jl_lib/jlutil.h>
#include <algorithm>
#include <functional>
using namespace std;

ostream& operator <<(ostream& os, BnormInfo& obj)
{
	os << obj.n << "\t";
	os.precision(4);
	copy( obj.mean.begin(), obj.mean.end(), ostream_iterator<double>(os, "\t"));
	copy( obj.stdev.begin(), obj.stdev.end(), ostream_iterator<double>(os, "\t"));

	return os;
}

BnormInfo::BnormInfo(string filename)
{
	ifstream ifs(filename.c_str());
	ifs >> n;
	mean = vector<double>(n);
	stdev = vector<double>(n);
	for( int i = 0; i < n; ++i )
		ifs >> mean[i];
	for( int i = 0; i < n; ++i )
		ifs >> stdev[i];
}

int Butil::get_nInput(int iModel)
{
	if( 0 == iModel ) // test with artificial data.
		return 10;
	else if( 1 == iModel ) // Input(1m, 5m, 10m, quoteImb, spread) Target(1m)
		return 5;
	else if( 2 == iModel || 3 == iModel ) // Input(1m, 5m, 10m, quoteImb, spread, previous output) Target(1m)
		return 6;
	else if( 4 == iModel )
		return 6;

	return 0;
}

int Butil::get_nOutput(int iModel)
{
	if( 4 >= iModel )
		return 1;

	return 0;
}

string Butil::get_inputName(int iModel, int i)
{
	if( 0 == iModel )
	{
		if( i < 10 )
		{
			char name[40];
			sprintf(name, "rtn_%d", i+1);
			return name;
		}
		else
			return "unnamed";
	}
	else if( 1 == iModel || 2 == iModel || 3 == iModel )
	{
		if( 0 == i )
			return "rtn_1m";
		else if( 1 == i )
			return "rtn_5m";
		else if( 2 == i )
			return "rtn_10m";
		else if( 3 == i )
			return "quote_imb";
		else if( 4 == i )
			return "spread";
		else if( 5 == i && 2 == iModel )
			return "prev_out";
		else
			return "unnamed";
	}
	else if( 4 == iModel )
	{
		 if( 0 == i )
			return "spread";
		else if( 1 == i )
			return "bid_size";
		else if( 2 == i )
			return "ask_size";
		else if( 3 == i )
			return "rtn_1m";
		else if( 4 == i )
			return "rtn_5m";
		else if( 5 == i )
			return "rtn_10m";
		else
			return "unnamed";
	}

	return "unknown";
}