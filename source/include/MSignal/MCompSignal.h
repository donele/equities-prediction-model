#ifndef __MSignal_MCompSignal__
#define __MSignal_MCompSignal__
#include <MFramework/MModuleBase.h>
#include <gtlib_sigread/SignalBuffer.h>
#include <gtlib_sigread/SignalWholeDay.h>
#include <map>
#include <string>
#include <vector>

class MCompSignal: public MModuleBase {
public:
	MCompSignal(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MCompSignal();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool useSimpleTicker_;
	bool compByTicker_;
	bool printBmpred_;
	bool printPred_;
	int nInputs_;
	int printVarIndex_;
	std::string sigType_;
	std::string fitDesc_;
	std::string txtPath_;
	std::string compModel_;
	std::string targetName_;
	std::string sigModel_;
	std::string predModel_;
	std::vector<std::string> inputNames_;

	bool doCompPred() const;
	gtlib::SignalBuffer* getCompSigBuf(int idate);
	gtlib::SignalBuffer* getBinSigBuf(const std::string& model, int idate);
	gtlib::SignalBuffer* getBinSigBuf(const std::string& sigModel, const std::string& predModel, int idate);
	void compareAllData(gtlib::SignalWholeDay& sig1, gtlib::SignalWholeDay& sig2);
	void compareByTicker(gtlib::SignalWholeDay& sig1, gtlib::SignalWholeDay& sig2);
	void printCompInput(const std::vector<Corr>& vCorr);
	void printCompTarget(const Corr& corrTarget);
	void printCompPred(const Corr& corrPred, const Corr& corrTargetPred1,
			const Corr& corrTargetPred2, const std::string& predDesc);
	void printCompInputByTicker(const std::map<std::string, std::vector<Corr>>& mvCorr);
	void printCompPredByTicker(const std::map<std::string, Corr>& mCorr,
		const std::map<std::string, Corr>& mCorrTargetPred1, const std::map<std::string, Corr>& mCorrTargetPred2);
};

#endif
