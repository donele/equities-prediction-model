#ifndef __gtlib_signal_QuoteSample__
#define __gtlib_signal_QuoteSample__
#include <optionlibs/TickData.h>
#include <jl_lib/Sessions.h>
#include <vector>

namespace gtlib {

class QuoteSample {
public:
	QuoteSample(){}
	QuoteSample(int openMsecs, int closeMsecs, int interval, bool reqVal, bool fillZeros, bool allowCross);
	QuoteSample(int openMsecs, int closeMsecs);
	QuoteSample(const std::vector<QuoteInfo>& quotes, const Sessions& session,
			int openMsecs, int closeMsecs, int interval, bool reqVal, bool fillZeros, bool allowCross);
	void addQuote(const QuoteInfo& quote);
	QuoteSample(const std::vector<QuoteDataMicro>& quotes, const Sessions& session,
			int openMsecs, int closeMsecs, int interval, bool reqVal, bool fillZeros, bool allowCross);
	void addQuote(const QuoteDataMicro& quote);
	void endTicker();
	float getMid(int msecs) const;
private:
	bool reqVal_;
	bool fillZeros_;
	bool allowCross_;
	int interval_;
	int openMsecs_;
	int closeMsecs_;
	int nSample_;
	int prevIndex_;
	std::vector<float> vMid_;
};

} // namespace gtlib

#endif
