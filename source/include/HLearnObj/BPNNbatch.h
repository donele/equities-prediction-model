#ifndef __HNN_BPNNbatch__
#define __HNN_BPNNbatch__
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
	class CLASS_DECLSPEC BPNNbatch : public NNBase {
	public:
		BPNNbatch();
		BPNNbatch(const BPNNbatch & rhs);
		BPNNbatch(std::vector<int>& N, double learn, double wgtDecay, double maxWgt, int verbose, double biasOut);
		virtual ~BPNNbatch();

		virtual BPNNbatch* clone() const { return new BPNNbatch(*this); }
		virtual std::vector<double> train(const std::vector<float>& input, const QuoteInfo& quote, const double target);
		virtual std::vector<double> trading_signal(const std::vector<float>& input, const QuoteInfo& quote, const double lastPos);
		virtual std::ostream& write(std::ostream& str) const;
		virtual std::istream& read(std::istream& str);

	private:
		std::vector<double> forward(const std::vector<float>& input, const QuoteInfo& quote);
		void backprop(double error);
	};
}

#endif
