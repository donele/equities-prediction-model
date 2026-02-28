#include <gtlib_fitana/AnaObjectBySprd.h>
#include <gtlib_fitana/AnaDayStatMaker.h>
#include <gtlib_fitana/AnaAllDaysStat.h>
using namespace std;

namespace gtlib {

AnaObjectBySprd::AnaObjectBySprd()
{
}

AnaObjectBySprd::AnaObjectBySprd(float minSprd, float maxSprd, int nTrees, float fee)
	:minSprd_(minSprd),
	maxSprd_(maxSprd),
	nTrees_(nTrees),
	fee_(fee)
{
}

void AnaObjectBySprd::addData(TargetPred* pTargetPred)
{
	int predIndex = pTargetPred->getPredIndex("pred" + itos(nTrees_));
	vector<int> idates = pTargetPred->getDailyDataCount().getIdates();
	for( int idate : idates )
	{
		AnaDayStatMaker dsMaker;
		int offset1 = pTargetPred->getDailyDataCount().getOffset(idate);
		int offset2 = offset1 + pTargetPred->getDailyDataCount().getNdata(idate);
		for( int iSample  = offset1; iSample  < offset2; ++iSample  )
		{
			float sprd = pTargetPred->sprd(iSample);
			if( sprd >= minSprd_ & sprd < maxSprd_ )
			{
				float target = pTargetPred->target(iSample);
				float bmpred = pTargetPred->bmpred(iSample);
				float pred = pTargetPred->pred(iSample, predIndex);
				float cost = .5 * sprd + fee_;
				dsMaker.addData(target, bmpred, pred, cost);
			}
		}
		AnaDayStat ds = dsMaker.getDayStat();
		mStat_[idate] = ds;
	}
}

void AnaObjectBySprd::writeByDay(ostream& os, int idate)
{
	os << idate << "\t" << minSprd_ << "\t" << maxSprd_ << "\t" << nTrees_ << "\t";
	os << mStat_[idate];
}

void AnaObjectBySprd::writeSumm(ostream& os)
{
	os << minSprd_ << "\t" << maxSprd_ << "\t" << nTrees_ << "\t";
	AnaAllDaysStat adStat(mStat_);
	os << adStat;
}

pair<int, float> AnaObjectBySprd::getNtreeMbias()
{
	AnaAllDaysStat adStat(mStat_);
	float mbias = adStat.getMbias();
	pair<int, float> p = make_pair(nTrees_, mbias);
	return p;
}

} // namespace gtlib
