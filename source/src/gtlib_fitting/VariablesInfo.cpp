#include <gtlib_fitting/VariablesInfo.h>
#include <string>
using namespace std;

namespace gtlib {

int VariablesInfo::nTargets()
{
	return targetNames.size();
}

string VariablesInfo::fullTargetName()
{
	assert( targetWeights.empty() || targetNames.size() == targetWeights.size() );
	string fullName;
	char buf[100];
	int nT = nTargets();
	if( nT == 1 )
		fullName = targetNames[0];
	else if( nT > 1 )
	{
		for( int i = 0; i < nT; ++i )
		{
			if( i > 0 )
				fullName += (string)"_";
			if( targetWeights.empty() )
				sprintf(buf, "%s", targetNames[i].c_str());
			else
				sprintf(buf, "%s_%.1f", targetNames[i].c_str(), targetWeights[i]);
			fullName += buf;
		}
	}
	int nP = bmpredNames.size();
	for( int i = 0; i < nP; ++i )
	{
		fullName += (string)"_x";
		fullName += bmpredNames[i];
	}
	return fullName;
}

int VariablesInfo::maxHorizon()
{
	int maxHorizon = 0;
	for( auto it = targetNames.begin(); it != targetNames.end(); ++it )
	{
		int N = it->size();
		int pos_semicolon = it->find(";");
		if( N > 3 && it->substr(0, 3) == "tar" )
		{
			int horizon = atoi(it->substr(3, pos_semicolon - 3).c_str()) + atoi(it->substr(pos_semicolon + 1, N - pos_semicolon).c_str());
			if( horizon > maxHorizon )
				maxHorizon = horizon;
		}
		else if( N > 6 && it->substr(0, 6) == "restar" )
		{
			int horizon = atoi(it->substr(3, pos_semicolon - 6).c_str()) + atoi(it->substr(pos_semicolon + 1, N - pos_semicolon).c_str());
			if( horizon > maxHorizon )
				maxHorizon = horizon;
		}
	}
	return maxHorizon;
}

} // namespace gtlib
