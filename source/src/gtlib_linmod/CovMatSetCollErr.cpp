#include <gtlib_linmod/CovMatSetCollErr.h>
using namespace std;

namespace gtlib {

CovMatSetCollErr::CovMatSetCollErr(){}

const CovMatSet& CovMatSetCollErr::getCovMatSet(int iH, const string& ticker)
{
	return vmCovMatSet[iH][ticker];
}

istream& operator >>(std::istream& is, CovMatSetCollErr& obj)
{
	is >> obj.nHorizon >> obj.nTicker >> obj.nSig;
	obj.vmCovMatSet = vector<unordered_map<string, CovMatSet>>(obj.nHorizon, unordered_map<string, CovMatSet>());
	string ticker;
	for( int iH = 0; iH < obj.nHorizon; ++iH )
	{
		for( int iT = 0; iT < obj.nTicker; ++iT )
		{
			is >> ticker;
			obj.vTicker.push_back(ticker);
			CovMatSet cSet(obj.nSig);
			is >> cSet;
			obj.vmCovMatSet[iH][ticker] = cSet;
		}
	}
	return is;
}

} // namespace gtlib
