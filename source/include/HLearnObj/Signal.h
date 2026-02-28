#ifndef __HNN_Signal__
#define __HNN_Signal__
#include "optionlibs/TickData.h"
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

namespace hnn {
	struct CLASS_DECLSPEC SignalSummary {
		SignalSummary(){}
		SignalSummary(std::vector<std::string>& signalNames, std::vector<std::string>& tickers);
		int nSignals;
		std::vector<std::string> signalNames;
		int nTickers;
		std::vector<std::string> tickers;
		void print();
	};

	struct CLASS_DECLSPEC SignalLabel {
		SignalLabel(){}
		SignalLabel(std::string ticker, int nEntries);
		std::string ticker;
		int nEntries;
		void print();
	};

	struct CLASS_DECLSPEC SignalContent {
		SignalContent(){}
		SignalContent(const SignalContent& ns);
		SignalContent(std::vector<float>& vi);
		std::vector<float> input;
		void print();
	};

	struct CLASS_DECLSPEC BinSigHeader {
		BinSigHeader(){}
		int nrows;
		int ncols;
		std::vector<std::string> labels;
		bool valid();
	};

	struct CLASS_DECLSPEC BinSig {
		BinSig(int ncols_);
		int ncols;
		std::vector<float> input;
	};
}

FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const hnn::SignalSummary& obj);
FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const hnn::SignalLabel& obj);
FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const hnn::SignalContent& obj);
FUNC_DECLSPEC std::istream& operator >>(std::istream& is, hnn::SignalSummary& obj);
FUNC_DECLSPEC std::istream& operator >>(std::istream& is, hnn::SignalLabel& obj);
FUNC_DECLSPEC std::istream& operator >>(std::istream& is, hnn::SignalContent& obj);
FUNC_DECLSPEC std::istream& operator >>(std::istream& is, hnn::BinSigHeader& obj);
FUNC_DECLSPEC std::istream& operator >>(std::istream& is, hnn::BinSig& obj);

#endif
