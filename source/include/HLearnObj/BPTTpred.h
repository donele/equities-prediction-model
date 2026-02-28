#ifndef __HNN_BPTTpred__
#define __HNN_BPTTpred__
#include "NNBase.h"
#include "optionlibs/TickData.h"
#include <vector>
#include <iostream>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

namespace hnn {
	class CLASS_DECLSPEC BPTTpred : public NNBase {
		friend class BPTTpred;
	public:
		BPTTpred();
		BPTTpred(const BPTTpred & rhs);
		BPTTpred(std::vector<int>& N, double learn, double wgtDecay, double maxWgt, int verbose, double biasOut, double costMult,
			double sigP, double poslim, double maxPos, int lotSize, bool maxRet, bool noTwoTradesAtSamePrice);
		virtual ~BPTTpred();

		virtual BPTTpred* clone() const { return new BPTTpred(*this); }
		virtual void beginDay();
		virtual std::vector<double> train(const std::vector<float>& input, const QuoteInfo& quote, const double target);
		virtual std::vector<double> trading_signal(const std::vector<float>& input, const QuoteInfo& quote, const double lastPos);
		virtual std::ostream& write(std::ostream& str) const;
		virtual std::istream& read(std::istream& str);

		virtual double costMult();
		virtual void start_ticker(double position);

	private:
		double sigP_;
		double poslim_;
		double maxPos_;
		double costMult_;
		int lotSize_;
		bool maxRet_;
		bool noTwoTradesAtSamePrice_;

		double last_mid_;
		double last_trade_price_;
		bool trade_aborted_;
		QuoteInfo quote_;
		double F_;
		double F_p_;
		double P_;
		double P_p_;
		std::vector<std::vector<std::vector<double> > > pFpw_;
		std::vector<std::vector<std::vector<double> > > dFdw_;
		std::vector<std::vector<std::vector<double> > > p_dFdw_;

		int iPred1m_;
		int iPred40m_;

		void init_vectors();

		double get_R0();
		double get_R();
		double get_g();

		double get_pRpP();
		double get_pRpP_p();
		double get_pgpP();
		double get_pgpP_p();
		double get_pFpF_p();

		void get_iPred();
		std::vector<double> forward(const std::vector<float>& input, const QuoteInfo& quote);
		void backprop(double error);
	};
}

#endif