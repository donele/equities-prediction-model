#ifndef __MMergePred__
#define __MMergePred__
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

class CLASS_DECLSPEC MMergePred: public MModuleBase {
public:
	MMergePred(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MMergePred();

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
	std::string targetName_;
	std::string sigType_;
	std::string fitDescBase_;
	std::string fitDescAdd_;
	std::string modelBase_;
	std::string modelAdd_;

	std::string sigDir_;
	std::string txtDir_;

	void read_data(int idate, const std::string& model, const std::string& fitdesc,
		std::map<std::string, std::map<int, std::string> >& m, std::map<std::string, std::map<int, std::string> >& mTxt);
	void write_data(int idate, const std::string& model, const std::string& fitdesc,
		std::map<std::string, std::map<int, std::string> >& mBase, std::map<std::string, std::map<int, std::string> >& mAdd,
		std::map<std::string, std::map<int, std::string> >& mTxtBase, std::map<std::string, std::map<int, std::string> >& mTxtAdd);
	void write_all(std::ofstream& ofs, std::ofstream& ofsTxt, std::map<int, std::string>::iterator itt, std::map<int, std::string>::iterator ittTxt,
		std::map<int, std::string>::iterator theEnd);
	void write_one(std::ofstream& ofs, std::ofstream& ofsTxt, std::map<int, std::string>::iterator itt, std::map<int, std::string>::iterator ittTxt);
};

#endif
