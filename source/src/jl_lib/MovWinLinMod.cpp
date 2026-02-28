#include <jl_lib/MovWinLinMod.h>
#include <jl_lib/jlutil.h>
#include <vector>
#include <stdio.h>
using namespace std;

MovWinLinMod::MovWinLinMod()
:nInputs_(0)
{
}

MovWinLinMod::MovWinLinMod(const string& path)
{
	ifstream ifs(path.c_str());
	if( ifs.is_open() )
	{
		ols_ = OLS();
		ifs >> ols_;
		nInputs_ = ols_.length();
	}
}

MovWinLinMod::MovWinLinMod(int nInputs, double ridgeReg, int iDecomp)
:nInputs_(nInputs),
ridgeReg_(ridgeReg),
iDecomp_(iDecomp),
ols_(OLS(nInputs, ridgeReg, iDecomp))
{
	cInfo_ = CorrInfo(nInputs);
}

int MovWinLinMod::getNInputs()
{
	return nInputs_;
}

int MovWinLinMod::nPts()
{
	return ols_.nPts();
}

void MovWinLinMod::getCoeffs(bool ridge, bool use_cov, int verbose)
{
	ols_.getCoeffs(ridge, use_cov, verbose);
}

void MovWinLinMod::endDay(int idate, int idateFirst, const string& path, bool doWrite)
{
	cInfoArch_[idate] = cInfo_;
	cInfo_.clear();
	if( doWrite )
	{
		ols_ = OLS(nInputs_, ridgeReg_, iDecomp_);

		// Remove old data. Add up the daily data.
		for( map<int, CorrInfo>::iterator it = cInfoArch_.begin(); it != cInfoArch_.end(); )
		{
			if( it->first < idateFirst )
			{
				cInfoArch_.erase(it++);
			}
			else
			{
				ols_.addCorr(it->second.vCorr, it->second.vvCorr, it->second.vAcc, it->second.accY);
				++it;
			}
		}

		// Calculate.
		ols_.getCoeffs(true, false);
		ofstream ofs(path.c_str());
		ofs << ols_;
	}
}

void MovWinLinMod::add(const vector<float>& input, float target)
{
	cInfo_.add(input, target);
}

const OLS& MovWinLinMod::getOLS()
{
	return ols_;
}

float MovWinLinMod::pred(const vector<float>& input)
{
	return ols_.pred(input);
}
