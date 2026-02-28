#include <gtlib_model/mdl.h>
#include <iostream>
using namespace std;

namespace gtlib {
namespace mdl {

vector<string> markets(const string& model)
{
	vector<string> markets;
	if( model.size() >= 2 )
	{
		string baseModel = model.substr(0, 2);
		if( baseModel == "US" )
			markets = {"US"};
		else if( baseModel == "CA" )
			markets = {"CJ"};
		else if( baseModel == "EU" )
			markets = {"EA", "EB", "EI", "EP", "EF", "EL", "ED", "EM", "EZ", "EO", "EX", "EC", "EW", "EY"};
		else if( baseModel == "KR" )
			markets = {"AK", "AQ"};
		else
		{
			//cerr << "mdl::markets(): unknown model " << model << endl;
			//exit(28);
			markets = {baseModel};
		}
	}
	return markets;
}

string selModelMarkets(const string& model)
{
	string sel;
	string baseModel = model.substr(0, 2);
	vector<string> mkts = markets(baseModel);
	if( baseModel == "US" || baseModel == "CA" )
		;
	else if( mkts.size() == 1 )
	{
		char buf[200];
		sprintf(buf, " exchange = '%c' ", mkts[0][1]);
		sel = buf;
	}
	else
	{
		sel = " exchagne in (";
		int N = mkts.size();
		for( int i = 0; i < N; ++i )
		{
			if( i > 0 )
				sel += ",";
			sel += ("'" + mkts[i].substr(1, 1) + "'");
		}
		sel += ")";
	}
	return sel;
}

} // namespace mdl
} // namespace gtlib
