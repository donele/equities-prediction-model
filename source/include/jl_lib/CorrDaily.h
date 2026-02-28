#ifndef __CorrDaily__
#define __CorrDaily__
#include <jl_lib/jlutil.h>
#include <deque>

class CorrDaily {
	public:
		CorrDaily();
		CorrDaily(int maxSize_);
		void endDay();
		void add(double x, double y);
		double corr() const;
		double stdevX() const;
		double stdevY() const;
	private:
		bool readable_;
		int maxSize_;
		int minNData_;
		Corr corrBuf_;
		Corr corrSum_;
		std::deque<Corr> dqCorr_;
};

#endif
