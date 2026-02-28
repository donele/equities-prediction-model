#include <gtlib_linmod/CovMatSetCollUniv.h>
using namespace std;

namespace gtlib {

CovMatSetCollUniv::CovMatSetCollUniv(){}

const CovMatSet& CovMatSetCollUniv::getCovMatSet(int iH, int iT)
{
	return vvCovMatSet[iH][iT];
}

istream& operator >>(std::istream& is, CovMatSetCollUniv& obj)
{
	is >> obj.nHorizon >> obj.nTimeSlice >> obj.nSig;
	obj.vvCovMatSet = vector<vector<CovMatSet>>(obj.nHorizon, vector<CovMatSet>(obj.nTimeSlice, CovMatSet(obj.nSig)));
	for( int iH = 0; iH < obj.nHorizon; ++iH )
	{
		for( int iT = 0; iT < obj.nTimeSlice; ++iT )
		{
			is >> obj.vvCovMatSet[iH][iT];
		}
	}
	return is;
}

} // namespace gtlib
