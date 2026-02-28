#ifndef __MSignal_MTopBottomAna__
#define __MSignal_MTopBottomAna__
#include <MFramework/MModuleBase.h>
#include <gtlib_sigread/SignalBuffer.h>
#include <gtlib_sigread/SignalWholeDay.h>
#include <map>
#include <string>
#include <vector>
#include <fstream>

class MTopBottomAna: public MModuleBase {
public:
	MTopBottomAna(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MTopBottomAna();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	int nInputs_;
	int printVarIndex_;
	std::ofstream ofs_;
	std::string sigType_;
	std::string targetName_;
	std::vector<std::string> inputNames_;

	gtlib::SignalBuffer* getBinSigBuf(const std::string& model, int idate);
	void printHeader();
	void printRow(gtlib::SignalBuffer* pSigBuf);
};

#endif
