#ifndef __MPasteSample__
#define __MPasteSample__
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>
#include <map>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MPasteSample: public MModuleBase {
public:
	MPasteSample(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MPasteSample();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	int nInput_;
	int nTargets_;
	int cntFitData_;
	int cntOOSData_;
	std::string sigType_;
	std::string fitDescBase_;
	std::string fitDescAdd_;
	std::string modelBase_;
	std::string modelAdd_;

	std::string sigDir_;
	std::string txtDir_;

	void read_data(int idate, const std::string& model, const std::string& fitdesc, hff::SignalLabel& sh,
		std::map<std::string, std::map<int, std::vector<float> > >& m, std::map<std::string, std::map<int, std::string> >& mTxt);
	void write_data(int idate, const std::string& model, const std::string& fitdesc, const hff::SignalLabel& shBase, const hff::SignalLabel& shAdd,
		std::map<std::string, std::map<int, std::vector<float> > >& mBase, std::map<std::string, std::map<int, std::vector<float> > >& mAdd,
		std::map<std::string, std::map<int, std::string> >& mTxt);
};

#endif
