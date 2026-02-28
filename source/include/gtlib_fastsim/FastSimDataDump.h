#ifndef __gtlib_sigread_FastSimDataDump__
#define __gtlib_sigread_FastSimDataDump__
#include <gtlib_sigread/SignalBuffer.h>
#include <gtlib_predana/PredWholeDay.h>
#include <gtlib_fastsim/fastsimObjs.h>

namespace gtlib {

	enum {
		HEADER = 1,
		SAMPLE = 2
	};

	struct EpisodeHeader {
		int flag;
		int idate;
		uint32_t length;
		char ticker [8];
		EpisodeHeader():flag(HEADER){};
	};

	struct EpisodeSampleRaw {
		int flag;
		uint64_t usecs;
		float bid;
		float ask;
		int bidSize;
		int askSize;
		float pred1;
		float pred2;
		float pred3;
		float pred4;
		EpisodeSampleRaw():flag(SAMPLE){};
	};

class FastSimDataDump {
public:
	FastSimDataDump():debug_(false){}
	FastSimDataDump(const std::string& baseDir, const std::string& fitDir,
			const std::vector<std::string>& vPredModel, const std::vector<std::string>& vSigModel,
			const std::vector<std::string>& vTargetName);
	~FastSimDataDump(){}
	void setDebug(bool x);
	void endDay(int udate, int idate,
		const std::map<std::string, std::string>* mTicker2Uid,
			const std::map<std::string, std::vector<double>>* mMid);

protected:
	bool debug_;
	int verbose_;
	int nTarget_;
	int openMsecs_;
	int closeMsecs_;
	int udate_;
	std::string baseDir_;
	std::vector<std::string> vPredModel_;
	std::vector<std::string> vSigModel_;
	std::vector<std::string> vTargetName_;
	std::string statDir_;
	std::string model_;
	std::string market_;
	std::map<std::string, gtlib::TickerData> mTickerData_;
	std::map<int, DailySimStat> mIdateStat_;

	gtlib::SignalBuffer* getBinSigBuf(const std::string& baseDir,
			const std::string& predModel, const std::string& sigModel,
			const std::string& targetName, int udate, int idate);

	gtlib::EpisodeSampleRaw getSample(std::vector<PredWholeDay*>& vPredDay, const std::string& ticker, int msecs,
			const std::vector<double>& vMid);

	void getData(int idate, std::vector<PredWholeDay*>& vPredDay,
			const std::map<std::string, std::string>* mTicker2Uid,
			const std::map<std::string, std::vector<double>>* mMid);
};

} // namespace gtlib

#endif

