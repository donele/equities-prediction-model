#include <gtlib_fitting/PredFromBoostedTree.h>
#include <gtlib_fitting/BoostedDecisionTree.h>
#include <gtlib_fitting/fittingFtns.h>
#include <boost/filesystem.hpp>
#include <thread>
using namespace std;
namespace fs = boost::filesystem;

namespace gtlib {

PredFromBoostedTree::PredFromBoostedTree()
{
}

PredFromBoostedTree::PredFromBoostedTree(FitData* pFitData, const string& fitDir, const string& coefFitDir, int udate, bool costWgt)
	:modelSource_("disk"),
	udate_(udate),
	useOnlyOneModel_(udate > 0),
	costWgt_(costWgt),
	fitDir_(fitDir),
	coefFitDir_(coefFitDir),
	pFitData_(pFitData),
	pvvPred_(nullptr),
	lenPredSeries_(0),
	sprdIndex_(-1),
	bidSizeIndex_(-1),
	bidIndex_(-1),
	askIndex_(-1),
	askSizeIndex_(-1),
	intraTarIndex_(-1)
{
}

PredFromBoostedTree::PredFromBoostedTree(FitData* pFitData, const string& fitDir, const string& dbname, const string& dbtable, const string& mkt, int udate, bool costWgt)
	:modelSource_("db"),
	udate_(udate),
	useOnlyOneModel_(udate > 0),
	costWgt_(costWgt),
	fitDir_(fitDir),
	dbname_(dbname),
	dbtable_(dbtable),
	mkt_(mkt),
	pFitData_(pFitData),
	pvvPred_(nullptr),
	lenPredSeries_(0),
	sprdIndex_(-1),
	bidSizeIndex_(-1),
	bidIndex_(-1),
	askIndex_(-1),
	askSizeIndex_(-1),
	intraTarIndex_(-1)
{
}

PredFromBoostedTree::~PredFromBoostedTree()
{
	for( auto& kv : mUdateFitter_ )
		delete kv.second;
	if( pvvPred_ )
		delete pvvPred_;
}

// calculatePred() -------------------------------------------------------------

void PredFromBoostedTree::calculatePred()
{
	fitDates_ = pFitData_->getDailyDataCount().getIdates();
	pvvPred_ = new vector<vector<float>>(pFitData_->nSamplePoints);
	readModels();
	bool singleThread = false;
	if( singleThread )
	{
		for( int idate : fitDates_ )
			calculatePredDay(idate);
	}
	else
	{
		vector<thread> vThread;
		for( int idate : fitDates_ )
			vThread.push_back(thread(&PredFromBoostedTree::calculatePredDay, this, idate));
		for( auto& t : vThread )
			t.join();
	}
} 

vector<int> PredFromBoostedTree::getUsedModelDates()
{
	vector<int> modelDates = getModelDates(coefFitDir_);
	int minModelDate = getMinModelDate(modelDates, fitDates_[0]);
	int maxFitDate = fitDates_[fitDates_.size() - 1];

	vector<int> usedDates;
	for( int modelDate : modelDates )
	{
		if( modelDate >= minModelDate && modelDate <= maxFitDate )
			usedDates.push_back(modelDate);
	}
	return usedDates;
}

void PredFromBoostedTree::readModels()
{
	if(modelSource_ == "disk")
	{
		if( useOnlyOneModel_ )
			mUdateFitter_[udate_] = new BoostedDecisionTree(pFitData_, getModelPath(coefFitDir_, udate_));
		else
		{
			vector<int> usedModelDates = getUsedModelDates();
			for( int modelDate : usedModelDates )
				mUdateFitter_[modelDate] = new BoostedDecisionTree(pFitData_, getModelPath(coefFitDir_, modelDate));
		}
	}
	else if(modelSource_ == "db")
	{
		if( useOnlyOneModel_ )
			mUdateFitter_[udate_] = new BoostedDecisionTree(pFitData_, dbname_, dbtable_, mkt_, udate_);
	}
}

int PredFromBoostedTree::getMinModelDate(const vector<int>& modelDates, int minFitDate)
{
	int minModelDate = 0;
	for( int modelDate : modelDates )
	{
		if( modelDate > minModelDate && modelDate <= minFitDate )
			minModelDate = modelDate;
	}
	return minModelDate;
}

void PredFromBoostedTree::calculatePredDay(int idate)
{
	BoostedDecisionTree* fitter = getFitter(idate);
	int iSampleFrom = pFitData_->getDailyDataCount().getOffset(idate);
	int iSampleTo = iSampleFrom + pFitData_->getDailyDataCount().getNdata(idate);
	for( int iSample = iSampleFrom; iSample < iSampleTo; ++iSample )
		(*pvvPred_)[iSample] = fitter->getPredSeries(iSample);
}

BoostedDecisionTree* PredFromBoostedTree::getFitter(int idate)
{
	int modelDate = getModelDate(idate);
	if( !mUdateFitter_.count(modelDate) )
	{
		cerr << "ERROR: model " << modelDate << " not available.\n";
		exit(13);
	}
	return mUdateFitter_[modelDate];
}

int PredFromBoostedTree::getModelDate(int idate)
{
	int modelDate = 0;
	if( useOnlyOneModel_ )
		modelDate = udate_;
	else
	{
		for( auto& kv : mUdateFitter_ )
		{
			if( kv.first <= idate )
				modelDate = kv.first;
			else
				break;
		}
	}
	return modelDate;
}

// writePred() ----------------------------------------------------------------

void PredFromBoostedTree::writePred()
{
	if( !pvvPred_ )
	{
		cout << "PrdFromBoostedTree::writePred(): predictions are not calculated.\n";
		return;
	}

	sprdIndex_ = pFitData_->getSpectatorIndex("sprd");
	//priceIndex_ = pFitData_->getSpectatorIndex("logPrice");
	bidSizeIndex_ = pFitData_->getSpectatorIndex("bidSize");
	bidIndex_ = pFitData_->getSpectatorIndex("bid");
	askIndex_ = pFitData_->getSpectatorIndex("ask");
	askSizeIndex_ = pFitData_->getSpectatorIndex("askSize");
	intraTarIndex_ = pFitData_->getSpectatorIndex("tarCloseuh");
	for( int idate : fitDates_ )
	{
		int modelDate = getModelDate(idate);
		string predDir = getPredDir(fitDir_, modelDate);
		mkd(predDir);
		ofstream fpred(getPredPath(fitDir_, modelDate, idate).c_str());
		if( fpred.is_open() )
		{
			fpred << getPredHeader();
			int iSampleFrom = pFitData_->getDailyDataCount().getOffset(idate);
			int iSampleTo = iSampleFrom + pFitData_->getDailyDataCount().getNdata(idate);
			for( int i = iSampleFrom; i < iSampleTo; ++i )
				printPredRow(fpred, i);
		}
	}
}

string PredFromBoostedTree::getPredHeader()
{
	string header = "target\tbmpred\tpred\tsprd\tprice\tintraTar\tbidSize\taskSize";
	vector<string> predSeriesNames = mUdateFitter_.begin()->second->getPredSeriesNames();
	lenPredSeries_ = predSeriesNames.size();
	for( string name : predSeriesNames )
		header += "\t" + name;
	header += "\n";
	return header;
}

void PredFromBoostedTree::printPredRow(ostream& os, int iSample)
{

	float bmpred = pFitData_->bmpred(iSample);
	float target = pFitData_->target(iSample) + bmpred;
	float pred = *(*pvvPred_)[iSample].rbegin() + bmpred;
	float sprd = pFitData_->spectator(sprdIndex_, iSample);
	float intraTar = pFitData_->spectator(intraTarIndex_, iSample);
	int bidSize = pFitData_->spectator(bidSizeIndex_, iSample);
	float bid = pFitData_->spectator(bidIndex_, iSample);
	float ask = pFitData_->spectator(askIndex_, iSample);
	int askSize = pFitData_->spectator(askSizeIndex_, iSample);
	float price = (bid + ask) / 2.;

	float halfSprd = sprd * .5;
	if(costWgt_)
	{
		float avgCost = pFitData_->avgCost(iSample);
		if(avgCost > 0.)
			pred = *(*pvvPred_)[iSample].rbegin() * avgCost + bmpred;
		else
			pred = 0.;
	}

	char buf[1000];
	sprintf(buf, "%.4f\t%.4f\t%.4f\t%.2f\t%.4f\t%.4f\t%d\t%d", target, bmpred, pred, sprd, price, intraTar, bidSize, askSize);

	vector<float>& predSeries = (*pvvPred_)[iSample];
	//for( float pred : predSeries )
	for(int i = 0; i < lenPredSeries_; ++i)
	{
		float pred = predSeries[i];
		char newbuf[100];
		if(costWgt_)
			sprintf(newbuf, "\t%.4f", pred * halfSprd + bmpred);
		else
			sprintf(newbuf, "\t%.4f", pred + bmpred);
		strcat(buf, newbuf);
	}
	strcat(buf, "\n");
	os << buf;
}

} // namespace gtlib

