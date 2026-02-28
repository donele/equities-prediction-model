#ifndef __HFillTimeHist__
#define __HFillTimeHist__
#include <HLib/HModule.h>
#include <jl_lib/MercatorOrder.h>
#include <HOrders.h>
#include <boost/thread.hpp>
#include <map>
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

class CLASS_DECLSPEC HFillTimeHist: public HModule {
public:
	HFillTimeHist(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HFillTimeHist();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	int nDay_;
	int fakeMsecsMax_;
	std::string procName_;

	boost::mutex hist_mutex_;
	boost::mutex map_mutex_;
	boost::mutex fillrat_mutex_;
	boost::mutex nOrd_mutex_;

	// Match
	TH1F h_quoteMatch_;
	TH1F h_quoteMatch_day_;

	// distributions.
	TH1F h_o2e_filled_;
	TH1F h_o2v_filled_;
	TH1F h_q2d_filled_;
	TH1F h_q2o_filled_;
	TH1F h_q2i_filled_;
	TH1F h_i2x_filled_;

	TH1F h_o2e_notFilled_;
	TH1F h_o2v_notFilled_;
	TH1F h_q2d_notFilled_;
	TH1F h_q2o_notFilled_;
	TH1F h_q2i_notFilled_;
	TH1F h_i2x_notFilled_;

	// q2o.
	TProfile p_q2o_o_;
	TProfile p_q2o_alph_;

	// times by day.
	TH1F h_o2e_filled_day_;
	TH1F h_o2v_filled_day_;
	TH1F h_q2d_filled_day_;
	TH1F h_q2o_filled_day_;
	TH1F h_q2i_filled_day_;
	TH1F h_i2x_filled_day_;
	TH1F h_fakeRat_filled_day_;

	TH1F h_o2e_notFilled_day_;
	TH1F h_o2v_notFilled_day_;
	TH1F h_q2d_notFilled_day_;
	TH1F h_q2o_notFilled_day_;
	TH1F h_q2i_notFilled_day_;
	TH1F h_i2x_notFilled_day_;
	TH1F h_fakeRat_notFilled_day_;

	TH1F h_o2e_filled_today_;
	TH1F h_o2v_filled_today_;
	TH1F h_q2d_filled_today_;
	TH1F h_q2o_filled_today_;
	TH1F h_q2i_filled_today_;
	TH1F h_i2x_filled_today_;

	TH1F h_o2e_notFilled_today_;
	TH1F h_o2v_notFilled_today_;
	TH1F h_q2d_notFilled_today_;
	TH1F h_q2o_notFilled_today_;
	TH1F h_q2i_notFilled_today_;
	TH1F h_i2x_notFilled_today_;

	// orders by day.
	double nOrd_total_;
	double nOrd_matched_;
	double nOrd_unmatched_;
	double nOrd_directFill_;
	double nOrd_insertNFill_;
	double nOrd_insertNCancel_;
	double nOrd_unidFill_;
	double nOrd_unidCancel_;

	TH1F h_nOrd_total_;
	TH1F h_nOrd_matched_;
	TH1F h_nOrd_unmatched_;
	TH1F h_nOrd_directFill_;
	TH1F h_nOrd_insertNFill_;
	TH1F h_nOrd_insertNCancel_;
	TH1F h_nOrd_unidFill_;
	TH1F h_nOrd_unidCancel_;

	TH1F h_ratOrd_matched_;
	TH1F h_ratOrd_unmatched_;
	TH1F h_ratOrd_directFill_;
	TH1F h_ratOrd_insertNFill_;
	TH1F h_ratOrd_insertNCancel_;
	TH1F h_ratOrd_unidFill_;
	TH1F h_ratOrd_unidCancel_;
	TH1F h_ratOrd_fillWhenInserted_;

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

	TH1F h_rat_full_;
	TH1F h_rat_incl_;
	TH1F h_rat_dv_;
	TH1F h_rat_fillAgainstFakeOrder_;
	TH1F h_rat_fillAgainstNonFakeOrder_;

	// Fill ratio by match.
	std::map<int, double> mMatchNOrderFilledFull_;
	std::map<int, double> mMatchNOrderFilledIncl_;
	std::map<int, double> mMatchNOrderTotal_;
	std::map<int, double> mMatchDollarvolExec_;
	std::map<int, double> mMatchDollarvol_;

	TH2F h_fillrat_full_;
	TH2F h_fillrat_incl_;
	TH2F h_fillrat_dv_;

	void set_axis_title(TH1& h);
	void fill_hist( const std::vector<MercatorOrder>* vp, std::string  ticker );
	void update_fillrat_bymatch( const std::vector<MercatorOrder>* vp );
	void update_fillrat( const std::vector<MercatorOrder>* vp );
	void update_nOrders( const std::vector<MercatorOrder>* vp );
	double get_top5bin_avg(TH1F& h, double thresMsecs = -10000);
	double get_fakeRat(TH1F& h);
};

#endif
