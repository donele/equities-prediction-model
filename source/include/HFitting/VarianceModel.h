#ifndef __VarianceModel__
#define __VarianceModel__
#include <HLib.h>
#include <vector>
#include <map>

class VarianceModel {
public:
	VarianceModel();
	VarianceModel(int nHorizon, int om_num_sig, int om_fit_days, int gridInterval, int pid, int verbose = 0);
	void endJob();

	void beginDay(int idate, int idateFirst);
	void update(int iT, std::vector<float>& v, double target);
	double pred(int iT, std::vector<float>& v);

private:
	int pid_;
	int verbose_;
	int iDecomp_;
	std::string dir_;
	std::set<int> sIdate_;
	std::vector<OLSmovMM> olsmov_;
};

#endif
