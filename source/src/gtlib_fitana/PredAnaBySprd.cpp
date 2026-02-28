#include <gtlib_fitana/PredAnaBySprd.h>
#include <gtlib_fitana/AnaObjectBySprd.h>
#include <gtlib_fitting/fittingFtns.h>
#include <jl_lib/jlutil.h>
#include <thread>
#include <fstream>
#include <boost/filesystem.hpp>
using namespace std;

namespace gtlib {

PredAnaBySprd::PredAnaBySprd()
{
}

PredAnaBySprd::PredAnaBySprd(TargetPred* pTargetPred,
		const vector<pair<float, float>>& vSprdRanges,
		const string& fitDir, int udate, float fee)
	:statDir_(getStatDir(fitDir, udate)),
	vSprdRanges_(vSprdRanges),
	pTargetPred_(pTargetPred),
	fee_(fee)
{
	mkd(statDir_);
	idates_ = pTargetPred_->getDailyDataCount().getIdates();
}

PredAnaBySprd::~PredAnaBySprd()
{
	for( auto& pAnaObj : vpAnaObj_ )
		delete pAnaObj;
}

void PredAnaBySprd::analyze()
{
	createAnaObjs();
	addDataToAnaObjs();
}

void PredAnaBySprd::createAnaObjs()
{
	int NS = vSprdRanges_.size();
	int NP = pTargetPred_->getNPredSeries();
	for( int iS = 0; iS < NS; ++iS )
	{
		for( int iP = 0; iP < NP; ++iP )
			vpAnaObj_.push_back(newAnaObject(iS, iP));
	}
}

AnaObject* PredAnaBySprd::newAnaObject(int iS, int iP)
{
	float minSprd = vSprdRanges_[iS].first;
	float maxSprd = vSprdRanges_[iS].second;
	int nTrees = getNTrees(iP);
	return new AnaObjectBySprd(minSprd, maxSprd, nTrees, fee_);
}

void PredAnaBySprd::addDataToAnaObjs()
{
	bool singleThread = false;
	if( singleThread ) // single thread.
	{
		for( auto pAnaObj : vpAnaObj_ )
			pAnaObj->addData(pTargetPred_);
	}
	else // multi thread.
	{
		vector<thread> vThread;
		for( auto pAnaObj : vpAnaObj_ )
			vThread.push_back(thread(&AnaObject::addData, pAnaObj, pTargetPred_));
		for( auto& t : vThread )
			t.join();
	}
}

void PredAnaBySprd::write()
{
	vector<thread> vThread;
	vThread.push_back(thread(&PredAnaBySprd::writeByDay, this));
	vThread.push_back(thread(&PredAnaBySprd::writeSumm, this));
	for( auto& t : vThread )
		t.join();
}

void PredAnaBySprd::writeByDay()
{
	ofstream ofsByDay(getByDayPath().c_str());
	writeByDayHeader(ofsByDay);
	writeByDayContent(ofsByDay);
}

string PredAnaBySprd::getByDayPath()
{
	int idateFrom = *idates_.begin();
	int idateTo = *idates_.rbegin();
	string filename = "oos_" + itos(idateFrom) + "_" + itos(idateTo) + ".txt";
	string path = statDir_ + "/" + filename;
	return path;
}

void PredAnaBySprd::writeByDayHeader(ostream& os)
{
	os << "date\tmnSp\tmxSp\tntree"
		<< "\tcbm\tctb\tebm\tetb\tofbm\toftb\tmpredbm\tmpredtb\tnpts"
		<< "\tmevbm\tmevtb\tbiasbm\tbiastb\tntrdbm\tntrdtb\n";
}

void PredAnaBySprd::writeByDayContent(ostream& os)
{
	for( int idate : idates_ )
	{
		for( auto pAnaObj : vpAnaObj_ )
			pAnaObj->writeByDay(os, idate);
	}
}

void PredAnaBySprd::writeSumm()
{
	ofstream ofsSumm(getSummPath().c_str());
	writeSummHeader(ofsSumm);
	writeSummContent(ofsSumm);
}

string PredAnaBySprd::getSummPath()
{
	int idateFrom = *idates_.begin();
	int idateTo = *idates_.rbegin();
	string filename = "anaoos_" + itos(idateFrom) + "_" + itos(idateTo) + ".txt";
	string path = statDir_ + "/" + filename;
	return path;
}

void PredAnaBySprd::writeSummHeader(ostream& os)
{
	os << "mnSp\tmxSp\tntree"
	   	<< "\tcbm\tctb\tebm\tetb\tofbm\toftb\tmpredbm\tmpredtb\tnpts"
		<< "\tthres\tmevbm\tmevtb\tbiasbm\tbiastb\tntrdbm\tntrdtb"
		<< "\tavgPnlbm\tavgPnltb\tshrpRat\n";
}

void PredAnaBySprd::writeSummContent(ostream& os)
{
	for( auto pAnaObj : vpAnaObj_ )
		pAnaObj->writeSumm(os);
}

int PredAnaBySprd::getNTrees(int iPred)
{
	string predLabel = pTargetPred_->getPredLabel(iPred);
	int nTrees = -1;
	if( predLabel.size() > 4 )
		nTrees = atoi(predLabel.substr(4, predLabel.size() - 4).c_str());
	return nTrees;
}

vector<pair<int, float>> PredAnaBySprd::getNtreeMbias()
{
	vector<pair<int, float>> vNtreeMbias;
	for( auto pAnaObj : vpAnaObj_ )
		vNtreeMbias.push_back(pAnaObj->getNtreeMbias());
	return vNtreeMbias;
}
} // namespace gtlib
