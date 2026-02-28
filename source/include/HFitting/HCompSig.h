#ifndef __HCompSig__
#define __HCompSig__
#include <HLib/HModule.h>
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

class CLASS_DECLSPEC HCompSig: public HModule {
public:
	HCompSig(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HCompSig();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	bool debug_;
	std::string type_;
	std::string input_;
	std::vector<int> omtxtinput_;
	std::vector<int> ombininput_;
	std::vector<int> tmtxtinput_;
	std::vector<int> tmbininput_;

	std::vector<int> vVars1_;
	std::string frame1_;
	std::string model1_;
	int machine1_;
	std::string drive1_;

	std::vector<int> vVars2_;
	std::string frame2_;
	std::string model2_;
	int machine2_;
	std::string drive2_;
};

#endif