#ifndef __HNN_InputInfo__
#define __HNN_InputInfo__
#include <vector>
#include <string>
#include <iostream>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

namespace hnn {
	struct CLASS_DECLSPEC InputInfo {
		InputInfo(){}
		InputInfo(const InputInfo& nni);
		InputInfo(std::string filename);
		InputInfo(int n_):n(n_), mean(std::vector<double>(n)), stdev(std::vector<double>(n)){}
		int n;
		std::vector<double> mean;
		std::vector<double> stdev;
	};
}

FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, hnn::InputInfo& obj);

#endif