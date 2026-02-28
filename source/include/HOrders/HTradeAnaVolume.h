#ifndef __HTradeAnaVolume__
#define __HTradeAnaVolume__
#include <HLib/HModule.h>
#include <jl_lib/jlutil.h>
#include <boost/thread.hpp>
#include "TH1.h"
#include <map>
#include <string>
#include <vector>
#include <list>

class CLASS_DECLSPEC HTradeAnaVolume: public HModule {
public:
	HTradeAnaVolume(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HTradeAnaVolume();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	struct CloseInfo {
		CloseInfo(){}
		CloseInfo(double close_):close(close_), pos(0), nextOpen(0), nextClose(0){}
		double close;
		int pos;
		double nextOpen;
		double nextClose;
	};

	bool debug_;
	int verbose_;
	double costMult_;
	int iDay_;
	double nHours_;
	double fee_bpt_;
	int idate0_;
	int nP_;
	std::string tradeModuleName_;
	std::map<std::string, CloseInfo> mClose_;
	boost::mutex e_mutex_;
	boost::mutex c_mutex_;
	double exchrat_;

	std::string id_;
	std::string msecName_;
	std::string prcName_;
	std::string volName_;
	std::string signName_;
	std::string midName_;
	std::string posName_;
	std::string sampleOpenName_;

	bool isExpr_;
	std::set<int> cdates_;
	std::set<int> edates_;
	std::map<std::string, Acc> mTickerMedvol_;
	std::map<std::string, Acc> cmTickerPnl_;
	std::map<std::string, Acc> emTickerPnl_;
	std::map<std::string, Acc> cmTickerPnlTot_;
	std::map<std::string, Acc> emTickerPnlTot_;

	void readClosePricePosition();
	void intra_day_stat(const std::vector<double>* msecT, const std::vector<double>* prcT, const std::vector<double>* volT,
		const std::vector<double>* signT, const std::vector<double>* midT,
		const std::vector<double>* msecQ, const std::vector<double>* midQ, std::string ticker);
};


#endif