#ifndef __TickSeriesFtns__
#define __TickSeriesFtns__
#include <optionlibs/TickData.h>
#include <vector>
#include <jl_lib/Sessions.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

namespace TickSeriesFtns
{
	void FUNC_DECLSPEC get_quote_series( std::vector<double>& msecQ, std::vector<double>& midQ,
				std::vector<double>& bidQ, std::vector<double>& askQ,
				const TickSeries<QuoteInfo>* ptsQ, std::vector<std::pair<int, int> >& sessions);
	void FUNC_DECLSPEC get_quote_series( std::vector<double>& msecQ, std::vector<QuoteInfo>& Q,
				const TickSeries<QuoteInfo>* ptsQ, Sessions& sessions);
	void FUNC_DECLSPEC get_quote_1s_series( std::vector<double>& msecQ1s, std::vector<double>& midQ1s,
				std::vector<double>& bidQ1s, std::vector<double>& askQ1s,
				std::vector<double>& msecQ, std::vector<double>& midQ,
				std::vector<double>& bidQ, std::vector<double>& askQ);
	void FUNC_DECLSPEC replace_zeros(std::vector<double>& series);
}

#endif
