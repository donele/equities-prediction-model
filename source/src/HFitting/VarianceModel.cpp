#include <HFitting/VarianceModel.h>
#include <boost/filesystem.hpp>
using namespace std;

VarianceModel::VarianceModel(){}

VarianceModel::VarianceModel(int nHorizon, int om_num_sig, int fit_days, int gridInterval, int pid, int verbose)
:pid_(pid),
verbose_(verbose),
iDecomp_(2)
{
	// OLS with moving window, by time slices.
	olsmov_ = vector<OLSmovMM>(nHorizon, OLSmovMM(om_num_sig, /*fit_days, */gridInterval, iDecomp_));
	dir_ = (string)".\\variance\\" + itos(pid_) + "\\";
	ForceDirectory( dir_ );
}

void VarianceModel::endJob()
{
//	for( set<int>::iterator it = sIdate_.begin(); it != sIdate_.end(); ++it )
//	{
//		// remove the file.
//		string filename = dir_ + "\\" + itos(*it) + ".cov";
//		remove(filename.c_str());
//	}
//
//	remove_dir(dir_);
	boost::filesystem::remove_all(dir_);
}

void VarianceModel::beginDay(int idate, int idateFirst)
{
	if( !sIdate_.empty() )
	{
		// Write the corrs of the previous day.

		int idate_prev = *(sIdate_.rbegin());
		string filename = dir_ + "\\" + itos(idate_prev) + ".cov";

		ofstream ofs;
		ofs.open(filename.c_str());

		for( vector<OLSmovMM>::iterator it = olsmov_.begin(); it != olsmov_.end(); ++it )
			it->saveCorrs(ofs);
	}

	// Calculate the coefficients.
	//if( sIdate_.size() == hff::var_fit_days_ )
	if( !sIdate_.empty() )
	{
		// Initialize.
		for( vector<OLSmovMM>::iterator it = olsmov_.begin(); it != olsmov_.end(); ++it )
			it->initOLS();

		// Read the corrs.
		for( set<int>::iterator itd = sIdate_.begin(); itd != sIdate_.end(); ++itd )
		{
			int this_date = *itd;
			string filename = dir_ + "\\" + itos(this_date) + ".cov";
			ifstream ifs(filename.c_str());

			if( ifs.is_open() )
			{
				for( vector<OLSmovMM>::iterator it = olsmov_.begin(); it != olsmov_.end(); ++it )
					it->loadCorrs(ifs);
			}
		}

		// Calculate.
		for( vector<OLSmovMM>::iterator it = olsmov_.begin(); it != olsmov_.end(); ++it )
			it->getCoeffs(true, false, verbose_);
	}

	// Get ready for today.
	for( vector<OLSmovMM>::iterator it = olsmov_.begin(); it != olsmov_.end(); ++it )
		it->beginDay();
	sIdate_.insert(idate);

	// Remove the old files.
	for( set<int>::iterator it = sIdate_.begin(); it != sIdate_.end(); )
	{
		if( *it < idateFirst )
		{
			// remove the file.
			string filename = dir_ + "\\" + itos(*it) + ".cov";
			remove(filename.c_str());

			sIdate_.erase(it++);
		}
		else
			++it;
	}
}

void VarianceModel::update(int iT, vector<float>& v, double target)
{
	olsmov_[iT].add(v, target);
}

double VarianceModel::pred(int iT, vector<float>& v)
{
	double pr = olsmov_[iT].getOLS().pred(v);
	return pr;
}
