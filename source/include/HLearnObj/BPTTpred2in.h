#ifndef __HNN_BPTTpred2in__
#define __HNN_BPTTpred2in__
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
	class CLASS_DECLSPEC BPTTpred2in : public NNBase {
		friend class BPTTpred2in;
	public:
		BPTTpred2in();
		BPTTpred2in(const BPTTpred2in & rhs);
		BPTTpred2in(std::vector<int>& N, double learn, double wgtDecay, double maxWgt, int verbose, double biasOut, double costMult,
			double sigP, double poslim, double maxPos, double maxPosChg, int lotSize, bool maxRet, bool noTwoTradesAtSamePrice);
		virtual ~BPTTpred2in();

		virtual BPTTpred2in* clone() const { return new BPTTpred2in(*this); }
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
		double maxPosChg_;
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

		int iPred41m_;

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