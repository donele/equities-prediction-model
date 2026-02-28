#ifndef __gtlib_linmod_CovMatSetCollUniv__
#define __gtlib_linmod_CovMatSetCollUniv__
#include <gtlib_linmod/CovMatSet.h>
#include <vector>
#include <string>
#include <iostream>

namespace gtlib {

class CovMatSetCollUniv {
public:
	CovMatSetCollUniv();
	int nHorizon;
	int nTimeSlice;
	int nSig;
	std::vector<std::vector<CovMatSet>> vvCovMatSet;

	const CovMatSet& getCovMatSet(int iH, int iT);
	friend std::istream& operator >>(std::istream& is, CovMatSetCollUniv& obj);
};

} // namespace gtlib

#endif
