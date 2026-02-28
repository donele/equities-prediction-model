#ifndef __cme__
#define __cme__
#include <jl_lib/crossCompile.h>
#include <string>
#include <vector>

namespace cme
{
	FUNC_DECLSPEC std::string month_code(int idate);
	FUNC_DECLSPEC std::vector<int> contract_months(std::string product);
	FUNC_DECLSPEC std::string front_ticker(std::string product, int idate);
}

#endif