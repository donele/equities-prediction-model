#ifndef __MReadSignalON__
#define __MReadSignalON__
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>
#include <MFitting/HFTrees.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MReadSignalON: public MModuleBase {
public:
	MReadSignalON(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MReadSignalON();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	int ndays_;
	int minMsecs_;
	int maxMsecs_;
	double weight_;
	std::string targetName_;

	int nSpectator_;
	int cntFitData_;
	int cntOOSData_;
	std::string fitDesc_;
	std::string sigType_;
	std::map<int, int> vNrowsFit_;
	std::map<int, int> vNrowsOOS_;
	std::vector<std::string> spectatorNames_;

	std::vector<float> vSpectator_;
	std::vector<float> vSpectatorOOS_;
	std::vector<std::vector<std::vector<float> > > vvvSpectatorOOS_;
	std::map<int, std::pair<int, int> > fitOffset_;
	std::map<int, std::pair<int, int> > oosOffset_;
};

#endif
