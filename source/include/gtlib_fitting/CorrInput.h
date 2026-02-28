#ifndef __CorrInput__
#define __CorrInput__
#include <gtlib_fitting/FitData.h>
#include <vector>
#include <map>
#include <jl_lib/crossCompile.h>

namespace gtlib {

class CorrInput {
public:
	CorrInput();
	CorrInput(FitData* fitData, const std::string& fitDir, int udate);
	void stat();

private:
	std::string statDir_;
	FitData* pFitData_;
};

}

#endif
