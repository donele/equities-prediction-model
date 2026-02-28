#include <HFitting/HMakePredIndex.h>
#include <HFitting.h>
#include <HLib.h>
#include <map>
#include <string>
#include "TFile.h"
using namespace std;

HMakePredIndex::HMakePredIndex(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
debug_(false),
verbose_(0),
fitAlg_("AR")
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("fitAlg") )
		fitAlg_ = conf.find("fitAlg")->second;
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
}

HMakePredIndex::~HMakePredIndex()
{}

void HMakePredIndex::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	TFile* f = HEnv::Instance()->outfile();
	f->cd();
	f->mkdir(moduleName_.c_str());

	char name[200];
	char title[400];
	const vector<hff::IndexFilter>& filters = HEnv::Instance()->indexFilters().filters;
	for( vector<hff::IndexFilter>::const_iterator it = filters.begin(); it != filters.end(); ++it )
	{
		const hff::IndexFilter& filter = *it;
		sprintf(name, "qp_%s", filter.title().c_str());
		sprintf(title, "Return vs. Pred, %s", filter.title().c_str());
		qp_.push_back(QProfile(name, title));
	}
}

void HMakePredIndex::beginDay()
{
	ar_.clear();
	int idate = HEnv::Instance()->idate();
	const vector<hff::IndexFilter>& filters = HEnv::Instance()->indexFilters().filters;
	for( vector<hff::IndexFilter>::const_iterator it = filters.begin(); it != filters.end(); ++it )
	{
		const hff::IndexFilter& filter = *it;
		string model_path = flt::get_model_path(filter, fitAlg_, idate);
		if( !model_path.empty() )
			ar_.push_back(ARMulti(model_path));
	}

	if( ar_.size() == filters.size() )
	{
		const hff::LinearModel& linMod = HEnv::Instance()->linearModel();

		// There may be multiple filters to calculate.
		int iFilter = 0;
		for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end(); ++itf, ++iFilter )
		{
			const hff::IndexFilter& filter = *itf;
			vector<const vector<ReturnData>*> vp = flt::get_data(filter.names, idate);
			vector<float> vPred(linMod.gpts);
			if( flt::valid_data(vp) )
			{
				int NF = filter.names.size();
				int iSerTarget = 0;
				int totlen = vp[0]->size();

				for( int i = 1; i < linMod.gpts; ++i )
				{
					int t = i * linMod.gridInterval;

					// Predictors.
					vector<vector<float> > vv(NF - 1, vector<float>(filter.length));
					for( int iSer = 1; iSer < NF; ++iSer )
					{
						int jBegin = t - filter.length;
						if( jBegin < 0 )
							jBegin = 0;
						int jEnd = t; // exclusive.
						for( int j = jBegin; j < jEnd; ++j )
						{
							double val2 = clip_off( (*vp[iSer])[j].ret, filter.clip[iSer] );
							vv[iSer - 1][j - jBegin] = val2;
						}
					}
					float pred = ar_[iFilter].pred(vv);
					if( pred < - linMod.clip_pred_index_ )
						pred = - linMod.clip_pred_index_;
					else if( pred > linMod.clip_pred_index_ )
						pred = linMod.clip_pred_index_;
					vPred[i] = pred;

					// debug.
					double target = 0.;
					int jBegin = t + filter.targetLag;
					int jEnd = t + filter.horizon; // exclusive.
					if( jEnd > totlen )
						jEnd = totlen;
					for( int j = jBegin; j < jEnd; ++j )
						target += (*vp[iSerTarget])[j].ret;
					qp_[iFilter].Fill(target, pred);
					if( debug_ )
						printf("%8d %7.5f %7.5f\n", t, pred, target);
				}

				// Debug. Calculate every one second.
				if( debug_ )
				{
					int lagMax = filter.length + filter.horizon;
					for( int i = filter.skip + filter.length; i < totlen - filter.horizon + 1; ++i ) // index of the target series.
					{
						// Target.
						double target = 0.;
						for( int t = i + filter.targetLag; t < i + filter.horizon; ++t )
							target += (*vp[iSerTarget])[t].ret;

						// Predictors.
						vector<vector<float> > vv(NF - 1, vector<float>(filter.length));

						for( int iSer = 1; iSer < NF; ++iSer ) // predictor series.
						{
							int jBegin = i - filter.length;
							int jEnd = i; // exclusive.

							for( int j = jBegin; j < jEnd; ++j ) // index of the predictor series.
							{
								double val2 = clip_off( (*vp[iSer])[j].ret, filter.clip[iSer] );
								vv[iSer - 1][j - jBegin] = val2;
							}
						}
						float pred = ar_[iFilter].pred(vv);
						//vPred[i - 1] = pred;

						// debug.
						//qp_[iFilter].Fill(target, pred);
						if( debug_ )
							printf("%8d %7.5f %7.5f\n", i, pred, target);
					}
				}
			}
			string predName = filter.title() + "_pred";
			HEvent::Instance()->add<vector<float> >("", predName, vPred);
		}
	}
	if( verbose_ >= 1 )
	{
		cout << "\HMakePredIndex::beginDay() " << idate << ".\n";
		if( verbose_ > 1 )
			PrintMemoryInfoSimple();
	}
}

void HMakePredIndex::endDay()
{
}

void HMakePredIndex::endJob()
{
	TFile* f = HEnv::Instance()->outfile();
	f->cd();
	f->cd(moduleName_.c_str());
	for( vector<QProfile>::iterator it = qp_.begin(); it != qp_.end(); ++it )
		it->getObject(10).Write();
}
