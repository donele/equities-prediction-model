#ifndef __HNN_Pred2Sig_2in__
#define __HNN_Pred2Sig_2in__
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
	class CLASS_DECLSPEC Pred2Sig_2in : public NNBase {
	public:
		Pred2Sig_2in();
		Pred2Sig_2in(const Pred2Sig_2in & rhs);
		Pred2Sig_2in(std::vector<int>& N, double learn, double wgtDecay, double maxWgt, int verbose, double biasOut);
		virtual ~Pred2Sig_2in();

		virtual Pred2Sig_2in* clone() const { return new Pred2Sig_2in(*this); }
		virtual std::vector<double> train(const std::vector<float>& input, const QuoteInfo& quote, const double target);
		virtual std::vector<double> trading_signal(const std::vector<float>& input, const QuoteInfo& quote, const double lastPos);
		virtual std::ostream& write(std::ostream& str) const;
		virtual std::istream& read(std::istream& str);

	private:
		int iPred41m_;

		void get_iPred();
		std::vector<double> forward(const std::vector<float>& input, const QuoteInfo& quote);
		void backprop(double error);
	};
}

#endif
