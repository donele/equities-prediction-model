#include <gtlib_fitting/CorrInput.h>
#include <gtlib_fitting/fittingFtns.h>
#include <MFramework.h>
#include <boost/filesystem.hpp>
#include <string>
using namespace std;

namespace gtlib {

CorrInput::CorrInput()
	:pFitData_(nullptr)
{}

CorrInput::CorrInput(FitData* pFitData, const string& fitDir, int udate)
	:pFitData_(pFitData),
	statDir_(getStatDir(fitDir, udate))
{
	mkd(statDir_);
}

void CorrInput::stat()
{
	vector<int> idates = pFitData_->getDailyDataCount().getIdates();
	int nDays = idates.size();
	int nInput = pFitData_->nInputFields;

	vector<vector<Corr>> vCorrDay(nDays, vector<Corr>(nInput));
	vector<vector<Corr>> vCorrTotDay(nDays, vector<Corr>(nInput));

	vector<Corr> vCorr(nInput);
	vector<Corr> vCorrTot(nInput);
	vector<vector<Corr>> vvCorr(nInput, vector<Corr>(nInput));

	// Loop dates.
	for( int iDay = 0; iDay < nDays; ++iDay )
	{
		// find offset.
		int testDate = idates[iDay];
		int offset1 = pFitData_->getDailyDataCount().getOffset(testDate);
		int offset2 = offset1 + pFitData_->getDailyDataCount().getNdata(testDate);

		for( int iS = offset1; iS < offset2; ++iS )
		{
			float target = pFitData_->target(iS);
			float totTarget = target + pFitData_->bmpred(iS);
			for( int i1 = 0; i1 < nInput; ++i1 )
			{
				float input1 = pFitData_->input(i1, iS);
				vCorrDay[iDay][i1].add(input1, target);
				vCorrTotDay[iDay][i1].add(input1, totTarget);

				for( int i2 = i1 + 1; i2 < nInput; ++i2 )
				{
					float input2 = pFitData_->input(i2, iS);
					vvCorr[i1][i2].add(input1, input2);
				}
			}
		}

		// Update total corrs.
		for( int i = 0; i < nInput; ++i )
		{
			vCorr[i] += vCorrDay[iDay][i];
			vCorrTot[i] += vCorrTotDay[iDay][i];
		}
	}

	// Corr between inputs and residual targets.
	string outfile = statDir_ + "/corr_input_" + itos(*idates.begin()) + "_" + itos(*idates.rbegin()) + ".txt";
	ofstream ofs(outfile.c_str());
	vector<string>& inputNames = pFitData_->inputNames;
	ofs << "\ncorr(input, restar)\n";
	for( auto it = begin(inputNames); it != end(inputNames); ++it )
		ofs << *it << " ";
	ofs << "\n";

	char buf[200];
	for( int iDay = 0; iDay < nDays; ++iDay )
	{
		ofs << idates[iDay] << " ";
		for( int i = 0; i < nInput; ++i )
		{
			sprintf(buf, "%8.3f ", vCorrDay[iDay][i].corr() * 100.);
			ofs << buf;
		}
		ofs << "\n";
	}
	sprintf(buf, "%8s ", "All");
	ofs << buf;
	for( int i = 0; i < nInput; ++i )
	{
		sprintf(buf, "%8.3f ", vCorr[i].corr() * 100.);
		ofs << buf;
	}
	ofs << "\n";

	// Corr between inputs and total targets.
	ofs << "\ncorr(input, tar)\n";
	for( auto it = begin(inputNames); it != end(inputNames); ++it )
		ofs << *it << " ";
	ofs << "\n";

	for( int iDay = 0; iDay < nDays; ++iDay )
	{
		ofs << idates[iDay] << " ";
		for( int i = 0; i < nInput; ++i )
		{
			sprintf(buf, "%8.3f ", vCorrTotDay[iDay][i].corr() * 100.);
			ofs << buf;
		}
		ofs << "\n";
	}
	sprintf(buf, "%8s ", "All");
	ofs << buf;
	for( int i = 0; i < nInput; ++i )
	{
		sprintf(buf, "%8.3f ", vCorrTot[i].corr() * 100.);
		ofs << buf;
	}
	ofs << "\n";

	// Corr between the inputs.
	ofs << "\ncorr(input, input)\n";
	for( int i1 = 0; i1 < nInput; ++i1 )
	{
		for( int i2 = 0; i2 < nInput; ++i2 )
		{
			double corr = 1.;
			if( i1 < i2 )
				corr = vvCorr[i1][i2].corr();
			else if( i1 > i2 )
				corr = vvCorr[i2][i1].corr();
			sprintf(buf, "%16s %16s %8.3f\n", inputNames[i1].c_str(), inputNames[i2].c_str(), corr * 100.);
			ofs << buf;
		}
	}
}

} // namespace gtlib
