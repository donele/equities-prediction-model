#ifndef __stat_util__
#define __stat_util__
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

FUNC_DECLSPEC float mean(const std::vector<float>& v);
FUNC_DECLSPEC double mean(const std::vector<double>& v);
FUNC_DECLSPEC float variance(const std::vector<float>& v);
FUNC_DECLSPEC double variance(const std::vector<double>& v);
FUNC_DECLSPEC float stdev(const std::vector<float>& v);
FUNC_DECLSPEC double stdev(const std::vector<double>& v);
FUNC_DECLSPEC float corr(const std::vector<float>& v1, const std::vector<float>& v2);
FUNC_DECLSPEC double corr(const std::vector<double>& v1, const std::vector<double>& v2);

FUNC_DECLSPEC double sqr(double x);

#endif
