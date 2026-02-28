#ifndef __HReturnAna__
#define __HReturnAna__
#include <HLib/HModule.h>
#include <jl_lib/jlutil.h>
#include <boost/thread.hpp>
#include "TH1.h"
#include "TH2.h"
#include <map>
#include <string>
#include <vector>
#include <list>

class CLASS_DECLSPEC HReturnAna: public HModule {
public:
	HReturnAna(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HReturnAna();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	bool debug_;
	int verbose_;
	int nBinsPerDay_;
	int secsPerBin_;
	double fee_bpt_;
	std::string tradeModuleName_;
	boost::mutex lag_mutex_;

	std::string id_;
	std::string msecName_;
	std::string prcName_;
	std::string volName_;
	std::string signName_;
	std::string schedName_;
	std::string midName_;
	std::string posName_;
	std::string sampleOpenName_;

	int nLags_;
	std::vector<int> lags_;
	std::map<int, std::vector<double> > lag_sumSignedRtn_tt_;
	std::map<int, std::vector<double> > lag_sumSignedRtn_rt_;
	std::vector<std::vector< std::vector<double> > > sumSignedRtn2d_rt_;

	int nLagsNext_;
	std::vector<int> lagsNext_;
	std::map<int, std::vector<double> > lagNext_sumSignedRtn_tt_;
	std::map<int, std::vector<double> > lagNext_sumSignedRtn_rt_;
	std::vector<std::vector< std::vector<double> > > sumSignedRtn2dNext_rt_;

	void fill_return(std::map<int, std::vector<double> >& all_lag_sumSignedRtn, std::vector<int>& lags,
		const std::vector<double>* msecT, const std::vector<double>* prcT, const std::vector<double>* shrT,
		const std::vector<double>* signT, const std::vector<double>* msecQ, const std::vector<double>* midQ);
	void fill_return(std::map<int, std::vector<double> >& lag_sumSignedRtn, std::vector<int>& lags,
		double prc0, double shr, int sign, const std::vector<double>* midQ, int n);
	void fill_return_2d(std::vector<std::vector<std::vector<double> > >& sumSignedRtn2d, std::vector<int>& lags,
		const std::vector<double>* msecT, const std::vector<double>* prcT, const std::vector<double>* shrT,
		const std::vector<double>* signT, const std::vector<double>* msecQ, const std::vector<double>* midQ);
	void fill_return_2d(std::vector<std::vector<std::vector<double> > >& sumSignedRtn2d, std::vector<int>& lags,
		double prc0, double shr, int sign, const std::vector<double>* midQ, int n);
	void make_graph(std::vector<double>& x, std::vector<double>& y, std::vector<double>& ex, std::vector<double>& ey,
		std::string varName, std::string timeUnit, std::string market, std::string dir);

	void plot_return();
	void plot_return(std::string dir, std::map<int, std::vector<double> >& m, std::string flag);
	void plot_return_2d(std::string dir, std::vector<std::vector<std::vector<double> > >& vvv, std::vector<int>& lags);
};


#endif