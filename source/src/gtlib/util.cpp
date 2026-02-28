#include <gtlib/util.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GEX.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/mto.h>
#include <iostream>
#include <vector>
#include <string>
using namespace std;

namespace gtlib {

string concat(const vector<string>& v)
{
	string ret;
	for( const auto& str : v )
	{
		if( ret.empty() )
			ret = str;
		else
			ret += "_" + str;
	}
	return ret;
}

string firstToUpper(const string& str)
{
	string ret = str;
	if( !ret.empty() )
		ret[0] = static_cast<char>(toupper(str[0]));
	return ret;
}

vector<float> getNextLineSplitFloat(istream& is)
{
	string line;
	getline(is, line);
	vector<string> sl = split(line);
	vector<float> sf;
	for( string s : sl )
		sf.push_back(atof(s.c_str()));
	return sf;
}

vector<string> getNextLineSplitString(istream& is)
{
	string line;
	getline(is, line);
	return split(line);
}

vector<char> getConfigValuesChar(const multimap<string, string>& conf, string id)
{
	vector<char> v;
	if( conf.count(id) )
	{
		auto inputs = conf.equal_range(id);
		for( auto mi = inputs.first; mi != inputs.second; ++mi )
		{
			string val = mi->second;
			if(val.size() == 1)
				v.push_back(val[0]);
			else
				exit(17);
		}
	}
	return v;
}

vector<string> getConfigValuesString(const multimap<string, string>& conf, string id)
{
	vector<string> v;
	if( conf.count(id) )
	{
		auto inputs = conf.equal_range(id);
		for( auto mi = inputs.first; mi != inputs.second; ++mi )
		{
			string val = mi->second;
			v.push_back(val);
		}
	}
	return v;
}

vector<int> getConfigValuesInt(const multimap<string, string>& conf, string id)
{
	vector<int> v;
	if( conf.count(id) )
	{
		auto inputs = conf.equal_range(id);
		for( auto mi = inputs.first; mi != inputs.second; ++mi )
		{
			int val = atoi(mi->second.c_str());
			v.push_back(val);
		}
	}
	return v;
}

vector<double> getConfigValuesDouble(const multimap<string, string>& conf, string id)
{
	vector<double> v;
	if( conf.count(id) )
	{
		auto inputs = conf.equal_range(id);
		for( auto mi = inputs.first; mi != inputs.second; ++mi )
		{
			double val = atof(mi->second.c_str());
			v.push_back(val);
		}
	}
	return v;
}

vector<float> getConfigValuesFloat(const multimap<string, string>& conf, string id)
{
	vector<float> v;
	if( conf.count(id) )
	{
		auto inputs = conf.equal_range(id);
		for( auto mi = inputs.first; mi != inputs.second; ++mi )
		{
			float val = atof(mi->second.c_str());
			v.push_back(val);
		}
	}
	return v;
}

} // namespace gtlib
