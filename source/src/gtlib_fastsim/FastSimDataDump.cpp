#include <gtlib_fastsim/FastSimDataDump.h>
#include <gtlib_model/mdl.h>
#include <gtlib_model/mFtns.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_sigread/SignalBufferDecoratorHeader.h>
#include <gtlib_sigread/SignalBufferPredMulti.h>
#include <gtlib_predana/PredWholeDay.h>
#include <jl_lib/mto.h>
#include <boost/filesystem.hpp>
using namespace std;

// endDay
//   getData
//     getSample

namespace gtlib {

FastSimDataDump::FastSimDataDump(const string& baseDir,
		const string& fitDir, const vector<string>& vPredModel, const vector<string>& vSigModel,
		const vector<string>& vTargetName)
	:debug_(false),
	verbose_(1),
	baseDir_(baseDir),
	vPredModel_(vPredModel),
	vSigModel_(vSigModel),
	vTargetName_(vTargetName),
	nTarget_(vTargetName.size()),
	model_(vPredModel[0]),
	market_(mdl::markets(model_)[0])
{
	openMsecs_ = mto::msecOpen(market_);
	closeMsecs_ = mto::msecClose(market_);
}

void FastSimDataDump::endDay(int udate, int idate,
		const map<string, string>* mTicker2Uid, const map<string, vector<double>>* mMid)
{
	vector<PredWholeDay*> vPredDay;
	int nPred = vPredModel_.size();
	for( int i = 0; i < nPred; ++i )
		vPredDay.push_back(new PredWholeDay(getBinSigBuf(baseDir_, vPredModel_[i], vSigModel_[i], vTargetName_[i], udate, idate)));
	getData(idate, vPredDay, mTicker2Uid, mMid);
	for( auto p : vPredDay )
		delete p;
}

void FastSimDataDump::setDebug(bool x)
{
	debug_ = x;
}

SignalBuffer* FastSimDataDump::getBinSigBuf(const string& baseDir,
		const string& predModel, const string& sigModel, const string& targetName, int udate, int idate)
{
	SignalBuffer* pSigBuf = nullptr;
	string binPath = get_sig_path(baseDir, sigModel, idate, getSigType(targetName), "reg");
	string predPath = get_pred_path(baseDir, predModel, idate, targetName, "reg", udate);
	pSigBuf = new SignalBufferDecoratorHeader(
			new SignalBufferPredMulti(predPath), binPath);
	return pSigBuf;
}

EpisodeSampleRaw FastSimDataDump::getSample(vector<PredWholeDay*>& vPredDay, const string& ticker, int msecs,
		const vector<double>& vMid)
{
	float sprd = vPredDay[0]->getSprd(ticker, msecs) / basis_pts_;
	int sec = (msecs - openMsecs_) / 1000;
	float price = vMid[sec];
	float sprdAdj = .5 * sprd + mto::fee_bpt(model_, ExecFeesPrimex(model_, ticker), price) / basis_pts_;
	double bid = price - sprd * price / 2.;
	double ask = sprd * price / 2. + price;
	int bidSize = vPredDay[1]->getBidSize(ticker, msecs);
	int askSize = vPredDay[1]->getAskSize(ticker, msecs);

	EpisodeSampleRaw sample;
	uint64_t usecs = (uint64_t)msecs * 1000;
	sample.usecs = usecs;
	sample.bid = bid;
	sample.ask = ask;
	sample.bidSize = bidSize;
	sample.askSize = askSize;

	if( sprdAdj < 0. )
		sprdAdj = 0.;

	if( debug_ )
		printf("debug %6.2f ", sprdAdj * basis_pts_);
	double pred = 0.;
	for( int i = 0; i < nTarget_; ++i )
	{
		double predPiece = vPredDay[i]->getPred(ticker, msecs);

		switch (i) {
			case 0: sample.pred1 = predPiece;
					break;
			case 1: sample.pred2 = predPiece;
					break;
			case 2: sample.pred3 = predPiece;
					break;
		}

		if( debug_ )
			printf("%6.2f ", predPiece);
	}
	if( debug_ )
		printf("\n");

	return sample;
}

void FastSimDataDump::getData(int idate, vector<PredWholeDay*>& vPredDay,
		const map<string, string>* mTicker2Uid, const map<string, vector<double>>* mMid)
{
	string basename = market_ + itos(idate);
	string filename = basename + ".bin";
	string testfilename = basename + ".txt";
	ofstream outfile(filename.c_str(), ios::binary);
	ofstream testfile(testfilename.c_str());

	map<string, vector<double> >::const_iterator mMidEnd = mMid->end();
	map<string, vector<int>> tickerMsecs = vPredDay[0]->getTickerMsecs();
	for( auto& kv : tickerMsecs )
	{
		string ticker = kv.first;
		map<string, string>::const_iterator ittEnd = mTicker2Uid->end();
		auto itt = mTicker2Uid->find(ticker);
		if( itt != ittEnd )
		{
			string uid = itt->second;
			auto itMid = mMid->find(uid);
			if( itMid != mMidEnd )
			{
				vector<EpisodeSampleRaw> vSample;
				mTickerData_[ticker];
				for( int& msecs : kv.second )
				{
					bool allExist = true;
					for( auto p : vPredDay )
					{
						if( !p->exist(ticker, msecs) )
						{
							allExist = false;
							break;
						}
					}
					if( allExist )
					{
						EpisodeSampleRaw newSample = getSample(vPredDay, ticker, msecs, itMid->second);
						vSample.push_back(newSample);
					}
				}

				EpisodeHeader header;
				header.idate = idate;
				header.length = vSample.size();
				strcpy(header.ticker, ticker.c_str());

				outfile.write((char*)&header, sizeof(EpisodeHeader));
				for( auto& s : vSample )
					outfile.write((char*)&s, sizeof(EpisodeSampleRaw));

				testfile << header.idate << '\t' << header.length << '\t' << header.ticker << endl;
				for( auto& s : vSample )
					testfile << s.usecs << '\t' << s.bid << '\t' << s.ask << '\t' << s.bidSize << '\t' << s.askSize
						<< '\t' << s.pred1 << '\t' << s.pred2 << '\t' << s.pred3 << endl;
			}
		}
	}
	outfile.close();
	testfile.close();

	ifstream ifs(filename.c_str(), ios::binary);
	EpisodeHeader header;
	EpisodeSampleRaw s;
	ifs.read((char*)&header, sizeof(header));
	ifs.read((char*)&s, sizeof(s));
	cout << header.idate << '\t' << header.length << '\t' << header.ticker << endl;
	cout << s.usecs << '\t' << s.bid << '\t' << s.ask << '\t' << s.bidSize << '\t' << s.askSize
		<< '\t' << s.pred1 << '\t' << s.pred2 << '\t' << s.pred3 << endl;
}

}
