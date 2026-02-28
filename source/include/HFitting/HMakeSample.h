#ifndef __HMakeSample__
#define __HMakeSample__
#include <HLib/HModule.h>
#include <HFitting/sig.h>
#include <gtlib_linmod/BiLinearModel.h>
#include <HFitting/VarianceModel.h>
#include <HLib.h>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HMakeSample: public HModule {
public:
	HMakeSample(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HMakeSample();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginMarket();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endMarket();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	int verbose_;
	int pid_;
	int nThreads_;
	bool longTicker_;
	bool write_bmtxt_;
	bool write_omtxt_;
	bool write_ombin_;
	bool write_tmtxt_;
	bool write_tmbin_;
	bool debug_write_bin_;
	bool txt_corr_;
	boost::mutex om_mutex_;
	boost::mutex tm_mutex_;
	boost::mutex biLinMod_mutex_;
	boost::mutex varMod_mutex_;
	boost::mutex tta_mutex_;
	boost::mutex oba_mutex_;
	std::set<std::string> uids_;
	std::map<std::string, std::string> mTickerUid_;

	int mem_;
	int cntDay_;
	int idate_;
	int idate_p_;
	int idate_pp_;
	int idate_n_;
	bool hValid_;
	TickSources tSources_;
	std::vector<std::string> okECNs_;
	BiLinearModel biLinMod_;
	//VarianceModel varMod_;
	std::vector<AccMov> vAccMov_;
	std::vector<AccMov> vAccMovTarget_;
	sig::HedgeInfo hInfo_;
	std::vector<hff::SigC> vSig_;
	std::vector<int> vSigInUse_;

	Corr corrPr_;
	Corr corrPrTot_;
	Corr corrVar_;
	Corr corrBm0_;
	Corr corrBm1_;
	Corr corrBm2_;
	Corr corrBm3_;
	Corr corrBm11_;
	Corr corrBm12_;
	Corr corrBm13_;
	Corr corrT20T40_;
	Corr corrP20T20_;
	Corr corrP40T40_;
	Corr corrP20T_;
	Corr corrP40T_;
	Corr corrPgT_;

	int cntOmBin_;
	int cntTmBin_;
	std::ofstream bmTxt_;
	std::ofstream omBin_;
	std::ofstream omBinTxt_;
	std::ofstream omTxt_;
	std::ofstream tmBin_;
	std::ofstream tmBinTxt_;
	std::ofstream tmTxt_;

	hff::SigC& get_sig_object(int& iSig);
	void free_sig_object(int iSig);

	std::map<boost::thread::id, TickTobAcc*> mIdTta_;
	std::map<boost::thread::id, std::map<std::string, OrderBkAcc<OrderData> > > mIdObaMap_;

	//template<class T> void read_tickdata(std::vector<T>& quotes, std::string market, int idate, std::string ticker);
	void get_prediction(hff::SigC& sig, std::string uid, int idate);
	void write_signal(hff::SigC& sig, std::string uid, std::string ticker);
	void get_uid_map(std::string market, int idate_p);
	TickTobAcc* get_tta(std::string market, int idate);
	std::map<std::string, OrderBkAcc<OrderData> >& get_obaMap(std::string market, int idate);
	double get_bmpred(std::vector<float>& pred, std::vector<float>& predErr, std::vector<float>& predVar);
	double get_bmpred_fixed(std::vector<float>& pred, std::vector<float>& predErr, double power);
	double get_bmpred_var(std::vector<float>& pred, std::vector<float>& predErr, double power);
};

//template<class T>
//void HMakeSample::read_tickdata(std::vector<T>& v, std::string market, int idate, std::string ticker)
//{
//	TickAccessMulti<T> ta;
//	string dir = tSources_.nbbodirectory(market, idate);
//	ta.AddRoot(dir, longTicker_);
//
//	TickSeries<T> ts(200000, 1.2);
//	ta.GetTickSeries( ticker, idate, &ts );
//	int nTicks =  ts.NTicks();
//
//	v.reserve(nTicks);
//
//	ts.StartRead();
//	T object;
//	for( int i = 0; i < nTicks; ++i )
//	{
//		ts.Read(&object);
//		v.push_back(object);
//	}
//}

#endif
