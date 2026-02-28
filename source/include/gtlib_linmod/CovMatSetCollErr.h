#ifndef __gtlib_linmod_CovMatSetCollErr__
#define __gtlib_linmod_CovMatSetCollErr__
#include <gtlib_linmod/CovMatSet.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>

namespace gtlib {

class CovMatSetCollErr {
public:
	CovMatSetCollErr();
	int nHorizon;
	int nTicker;
	int nSig;
	std::vector<std::string> vTicker;
	std::vector<std::unordered_map<std::string, CovMatSet>> vmCovMatSet;

	const CovMatSet& getCovMatSet(int iH, const std::string& ticker);
	friend std::istream& operator >>(std::istream& is, CovMatSetCollErr& obj);
};

} // namespace gtlib

#endif

