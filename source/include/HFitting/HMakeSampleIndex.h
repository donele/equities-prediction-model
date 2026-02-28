#ifndef __HMakeSampleIndex__
#define __HMakeSampleIndex__
#include <HLib/HModule.h>
#include <HLib.h>
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

class CLASS_DECLSPEC HMakeSampleIndex: public HModule {
public:
	HMakeSampleIndex(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HMakeSampleIndex();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	std::vector<ARMulti> ar_; // Debug.

	void make_sample(const hff::IndexFilter& filter, int n);
	bool valid_data(const std::vector<std::string>& filter);
};

#endif