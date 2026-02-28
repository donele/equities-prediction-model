#include <MFitObj/CorrInput.h>
#include <boost/filesystem.hpp>
#include <string>
using namespace std;

CorrInput::CorrInput()
:debug_ticker_(false),
	pvvInputTarget_(0),
	pvvSpectator_(0),
	udate_(0)
{}

CorrInput::CorrInput(vector<vector<float> >* pvvInputTarget, vector<vector<float> >* pvvSpectator, bool debug_ticker)
:debug_ticker_(debug_ticker),
	pvvInputTarget_(0),
	pvvSpectator_(0),
	udate_(0)
{
	pvvInputTarget_ = pvvInputTarget;
	pvvSpectator_ = pvvSpectator;
}

void CorrInput::setDir(string fitDir, int udate)
{
	udate_ = udate;
	statDir_ = fitDir + xpf("\\stat_") + itos(udate);
	mkd(statDir_);
}

void CorrInput::stat(string fitDir, int udate, vector<string>& inputNames, vector<int>& testDates, map<int, pair<int, int> >& mOffset, string model, string sigtype, string fitdesc)
{
	setDir(fitDir, udate);

	char filename[1000];
	sprintf(filename, "%s\\corr_input_%d_%d.txt", statDir_.c_str(), testDates[0], testDates[testDates.size() - 1]);
	ofstream ofs(xpf(filename).c_str());
	char buf[2000];

	int nDays = testDates.size();
	int nInput = pvvInputTarget_->size() - 1;

	vector<vector<Corr> > vCorrDay(nDays, vector<Corr>(nInput));
	vector<vector<Corr> > vCorrTotDay(nDays, vector<Corr>(nInput));

	vector<Corr> vCorr(nInput);
	vector<Corr> vCorrTot(nInput);
	vector<vector<Corr> > vvCorr(nInput, vector<Corr>(nInput));

	// Loop dates.
	for( int iDay = 0; iDay < nDays; ++iDay )
	{
		// find offset.
		int testDate = testDates[iDay];
		int offset1 = mOffset[testDate].first;
		int offset2 = mOffset[testDate].second;

		// debug ticker.
		//if( debug_ticker_ )
		//init_ticker_offset(testDate, model, sigtype, fitdesc);

		for( int iS = offset1; iS < offset2; ++iS )
		{
			float target = (*pvvInputTarget_)[nInput][iS];
			float totTarget = target + (*pvvSpectator_)[1][iS];
			for( int i1 = 0; i1 < nInput; ++i1 )
			{
				vCorrDay[iDay][i1].add((*pvvInputTarget_)[i1][iS], target);
				vCorrTotDay[iDay][i1].add((*pvvInputTarget_)[i1][iS], totTarget);

				float input1 = (*pvvInputTarget_)[i1][iS];
				for( int i2 = i1 + 1; i2 < nInput; ++i2 )
					vvCorr[i1][i2].add(input1, (*pvvInputTarget_)[i2][iS]);
			}

			if( debug_ticker_ )
				process_ticker(iS - offset1, (*pvvInputTarget_)[5][iS], totTarget);
		}
		if( debug_ticker_ )
			debug_ticker_end_day(ofs, testDate);

		// Update total corrs.
		for( int i = 0; i < nInput; ++i )
		{
			vCorr[i] += vCorrDay[iDay][i];
			vCorrTot[i] += vCorrTotDay[iDay][i];
		}
	}

	// Corr between inputs and residual targets.
	ofs << "\ncorr(input, restar)\n";
	for( vector<string>::iterator it = inputNames.begin(); it != inputNames.end(); ++it )
		ofs << *it << " ";
	ofs << "\n";

	for( int iDay = 0; iDay < nDays; ++iDay )
	{
		sprintf(buf, "%8d ", testDates[iDay]);
		ofs << buf;
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
	for( vector<string>::iterator it = inputNames.begin(); it != inputNames.end(); ++it )
		ofs << *it << " ";
	ofs << "\n";

	for( int iDay = 0; iDay < nDays; ++iDay )
	{
		sprintf(buf, "%8d ", testDates[iDay]);
		ofs << buf;
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

//void CorrInput::init_ticker_offset(int idate, string model, string sigtype, string fitdesc)
//{
//	corrTicker_.clear();
//	mIndxTicker_.clear();

//	string path = get_sigTxt_path(model, idate, sigtype, fitdesc);
//	ifstream ifs(path.c_str());
//	if( ifs.is_open() )
//	{
//		string line;
//		int indx = 0;
//		string last_ticker;
//		while(getline(ifs, line))
//		{
//			vector<string> sl = split(line, '\t');
//			if( sl[0] != "uid" )
//			{
//				string ticker = sl[1];
//				if( ticker != last_ticker )
//				{
//					mIndxTicker_[indx] = ticker;
//					last_ticker = ticker;
//				}
//				++indx;
//			}
//		}
//	}
//}

void CorrInput::process_ticker(int indx, double mret60, double totTarget)
{
	map<int, string>::iterator itt = mIndxTicker_.find(indx);
	static string ticker;

	if( itt != mIndxTicker_.end() )
	{
		if( indx > 0 )
		{
			if( !ticker.empty() )
				mCorrTicker_.insert(make_pair(corrTicker_.corr() * 100., ticker));
		}
		corrTicker_.clear();
		ticker = itt->second;
	}

	corrTicker_.add(mret60, totTarget);
}

void CorrInput::debug_ticker_end_day(ofstream &ofs, int testDate)
{
	// last ticker.
	if( !mIndxTicker_.empty() )
	{
		string ticker = mIndxTicker_.rbegin()->second;
		mCorrTicker_.insert(make_pair(corrTicker_.corr() * 100., ticker));
	}
	corrTicker_.clear();

	ofs << testDate << "\n";
	for( multimap<double, string>::iterator it = mCorrTicker_.begin(); it != mCorrTicker_.end(); ++it )
	{
		char buf[100];
		sprintf(buf, "%6s %8.3f\n", it->second.c_str(), it->first);
		ofs << buf;
	}
	mCorrTicker_.clear();
}
