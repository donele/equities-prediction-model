#ifndef __gtlib_sigread_FastSim__
#define __gtlib_sigread_FastSim__
#include <gtlib_sigread/SignalBuffer.h>
#include <gtlib_predana/PredWholeDay.h>
#include <gtlib_fastsim/fastsimObjs.h>

namespace gtlib {

class FastSim {
public:
	FastSim():debug_(false){}
	FastSim(const std::string& name, const std::string& baseDir, const std::string& fitDir,
			const std::vector<std::string>& vPredModel, const std::vector<std::string>& vSigModel,
			const std::vector<std::string>& vTargetName, const std::vector<double>& vWgt,
			double restore, double thres, int maxShrTrade, double maxDollarPosNet, double maxDollarPosTicker);
	~FastSim(){}
	void setDebug(const bool& x);
	void setIPred(const std::vector<int>& v);
	void endDay(std::vector<PredWholeDay*>& vPredDay,
			int udate, int idate, const std::map<std::string, gtlib::CloseInfo>* mClose,
			const std::map<std::string, std::string>* mTicker2Uid,
			const std::map<std::string, std::vector<double>>* mMid);
	void endDay(int udate, int idate, const std::map<std::string, gtlib::CloseInfo>* mClose,
			const std::map<std::string, std::string>* mTicker2Uid,
			const std::map<std::string, std::vector<double>>* mMid);
	void print();

protected:
	bool debug_;
	int verbose_;
	int maxShrTrade_;
	int nTarget_;
	int openMsecs_;
	int closeMsecs_;
	static int cntPrint_;
	double restore_;
	double thres_;
	double netDollarPos_;
	double maxDollarPosNet_;
	double maxDollarPosTicker_;
	int udate_;
	std::string name_;
	std::string baseDir_;
	std::vector<std::string> vPredModel_;
	std::vector<std::string> vSigModel_;
	std::vector<std::string> vTargetName_;
	std::string statDir_;
	std::string model_;
	std::string market_;
	std::vector<double> vWgt_;
	std::vector<int> viPred_;
	std::map<std::string, gtlib::TickerData> mTickerData_;
	std::map<int, DailySimStat> mIdateStat_;

	gtlib::SignalBuffer* getBinSigBuf(const std::string& baseDir,
			const std::string& predModel, const std::string& sigModel,
			const std::string& targetName, int udate, int idate);
	gtlib::DailySimStat getStat(int idate,
		const std::map<std::string, gtlib::CloseInfo>* mClose,
		const std::map<std::string, std::string>* mTicker2Uid);
	void getOvernightInfo(gtlib::DailySimStat& dss,
		const std::map<std::string, gtlib::CloseInfo>* mClose,
		const std::map<std::string, std::string>* mTicker2Uid);
	void getEnddayInfo(gtlib::DailySimStat& dss,
		const std::map<std::string, gtlib::CloseInfo>* mClose,
		const std::map<std::string, std::string>* mTicker2Uid);
	gtlib::SampleData getSample(std::vector<PredWholeDay*>& vPredDay, const std::string& ticker, int msecs,
			const std::vector<double>& vMid);
	void printSummDay(int idate, const gtlib::DailySimStat& x);

	virtual void getData(std::vector<PredWholeDay*>& vPredDay,
			const std::map<std::string, std::string>* mTicker2Uid,
			const std::map<std::string, std::vector<double>>* mMid) = 0;
	virtual void getIntradayInfo(gtlib::DailySimStat& dss,
		const std::map<std::string, gtlib::CloseInfo>* mClose,
		const std::map<std::string, std::string>* mTicker2Uid) = 0;
};

} // namespace gtlib

#endif

