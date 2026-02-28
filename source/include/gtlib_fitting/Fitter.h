#ifndef __gtlib_fitting_Fitter__
#define __gtlib_fitting_Fitter__
#include <gtlib_fitting/FitData.h>
#include <vector>
#include <string>

namespace gtlib {

class Fitter {
public:
	Fitter();
	virtual ~Fitter(){};
	virtual void fit() = 0;
	virtual void writeModel(const std::string& path) = 0;
	virtual double pred(const std::vector<float>& input) = 0;
	virtual double pred(int iSample) = 0;
};

}
#endif
