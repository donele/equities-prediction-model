#ifndef __HMarketMakingSpeed__
#define __HMarketMakingSpeed__
#include <HLib.h>
#include <jl_lib/MercatorOrder.h>
#include <HOrders.h>
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

class CLASS_DECLSPEC HMarketMakingSpeed: public HModule {
public:
	HMarketMakingSpeed(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HMarketMakingSpeed();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	bool write_hist_;
	bool insert_check_;
	int fakeMsecsMax_;
	int q2iMax_;
	int q2tMax_;
	int msecDirect_;
	int matchMin_;
	int matchMax_;
	int nday_;
	double histMax_;
	double fee_bpt_;
	double exchrat_;
	char execType_;
	int orderSchedType_;
	char side_;
	int verbose_;
	std::string dest_;
	std::string exchange_;
	std::string destCode_;

	boost::mutex hist_mutex_;
	boost::mutex fillrat_mutex_;
	boost::mutex nOrd_mutex_;
	boost::mutex vio_mutex_;

	struct FilledCancel {
		FilledCancel():price(0.), qty(0){}
		void add(float p, int q);
		//int cancelMsecs;
		//std::string ticker;
		float price;
		int qty;
	};

	struct Performance {
		Performance();
		void reset();
		Performance& operator+=(const Performance& p);
		void print_header();
		void print(std::string title, double exchrat, int nday);

		double n;
		double dv_filled;
		double dv_notfilled;
		double dv_matfilled;
		double dv_matnotfilled;
		double dv_dirfilled;
		double dv_missed;
		double dv_insfilled;
		double dv_misinsfilled;
		double profit_filled;
		double profit_notfilled;
		double profit_matfilled;
		double profit_matnotfilled;
		double profit_dirfilled;
		double profit_missed;
		double profit_insfilled;
		double profit_misinsfilled;
		double profit1;
		double profit10;
		double profitR;
	};

	//struct Count {
	//	void reset();
	//	void add(const Count& rhs);

	//	// orders by day.
	//	double nOrd_total;
	//	double nOrd_matched;
	//	double nOrd_unmatched;
	//	double nOrd_directFill;
	//	double nOrd_insertNFill;
	//	double nOrd_insertNCancel;
	//	double nOrd_unidFill;
	//	double nOrd_unidCancel;

	//	// shares.
	//	double nShr_total;
	//	double nShr_filled;

	//	// Fill ratio.
	//	double nFilledFull;
	//	double nFilledIncl;
	//	double nTotal;
	//	double dvFilled;
	//	double dvTotal;
	//	double nFilledOnFake;
	//	double nTotalOnFake;
	//	double nFilledOnNonFake;
	//	double nTotalOnNonFake;
	//};

	//struct HistSet {
	//	HistSet(){}
	//	void init(double histMax, std::string market, int fakeMsecsMax);
	//	void add_count(const Count& rhs);
	//	void write_hist(const std::string& moduleName, const std::string dest, char execType, int orderSchedType);

	//	void print_summary();
	//	void get_summary(Count& count, double& o2efilled, double& q2dfilled, double& q2tfilled, double& q2dnotfilled, double& q2tnotfilled, double& q2i,
	//				double& nfakeNotFilled, double& nfakeFilled,
	//				double& ncompetition, double& fillratshr, double& fillratincl, double& fillratinclmatched, double& fillratinclunmatched, double& fillratnonfake,
	//				double& ratdirectfill, double& ratmatched, double& ratfake, double& ratcomp);
	//	double get_top5bin_avg(TH1F& h, double thresMsecs = -10000);
	//	double get_nfake(TH1F& h);
	//	double get_competition(TH1F& h, double q2dfilled);

	//	int fakeMsecsMax;
	//	std::string market;
	//	Count count;
	//	Corr corr;
	//	Corr corr_filled;
	//	Corr corr_notFilled;

	//	// Match
	//	TH1F h_quoteMatch;

	//	// as function of time of day.
	//	TProfile p_q2o;
	//	TProfile p_fillrat;
	//	TH1F h_norders;

	//	// times by day.
	//	TH1F h_q2i;

	//	TH1F h_o2e_filled;
	//	TH1F h_o2v_filled;
	//	TH1F h_q2m_filled;
	//	TH1F h_q2d_filled;
	//	TH1F h_q2t_filled;
	//	TH1F h_q2m_direct_filled;
	//	TH1F h_q2d_direct_filled;
	//	TH1F h_q2o_filled;
	//	TH1F h_q2i_filled;
	//	TH1F h_i2x_filled;

	//	TH1F h_o2e_notFilled;
	//	TH1F h_q2d_notFilled;
	//	TH1F h_q2t_notFilled;
	//	TH1F h_q2o_notFilled;

	//	TProfile p_pred;
	//	TProfile p_pred_filled;
	//	TProfile p_pred_notFilled;
	//	TProfile p_pred1;
	//	TProfile p_pred1_filled;
	//	TProfile p_pred1_notFilled;
	//	TProfile p_pred10;
	//	TProfile p_pred10_filled;
	//	TProfile p_pred10_notFilled;
	//	TProfile p_predT;
	//	TProfile p_predT_filled;
	//	TProfile p_predT_notFilled;
	//	TProfile p_predR;
	//	TProfile p_predR_filled;
	//	TProfile p_predR_notFilled;

	//	TProfile p_fillrat_f2o;
	//	TProfile p_gpt_q2t_filled;
	//	TProfile p_gpt_q2t_notFilled;
	//};

	//Count countDay_;
	//Performance perfDay_;

	//Performance perfMarket_;
	//Performance perfTotal_;

	//HistSet histMarket_;
	//HistSet histTotal_;

	Performance perfDay_L_F_;
	Performance perfDay_L_FC_;
	Performance perfDay_A_F_;
	Performance perfDay_A_FC_;
	Performance perfMarket_L_F_;
	Performance perfMarket_L_FC_;
	Performance perfMarket_A_F_;
	Performance perfMarket_A_FC_;

	std::map<std::string, std::map<int, FilledCancel> > mFC_;

	void select_orders( std::vector<MercatorOrder>& orders, const std::vector<MercatorOrder>* vMOrder );
	//void fill_hist( HistSet& histset, std::vector<MercatorOrder>& orders, std::string  ticker );
	//void update_fillrat( std::vector<MercatorOrder>& orders );
	//void update_nOrders( std::vector<MercatorOrder>& orders, std::string ticker );
	void read_windstar(std::string market, int idate);
	void update_perf(std::vector<MercatorOrder>& orders, std::map<int, FilledCancel>& cancels);
	bool find_filled_cancel(MercatorOrder& order, FilledCancel& fc, std::map<int, FilledCancel>& cancels);
	std::string remove_quotes(std::string word);
};

#endif
