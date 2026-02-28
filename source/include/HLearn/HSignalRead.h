#ifndef __HSignalRead__
#define __HSignalRead__
#include <HLearnObj/Signal.h>
#include <HLearnObj/Sample.h>
#include <boost/thread.hpp>
#include "HLib.h"
#include "optionlibs/TickData.h"
#include "TH2.h"
#include "TStopwatch.h"
#include <string>
#include <map>
#include <set>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HSignalRead: public HModule {
public:
	HSignalRead(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HSignalRead();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	int iMarket_;
	int nInput_;
	int nSpectator_;
	bool normalize_;
	bool now_normalizing_;
	bool write_root_;
	bool do_hist_;
	int verbose_;
	int targetIndx_;
	std::string targetName_;
	std::string norm_file_;
	std::string sample_dir_;
	std::vector<std::string> inputNames_;
	std::vector<std::string> spectatorNames_;
	std::vector<int> inputIndx_;
	std::vector<int> spectatorIndx_;
	std::ifstream ifs_;
	boost::shared_ptr<boost::thread> thread_;
	std::set<std::string> sampleReadLog_;
	bool threadSampleRead_;

	TStopwatch swThreadRead_;

	std::vector<TH1F> h_input_;
	std::vector<TH2F> h_corr_;
	double input_n_;
	std::vector<double> input_sum_;
	std::vector<double> input_sum2_;

	void read_sample(std::string ticker);
	void normalize_sample(std::vector<hnn::Sample>& sample);
	void startThreadSampleRead();
};

#endif

