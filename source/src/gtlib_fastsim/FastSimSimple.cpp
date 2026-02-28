#include <gtlib_fastsim/FastSimSimple.h>
using namespace std;

// endDay
//   getData
//     getSample
//   getStat

namespace gtlib {

FastSimSimple::FastSimSimple(const string& name, const string& baseDir,
		const string& fitDir, const vector<string>& vPredModel, const vector<string>& vSigModel,
		const vector<string>& vTargetName, const vector<double>& vWgt,
		double restore, double thres, int maxShrTrade, double maxDollarPosNet, double maxDollarPosTicker)
	:FastSim(name, baseDir, fitDir, vPredModel, vSigModel, vTargetName, vWgt, restore, thres, maxShrTrade, maxDollarPosNet, maxDollarPosTicker)
{
}

void FastSimSimple::getData(vector<PredWholeDay*>& vPredDay,
		const map<string, string>* mTicker2Uid, const map<string, vector<double>>* mMid)
{
	map<string, vector<double> >::const_iterator mMidEnd = mMid->end();
	mTickerSample_.clear();
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
				vector<SampleData>& vSample = mTickerSample_[ticker];
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
						SampleData newSample = getSample(vPredDay, ticker, msecs, itMid->second);
						vSample.push_back(newSample);
					}
				}
			}
		}
	}
}

void FastSimSimple::getIntradayInfo(DailySimStat& dss,
		const map<string, CloseInfo>* mClose,
		const map<string, string>* mTicker2Uid)
{
	map<string, CloseInfo>::const_iterator itcEnd = mClose->end();
	map<string, string>::const_iterator ittEnd = mTicker2Uid->end();
	for( auto& kv : mTickerSample_ )
	{
		auto& ticker = kv.first;
		auto& tdata = mTickerData_[ticker];
		auto& vSample = kv.second;

		double tickerIntra = 0.;
		int lastMsecs = 0;
		auto itt = mTicker2Uid->find(ticker);
		if( itt != ittEnd )
		{
			string uid = itt->second;
			auto itc = mClose->find(uid);
			int lastSide = 0;
			double lastPrice = 0.;
			int nRepeat = 0;
			double repeatPnl = 0.;
			double tickerPnl = 0.;
			int tickerNtrd = 0;
			if( itc != itcEnd )
			{
				const CloseInfo& ci = itc->second;
				for( auto& sample : vSample )
				{
					// new code. Use tradePrice for repeat check. Use correct price for intra, clop, and clcl.
					int dPos = sample.getDPos(restore_, tdata.pos, maxShrTrade_, maxDollarPosTicker_,
							netDollarPos_, maxDollarPosNet_, thres_, openMsecs_, closeMsecs_);
					double tradePrice = sample.getTradePrice(dPos);
					bool repeatTrade = lastSide != 0 && lastPrice > 0. && lastSide * dPos > 0 && abs(lastPrice - tradePrice) < ltmb_;
					if( repeatTrade )
						++nRepeat;
					if( debug_ && repeatTrade )
					{
						//cout << "repeat " << ticker << '\t' << lastMsecs << '\t' << (static_cast<int>(sample.msecs)-lastMsecs)/1000 << endl;
						double retIntra = dPos * (ci.close / sample.price - 1.) - abs(dPos * sample.cost);
						repeatPnl += retIntra * tradePrice;
					}
					if( dPos != 0 && !repeatTrade )
					{
						tdata.pos += dPos;
						double retIntra = dPos * (ci.close / sample.price - 1.) - abs(dPos * sample.cost);
						double retClop = dPos * ci.retclop;
						double retClcl = dPos * ci.retclcl;

						dss.retIntra += retIntra;
						dss.retClop += retClop;
						dss.retClcl += retClcl;

						dss.intra += retIntra * tradePrice;
						dss.clop += retClop * ci.close;
						dss.clcl += retClcl * ci.close;

						double dv = abs(dPos) * tradePrice;
						dss.dv += dv;
						if( dPos > 0 )
							dss.dvBuy += dv;
						++dss.n;

						lastMsecs = sample.msecs;
						lastPrice = tradePrice;
						lastSide = (dPos > 0) ? 1 : ((dPos < 0) ? -1 : 0);

						tickerPnl += retIntra * tradePrice;
						++tickerNtrd;
					}
				}
				if( debug_ )
				{
					//cout << ticker << '\t' << " repeat " << nRepeat << " of " << dss.n + nRepeat << endl;
					printf("%8s rpt %d of %d pnl %.3f rPnl %.3f\n", ticker.c_str(), nRepeat, tickerNtrd+nRepeat, tickerPnl, repeatPnl);
				}
			}
		}
	}
}

} // namespace gtlib

