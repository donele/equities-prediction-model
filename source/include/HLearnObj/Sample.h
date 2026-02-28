#ifndef __HNN_Sample__
#define __HNN_Sample__
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
	struct CLASS_DECLSPEC SampleSummary {
		SampleSummary(){}
		SampleSummary(std::vector<std::string>& inputNames, std::vector<std::string>& tickers);
		int nInput;
		std::vector<std::string> inputNames;
		int nTickers;
		std::vector<std::string> tickers;
	};

	struct CLASS_DECLSPEC SampleHeader {
		SampleHeader(){}
		SampleHeader(std::string ticker, int nEntries);
		std::string ticker;
		int nEntries;
	};

	struct CLASS_DECLSPEC Sample {
		Sample(){}
		Sample(const Sample& ns);
		Sample(std::vector<float>& vi, float& t);
		Sample(std::vector<float>& vi, float& t, QuoteInfo& qt);
		Sample(std::vector<float>& vi, float& t, std::vector<float>& vs);
		std::vector<float> input;
		float target;
		std::vector<float> spectator;
		QuoteInfo quote;
	};
}

FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const hnn::SampleSummary& obj);
FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const hnn::SampleHeader& obj);
FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const hnn::Sample& obj);
FUNC_DECLSPEC std::istream& operator >>(std::istream& is, hnn::SampleSummary& obj);
FUNC_DECLSPEC std::istream& operator >>(std::istream& is, hnn::SampleHeader& obj);
FUNC_DECLSPEC std::istream& operator >>(std::istream& is, hnn::Sample& obj);

#endif
