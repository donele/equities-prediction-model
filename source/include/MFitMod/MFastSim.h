#ifndef __MFastSim__
#define __MFastSim__
#include <MFramework.h>
#include <gtlib_fastsim/fastsimObjs.h>
#include <gtlib_fastsim/FastSim.h>
#include <gtlib_sigread/SignalBufferDecoratorHeader.h>
#include <gtlib_sigread/SignalBufferPredMulti.h>
#include <string>
#include <vector>

class CLASS_DECLSPEC MFastSim: public MModuleBase {
public:
	MFastSim(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MFastSim();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	double restore_;
	double thres_;
	int maxShrTrade_;
	double tradeBias_;
	double maxDollarPosNet_;
	double maxDollarPosTicker_;
	std::string ticker_;
	std::string coefModel_;
	std::string fitDir_;
	std::string coefFitDir_;
	std::vector<std::string> targetNames_;
	std::vector<std::string> sigModels_;
	std::vector<std::string> predModels_;
	std::vector<double> weights_;
	std::vector<int> fitdates_;
	std::vector<int> udates_;
	std::vector<int> viPred_;
	gtlib::FastSim* pfs_;

	gtlib::SignalBuffer* getBinSigBuf(const std::string& baseDir, const std::string& predModel,
			const std::string& sigModel, const std::string& targetName, int udate, int idate);
};

#endif

