#include <gtlib_fastsim/FastSimNet.h>
using namespace std;

// endDay
//   getData
//     getSample
//   getStat

namespace gtlib {

FastSimNet::FastSimNet(const string& name, const string& baseDir,
		const string& fitDir, const vector<string>& vPredModel, const vector<string>& vSigModel,
		const vector<string>& vTargetName, const vector<double>& vWgt,
		double restore, double thres, int maxShrTrade, double maxDollarPosNet, double maxDollarPosTicker)
	:FastSim(name, baseDir, fitDir, vPredModel, vSigModel, vTargetName, vWgt, restore, thres, maxShrTrade, maxDollarPosNet, maxDollarPosTicker)
{
}

void FastSimNet::getData(vector<PredWholeDay*>& vPredDay,
		const map<string, string>* mTicker2Uid, const map<string, vector<double>>* mMid)
{
	vSample_.clear();
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
						vSample_.push_back(newSample);
					}
				}
			}
		}
	}

	// sort sample.
	sort(begin(vSample_), end(vSample_));
}

void FastSimNet::getIntradayInfo(DailySimStat& dss,
		const map<string, CloseInfo>* mClose,
		const map<string, string>* mTicker2Uid)
{
	map<string, CloseInfo>::const_iterator itcEnd = mClose->end();
	map<string, string>::const_iterator ittEnd = mTicker2Uid->end();
	for( auto& sample : vSample_ )
	{
		auto& ticker = sample.ticker;
		auto& tdata = mTickerData_[ticker];
		auto itt = mTicker2Uid->find(ticker);
		if( itt != ittEnd )
		{
			string uid = itt->second;
			auto itc = mClose->find(uid);
			if( itc != itcEnd )
			{
				const CloseInfo& ci = itc->second;
				{
					int dPos = sample.getDPos(restore_, tdata.pos, maxShrTrade_, maxDollarPosTicker_,
							netDollarPos_, maxDollarPosNet_, thres_, openMsecs_, closeMsecs_);
					double tradePrice = sample.getTradePrice(dPos);
					bool repeatTrade = tdata.lastSide != 0 && tdata.lastPrice > 0.
						&& tdata.lastSide * dPos > 0 && abs(tdata.lastPrice - tradePrice) < ltmb_;
					if( dPos != 0 )
					{
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

						tdata.pos += dPos;
						tdata.dvDay += dv;
						netDollarPos_ += dPos * sample.price;

						tdata.lastPrice = tradePrice;
						tdata.lastSide = (dPos > 0) ? 1 : ((dPos < 0) ? -1 : 0);
					}
				}
			}
		}
	}
}

} // namespace gtlib

