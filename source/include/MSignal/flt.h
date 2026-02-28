#ifndef __flt__
#define __flt__
#include <gtlib_model/hff.h>
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
std::vector<float> get_data(const std::string& name, int idate);
FUNC_DECLSPEC bool valid_data(std::vector<const std::vector<ReturnData>*>& vp);
FUNC_DECLSPEC std::vector<int> get_idates(int ndays, int idate);
FUNC_DECLSPEC std::string get_filter_dir(const hff::IndexFilter& Filter, const std::string& model);
FUNC_DECLSPEC std::string get_filter_path(const hff::IndexFilter& Filter, const std::string& model, int idate);
FUNC_DECLSPEC std::string get_filter_path_ols(const hff::IndexFilter& Filter, const std::string& model, int idate, int lag0 = 0, int lagMult = 0);
FUNC_DECLSPEC std::string get_beta_dir(const std::string& indexName, const std::string& model);
std::string get_beta_path(const std::string& indexName, const std::string& model, int idate);
}

#endif
