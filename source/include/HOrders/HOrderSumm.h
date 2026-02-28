#ifndef __HOrderSumm__
#define __HOrderSumm__
#include <HLib.h>
#include <HOrders.h>
#include <jl_lib/MercatorOrder.h>
#include <boost/thread.hpp>
#include <map>
#include <TFile.h>
#include <TProfile.h>
#include <TH1.h>
#include <TH2.h>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HOrderSumm: public HModule {
public:
	HOrderSumm(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HOrderSumm();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	bool writeDB_;
	bool dailyJob_;
	int fakeMsecsMax_;
	int q2iMax_;
	int msecDirect_;
	double histMax_;
	char execType_;
	int orderSchedType_;
	char side_;
	int verbose_;
	std::string dest_;
	std::string basedir_;
	std::string region_;

	boost::mutex hist_mutex_;
	boost::mutex fillrat_mutex_;
	boost::mutex nOrd_mutex_;
	boost::mutex vio_mutex_;

	// Match
	TH1F h_quoteMatch_;

	// as function of time of day.
	TProfile p_q2o_;
	TProfile p_fillrat_;
	TH1F h_norders_;

	// times by day.
	TH1F h_q2i_;

	TH1F h_o2e_filled_;
	TH1F h_o2v_filled_;
	TH1F h_q2d_filled_;
	TH1F h_q2t_filled_;
	TH1F h_q2d_direct_filled_;
	TH1F h_q2o_filled_;
	TH1F h_q2i_filled_;
	TH1F h_i2x_filled_;

	TH1F h_o2e_notFilled_;
	TH1F h_o2v_notFilled_;
	TH1F h_q2d_notFilled_;
	TH1F h_q2t_notFilled_;
	TH1F h_q2o_notFilled_;
	TH1F h_q2i_notFilled_;
	TH1F h_i2x_notFilled_;
	TH1F h_d2i_notFilled_;

	TH2F h_d2i_f2o_notFilled_;
	TProfile p_fillrat_f2o_;

	// orders by day.
	double nOrd_total_;
	double nOrd_matched_;
	double nOrd_unmatched_;
	double nOrd_directFill_;
	double nOrd_insertNFill_;
	double nOrd_insertNCancel_;
	double nOrd_unidFill_;
	double nOrd_unidCancel_;

	// shares.
	double nShr_total_;
	double nShr_filled_;

	// Fill ratio.
	double nFilledFull_;
	double nFilledIncl_;
	double nTotal_;
	double dvFilled_;
	double dvTotal_;
	double nFilledOnFake_;
	double nTotalOnFake_;
	double nFilledOnNonFake_;
	double nTotalOnNonFake_;

	// Performance.
	double dv_;
	double dv_filled_;
	double dv_notfilled_;
	double dv_missed_;
	double profit_;
	double profit_filled_;
	double profit_notfilled_;
	double profit_missed_;

	void reset();
	void get_summary(double& o2efilled, double& q2dfilled, double& q2tfilled, double& q2dnotfilled, double& q2tnotfilled, double& d2inotfilled, double& q2i,
				double& nfakeNotFilled, double& nfakeFilled,
				double& ncompetition, double& fillratshr, double& fillratincl, double& fillratinclmatched, double& fillratinclunmatched, double& fillratnonfake,
				double& ratdirectfill, double& ratmatched, double& ratfake, double& ratcomp);
	void select_orders( std::vector<MercatorOrder>& orders, const std::vector<MercatorOrder>* vMOrder );

	void fill_hist( std::vector<MercatorOrder>& orders, std::string  ticker );
	void update_fillrat( std::vector<MercatorOrder>& orders );
	void update_nOrders( std::vector<MercatorOrder>& orders );

	double get_top5bin_avg(TH1F& h, double thresMsecs = -10000);
	double get_nfake(TH1F& h, double thresMsecs);
	double get_competition(TH1F& h, double thresMsecs, double q2dfilled);
};

#endif
