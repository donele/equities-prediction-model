#ifndef __gtlib_linmod_CovMatSet__
#define __gtlib_linmod_CovMatSet__
#include <jl_lib/jlutil.h>
#include <vector>
#include <iostream>

namespace gtlib {

class CovMatSet {
public:
	CovMatSet(){}
	CovMatSet(int nSig);
	std::vector<Corr> vCorr;
	std::vector<std::vector<Corr>> vvCorr;
	std::vector<Acc> vAcc;
	Acc accY;

	bool empty() const;
	friend std::istream& operator >>(std::istream& is, CovMatSet& obj);
};

} // namespace gtlib

#endif
