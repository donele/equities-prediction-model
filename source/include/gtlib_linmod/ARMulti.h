#ifndef __ARMulti__
#define __ARMulti__
#include <gtlib_model/hff.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <jl_lib.h>

namespace gtlib {

class ARMulti {
public:
	ARMulti(){}
	ARMulti(int nSeries, int filterLen, int horizon = 1, int targetLag = 0);
	ARMulti(const std::string& path);

	bool coeffsGood();
	void add(int iSer1, int iSer2, int lag, double v1, double v2);
	void add(const hff::IndexFilter& filter, const std::vector<const std::vector<ReturnData>*>& vp);

	std::vector<std::vector<float> > getCoeffs(const std::string& target = "sum", int verbose = 0);
	std::vector<std::vector<float> >& coeffs();
	float pred(const std::vector<std::vector<float> >& v);

private:
	int nSeries_;
	int nPredictors_;
	int filterLen_;
	int horizon_;
	int targetLag_;
	std::vector<std::vector<std::vector<Corr> > > corrs_; // nSeries x lagMax.
	std::vector<std::vector<float> > coeffs_;
	friend FUNC_DECLSPEC std::ostream& operator <<(std::ostream& os, const ARMulti& obj);
	friend FUNC_DECLSPEC std::istream& operator >>(std::istream& is, ARMulti& obj);
};

} // namespace gtlib

#endif
