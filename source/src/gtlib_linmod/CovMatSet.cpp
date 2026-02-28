#include <gtlib_linmod/CovMatSet.h>
using namespace std;

namespace gtlib {

CovMatSet::CovMatSet(int nSig)
{
	vCorr = vector<Corr>(nSig);
	vvCorr = vector<vector<Corr> >(nSig);
	for( int i = 0; i < nSig; ++i )
		vvCorr[i] = vector<Corr>(i + 1);
	vAcc = vector<Acc>(nSig);
}

bool CovMatSet::empty() const
{
	return accY.n == 0;
}

istream& operator >>(std::istream& is, CovMatSet& obj)
{
	for( auto& corr : obj.vCorr )
		is >> corr;
	for( auto& vCorr : obj.vvCorr )
		for( auto& corr : vCorr )
			is >> corr;
	for( auto& acc : obj.vAcc )
		is >> acc;
	is >> obj.accY;
	return is;
}

} // namespace gtlib
