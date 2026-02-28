#ifndef __MFastSimDataDump__
#define __MFastSimDataDump__
#include <MFramework.h>
#include <gtlib_fastsim/fastsimObjs.h>
#include <gtlib_fastsim/FastSimDataDump.h>
#include <string>
#include <vector>

class CLASS_DECLSPEC MFastSimDataDump: public MModuleBase {
public:
	MFastSimDataDump(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MFastSimDataDump();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	std::string ticker_;
	std::string coefModel_;
	std::string fitDir_;
	std::string coefFitDir_;
	std::vector<std::string> targetNames_;
	std::vector<std::string> sigModels_;
	std::vector<std::string> predModels_;
	std::vector<int> fitdates_;
	std::vector<int> udates_;
	gtlib::FastSimDataDump* pfs_;
};

#endif

