#include <MSignal/MMakePredIndexOLSTest.h>
#include <MSignal/flt.h>
#include <jl_lib/MovWinLinMod.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
using namespace std;

MMakePredIndexOLSTest::MMakePredIndexOLSTest(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
verbose_(0),
lag0_(1),
lagMult_(1)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("fmodel") )
		fmodel_ = conf.find("fmodel")->second;
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("lag0") )
		lag0_ = atoi( conf.find("lag0")->second.c_str() );
	if( conf.count("lagMult") )
		lagMult_ = atoi( conf.find("lagMult")->second.c_str() );

	if( fmodel_.empty() )
		fmodel_ = MEnv::Instance()->model;
}

MMakePredIndexOLSTest::~MMakePredIndexOLSTest()
{}

void MMakePredIndexOLSTest::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	iim_ = IndexInputMaker(lag0_, lagMult_);
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	vCorrProd_ = vector<Corr>(filters.size());
	vCorrProdTarget_ = vector<Corr>(filters.size());
	vCorrTestTarget_ = vector<Corr>(filters.size());
	vAccErrProd_ = vector<Acc>(filters.size());
	vAccErrTest_ = vector<Acc>(filters.size());
}

void MMakePredIndexOLSTest::beginDay()
{
	read_filter();
	make_prediction();
}

void MMakePredIndexOLSTest::read_filter()
{
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	vLinMod_.clear();
	int idate = MEnv::Instance()->idate;
	for( vector<hff::IndexFilter>::const_iterator it = filters.begin(); it != filters.end(); ++it )
	{
		const hff::IndexFilter& filter = *it;
		string filter_path = get_weight_path(filter, fmodel_, idate, lag0_, lagMult_);
		if( exists(boost::filesystem::path(filter_path)) )
			vLinMod_.push_back(MovWinLinMod(filter_path));
	}
}

void MMakePredIndexOLSTest::make_prediction()
{
	int idate = MEnv::Instance()->idate;
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	if( vLinMod_.size() == filters.size() )
	{
		const hff::LinearModel& linMod = MEnv::Instance()->linearModel;

		// There may be multiple filters to calculate.
		int iFilter = 0;
		for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end(); ++itf, ++iFilter )
		{
			const hff::IndexFilter& filter = *itf;
			vector<const vector<ReturnData>*> vp = flt::get_data(filter.names, idate);
			vector<float> vPred(linMod.n1sec);
			vector<float> vTarget(linMod.n1sec);
			if( flt::valid_data(vp) )
			{
				int NF = filter.names.size();
				for( int t = 1; t < linMod.n1sec; ++t )
				{
					float target = iim_.createTarget(t, vp[0], filter.horizon);
					vector<float> input(vLinMod_[iFilter].getNInputs(), 0.);
					iim_.createInput(input, t, vp[1], filter.length);
					float pred = vLinMod_[iFilter].pred(input);
					clip(pred, linMod.clip_pred_index);
					vPred[t] = pred;
					vTarget[t] = target;

					if( debug_ && t % 5 == 0 )
						cout << "OLS" << "\t" << t << "\t" << target << "\t" << pred << endl;
				}
			}

			string predName = filter.title() + "_pred";
			const vector<float>* pvPred = static_cast<const vector<float>*>(MEvent::Instance()->get("", predName));
			if( pvPred != 0 )
			{
				for( int t = 1; t < linMod.n1sec; ++t )
				{
					vCorrProd_[iFilter].add(vPred[t], (*pvPred)[t]);
					vCorrProdTarget_[iFilter].add(vTarget[t], (*pvPred)[t]);
					vCorrTestTarget_[iFilter].add(vTarget[t], vPred[t]);
					vAccErrProd_[iFilter].add(vTarget[t] - (*pvPred)[t]);
					vAccErrTest_[iFilter].add(vTarget[t] - vPred[t]);
				}
			}
		}
	}
}

void MMakePredIndexOLSTest::endDay()
{
	int idate = MEnv::Instance()->idate;
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	int iFilter = 0;
	printf("\n%s %d %d\n", moduleName_.c_str(), lag0_, lagMult_);
	for( vector<hff::IndexFilter>::const_iterator it = filters.begin(); it != filters.end(); ++it, ++iFilter )
	{
		printf("filter# %d test-prod %.4f prod-target %.4f test-target %.4f\n", iFilter, vCorrProd_[iFilter].corr(), vCorrProdTarget_[iFilter].corr(), vCorrTestTarget_[iFilter].corr());
	}
}

void MMakePredIndexOLSTest::endJob()
{
	printf("\n%s %d %d\n", moduleName_.c_str(), lag0_, lagMult_);
	printf("filter# test-prod prod-target test-target RMSErrProd RMSErrTest ofProd ofTest\n");
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	int iFilter = 0;
	for( vector<hff::IndexFilter>::const_iterator it = filters.begin(); it != filters.end(); ++it, ++iFilter )
	{
		double stdevTarget = vCorrProdTarget_[iFilter].accX.stdev();
		double stdevProd = vCorrProdTarget_[iFilter].accY.stdev();
		double stdevTest = vCorrTestTarget_[iFilter].accY.stdev();
		double ofProd = stdevProd / stdevTarget - 1.;
		double ofTest = stdevTest / stdevTarget - 1.;
		printf("%7d %9.4f %11.4f %11.4f %10.6f %10.6f %6.3f %6.3f\n", iFilter, vCorrProd_[iFilter].corr(), vCorrProdTarget_[iFilter].corr(), vCorrTestTarget_[iFilter].corr(), vAccErrProd_[iFilter].RMS(), vAccErrTest_[iFilter].RMS(), ofProd, ofTest);
	}
}

string MMakePredIndexOLSTest::get_weight_path(const hff::IndexFilter& filter, string model, int idate, int lag0, int lagMult)
{
	string outdir = flt::get_filter_dir(filter, model);
	mkd(outdir);
	string filename;
	if( lag0 == 0 && lagMult == 0 )
		filename = "weight" + itos(idate) + ".txt";
	else
		filename = "weight" + itos(idate) + "_" + itos(lag0) + "_" + itos(lagMult) + ".txt";
	string path = xpf(outdir + "\\" + filename).c_str();
	return path;
}

