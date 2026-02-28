#include <gtlib_fitana/TargetPred.h>
#include <gtlib_fitana/PredLine.h>
#include <gtlib_fitting/fittingFtns.h>
#include <cassert>
#include <fstream>
#include <algorithm>
using namespace std;

namespace gtlib {

TargetPred::TargetPred()
{
}

TargetPred::TargetPred(const string& fitDir, vector<int> testDates, int udate)
	:TargetPred(fitDir, fitDir, testDates, udate)
{
}

TargetPred::TargetPred(const string& fitDir, const string& coefFitDir, vector<int> testDates, int udate)
{
	for( int idate : testDates )
	{
		string path = getPredPath(fitDir, coefFitDir, udate, idate);
		readPred(path, idate);
	}
}

TargetPred::~TargetPred()
{
}

void TargetPred::readPred(const string& path, int idate)
{
	ifstream ifs(path.c_str());
	if( ifs.is_open() )
	{
		cout << "Reading " << path << " ... ";
		cout.flush();
		string header;
		getline(ifs, header); // first line.
		PredLine predLine(header);
		assertHeader(header, predLine);

		int cnt = 0;
		for( ; ifs.rdstate() == 0; ++cnt )
		{
			ifs >> predLine;
			addLine(predLine);
		}
		ddCount_.add(idate, cnt);
		cout << "cnt " << cnt << endl;
	}
	else
		cerr << "file " << path << " is not open.\n";
}

void TargetPred::assertHeader(const string& header, PredLine& predLine)
{
	if( header_.empty() )
	{
		header_ = header;
		vPredLabel_ = predLine.getPredLabels();
	}
	else
		assert(header_ == header);
}

void TargetPred::addLine(PredLine& predLine)
{
	vSprd_.push_back(predLine.getSprd());
	vTarget_.push_back(predLine.getTarget());
	vBmpred_.push_back(predLine.getBmpred());
	vvPred_.push_back(predLine.getPredSeries());
}

DailyDataCount& TargetPred::getDailyDataCount()
{
	return ddCount_;
}

float& TargetPred::sprd(int iSample)
{
	return vSprd_[iSample];
}

float& TargetPred::target(int iSample)
{
	return vTarget_[iSample];
}

float& TargetPred::bmpred(int iSample)
{
	return vBmpred_[iSample];
}

float& TargetPred::pred(int iSample, int iSeries)
{
	return vvPred_[iSample][iSeries];
}

int TargetPred::getNPredSeries()
{
	return vPredLabel_.size();
}

string TargetPred::getPredLabel(int iSeries)
{
	return vPredLabel_[iSeries];
}

int TargetPred::getPredIndex(const string& label)
{
	return find(begin(vPredLabel_), end(vPredLabel_), label) - begin(vPredLabel_);
}

} // namespace gtlib
