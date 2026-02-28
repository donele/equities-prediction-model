#ifndef __MMakePredIndex__
#define __MMakePredIndex__
#include <MFramework/MModuleBase.h>
#include <MFramework.h>
#include <gtlib_linmod/ARMulti.h>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MMakePredIndex: public MModuleBase {
public:
	enum TransformFunctions {None, Asinh, CubicRoot};
	MMakePredIndex(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MMakePredIndex();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

	void get_filter();
	void read_filter();
	void make_prediction();

private:
	bool debug_;
	int verbose_;
	//std::string transform_;
	TransformFunctions transform_;;
	std::string fmodel_;
	std::string fitAlg_;
	std::vector<gtlib::ARMulti> ar_;
	float sum_returns(std::vector<ReturnData>::const_iterator& it1, std::vector<ReturnData>::const_iterator& it2);
	float getTransform(float pred);
};

#endif
