#ifndef __MOrderAna__
#define __MOrderAna__
#include <MFramework/MModuleBase.h>
#include <jl_lib.h>
#include <boost/thread.hpp>
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

class CLASS_DECLSPEC MOrderAna: public MModuleBase {
public:
	MOrderAna(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MOrderAna();

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
	bool write_hist_total_;
	bool insert_check_;
	bool exper_univ_;
	bool profitable_orders_only_;
	bool pos_sprd_;
	bool composite_state_only_;
	int fakeMsecsMax_;
	int q2iMax_;
	int q2tMax_;
	int msecDirect_;
	int matchMin_;
	int matchMax_;
	double priceMin_;
	double priceMax_;
	double histMax_;
	double exchrat_;
	char execType_;
	int orderSchedType_;
	char side_;
	int verbose_;
	std::string dest_;
	std::string exchange_;
	std::string destCode_;
	std::set<std::string> sTickers_;

	boost::mutex hist_mutex_;
	boost::mutex fillrat_mutex_;
	boost::mutex nOrd_mutex_;
	boost::mutex vio_mutex_;

	struct Performance {
		Performance();
		void reset();
		Performance& operator+=(const Performance& p);
		void print();

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
		double profitON;
		double nprofit1;
		double nprofit10;
		double nprofitR;
		double nprofitON;
	};

	struct PerformanceSet {
		void reset();
		void add(int idate, const Performance& p);
		void print();
		void print_day(int idate, Performance p);
		std::map<int, Performance> mperf;
	};

	struct Count {
		void reset();
		void add(const Count& rhs);

		// orders by day.
		double nOrd_total;
		double nOrd_matched;
		double nOrd_unmatched;
		double nOrd_directFill;
		double nOrd_insertNFill;
		double nOrd_insertNCancel;
		double nOrd_unidFill;
		double nOrd_unidCancel;

		// shares.
		double nShr_total;
		double nShr_filled;

		// Fill ratio.
		double nFilledFull;
		double nFilledIncl;
		double nTotal;
		double dvFilled;
		double dvTotal;
		double nFilledOnFake;
		double nTotalOnFake;
		double nFilledOnNonFake;
		double nTotalOnNonFake;
	};

	struct HistSet {
		HistSet(){}
		void init(double histMax, std::string market, int fakeMsecsMax);
		void add_count(const Count& rhs);
		void write_hist(const std::string& moduleName, const std::string& dest, char execType, int orderSchedType);

		std::string market;
		Count count;
		Corr corr;
		Corr corr_filled;
		Corr corr_notFilled;

		std::vector<float> v_matched;
		std::vector<float> v_msecs;
		std::vector<float> v_fill;
		std::vector<float> v_insert;
		std::vector<float> v_directfill;

		std::vector<float> v_q2i;
		std::vector<float> v_q2m;
		std::vector<float> v_q2d;
		std::vector<float> v_q2t;
		std::vector<float> v_q2o;

		std::vector<float> v_i2x;
		std::vector<float> v_o2e;

		std::vector<float> v_pnl;
		std::vector<float> v_gpt;
		std::vector<float> v_pred;
		std::vector<float> v_pred1;
		std::vector<float> v_pred10;
		std::vector<float> v_ret;
		std::vector<float> v_ret1;
		std::vector<float> v_ret10;
		std::vector<float> v_retT;
		std::vector<float> v_retR;
	};

	Count countDay_;
	Performance perfDay_;

	PerformanceSet perfSetMarket_;
	PerformanceSet perfSetTotal_;
	Performance perfMarket_;
	Performance perfTotal_;

	HistSet histMarket_;
	HistSet histTotal_;

	void select_orders( std::vector<MercatorOrder>& orders, const std::vector<MercatorOrder>* vMOrder, const std::string& ticker );
	void fill_hist( HistSet& histset, std::vector<MercatorOrder>& orders, std::string  ticker );
	void update_fillrat( std::vector<MercatorOrder>& orders );
	void update_nOrders( std::vector<MercatorOrder>& orders, std::string ticker );
};

#endif
