#ifndef __HNN_NNBase__
#define __HNN_NNBase__
#include "optionlibs/TickData.h"
#include <vector>
#include <iostream>
#include <boost/thread.hpp>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

namespace hnn {
	class CLASS_DECLSPEC NNBase {
		friend class NNBase;
	public:
		NNBase();
		NNBase(const NNBase & rhs);
		NNBase(std::vector<int>& N, double learn, double wgtDecay, double maxWgt, int verbose, double biasOut, bool recurrent);
		virtual ~NNBase();

		virtual NNBase* clone() const = 0;
		virtual void beginDay(){}
		virtual std::vector<double> train(const std::vector<float>& input, const QuoteInfo& quote, const double target = 0) = 0;
		virtual std::vector<double> trading_signal(const std::vector<float>& input, const QuoteInfo& quote, const double lastPos = 0) = 0;
		virtual std::ostream& write(std::ostream& str) const = 0;
		virtual std::istream& read(std::istream& str) = 0;

		void update_dw(const NNBase& rhs);
		void update_wgts();
		bool weights_valid();
		const std::vector<std::vector<std::vector<double> > >& get_dw() const;

		int get_nwgts() const;
		std::vector<double> get_wgts() const;
		std::vector<int> get_nnodes() const;
		void save();
		void restore();
		void scale_learn(double f);
		void set_learn(double learn);
		void set_wgtDecay(double wgtDecay);

		virtual double costMult(){ return 1.0; }
		virtual void start_ticker(double position){}
		virtual void set_position(double position){}

	protected:
		int nWgtWarnings_;
		double learn_;
		double wgtDecay_;
		double maxWgt_;
		int verbose_;
		bool useBias_; // Set to false during the training if biasOut is zero.
		double biasOut_;
		boost::mutex dw_mutex_;
		boost::mutex w_mutex_;

		int L_;
		std::vector<double> local_learn_;
		std::vector<int> N_;
		std::vector<std::vector<double> > ff_;
		std::vector<std::vector<double> > e_;
		std::vector<std::vector<std::vector<double> > > dw_;
		std::vector<std::vector<std::vector<double> > > w_;
		std::vector<std::vector<std::vector<double> > > wOld_;
		int nwgts_;

		void init_wgts();
		void reset_dw();
	};
}

FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, hnn::NNBase& obj);

#endif