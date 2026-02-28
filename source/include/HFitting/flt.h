#ifndef __flt__
#define __flt__
#include <string>
#include <vector>
#include <optionlibs/TickData.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

namespace flt {
	FUNC_DECLSPEC std::vector<const std::vector<ReturnData>*> get_data(const std::vector<std::string>& names, int idate);
	FUNC_DECLSPEC bool valid_data(std::vector<const std::vector<ReturnData>*>& vp);
	FUNC_DECLSPEC std::vector<int> get_idates(int ndays, int idate);
	FUNC_DECLSPEC std::string get_model_dir(const hff::IndexFilter& Filter, std::string fitAlg);
	FUNC_DECLSPEC std::string get_model_path(const hff::IndexFilter& Filter, std::string fitAlg, int idate);
}

#endif
