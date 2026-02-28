#ifndef __MSignal_MDumpSignalDL__
#define __MSignal_MDumpSignalDL__
#include <MFramework/MModuleBase.h>
#include <gtlib_sigread/SignalBuffer.h>
#include <gtlib_sigread/SignalWholeDay.h>
#include <map>
#include <string>
#include <vector>

class MDumpSignalDL: public MModuleBase {
public:
	MDumpSignalDL(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MDumpSignalDL();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool selectHighVolTicker_;
	bool useSimpleTicker_;
	int nInputs_;
	int nDatesTickerSel_;
	double retain_;
	std::string sigType_;
	std::string fitDesc_;
	std::string targetName_;
	std::string outDir_;
	std::vector<std::string> inputNames_;
	std::vector<std::string> allTickers_;
	std::set<std::string> highVolTickers_;

	void getAllTickers();
	void setHighVolTickers();
	void printTickerList();
	bool validTicker(const std::string& ticker);
	gtlib::SignalBuffer* getBinSigBuf(const std::string& sigModel, const std::string& predModel, int idate);
	void dump(gtlib::SignalWholeDay& sig, const std::string& model, int idate);
	std::string getPredSigFilename(const std::string& model, int idate);
	std::string getSigFilename(const std::string& model, int idate);
	std::string getTarFilename(const std::string& model, int idate);
	std::string getRestarFilename(const std::string& model, int idate);
	std::string getMetaFilename(const std::string& model, int idate);
	std::string getWgtFilename(const std::string& model, int idate);
	std::vector<int> getMsecs(const std::map<std::string, std::vector<int>>& tm);
};

#endif
