#include <MOrders/TradeQuantileInfo.h>
#include <jl_lib/jlutil.h>
#include <vector>
#include <iterator>
using namespace std;

TradeQuantileInfo::TradeQuantileInfo(int nBins)
	:nBins_(nBins)
{
}

void TradeQuantileInfo::addTrades(const vector<MercatorTrade>& trades)
{
	for(auto& t : trades)
	{
		if(t.qty > 0)
		{
			vDayVolat_.push_back(t.cv.dayVolat);
			vDayVolatSurprise_.push_back(t.cv.dayVolatSurprise);
			vDayVolume_.push_back(t.cv.dayVolume);
			vDayVolumeSurprise_.push_back(t.cv.dayVolumeSurprise);
			vVolat_.push_back(t.cv.volat);
			vVolatSurprise_.push_back(t.cv.volatSurprise);
			vDv_.push_back(t.cv.dv);
			vDvSurprise_.push_back(t.cv.dvSurprise);
			vShare_.push_back(t.cv.share);
			vShareSurprise_.push_back(t.cv.shareSurprise);
			vMercatorTrade_.push_back(t.cv.mercatorTrade);
			vMercatorTradeSurprise_.push_back(t.cv.mercatorTradeSurprise);
			vShare1s_.push_back(t.cv.share1s);
			vShare60s_.push_back(t.cv.share60s);
			vShare3600s_.push_back(t.cv.share3600s);
		}
	}
}

void TradeQuantileInfo::finish(bool debug)
{
	if(debug)
	{
		printf("trdEdges ");
		int nBinsDebug = 40;
		sort(begin(vVolatSurprise_), end(vVolatSurprise_));
		int nTot = vVolatSurprise_.size();
		for(int i = 1; i < nBinsDebug; ++i)
			printf("%6.3f ", vVolatSurprise_[i * nTot / nBinsDebug]);
		cout << endl;
	}

	finish_var(vDayVolat_, vEdgeDayVolat_);
	finish_var(vDayVolatSurprise_, vEdgeDayVolatSurprise_);
	finish_var(vDayVolume_, vEdgeDayVolume_);
	finish_var(vDayVolumeSurprise_, vEdgeDayVolumeSurprise_);
	finish_var(vVolat_, vEdgeVolat_);
	finish_var(vVolatSurprise_, vEdgeVolatSurprise_);
	finish_var(vDv_, vEdgeDv_);
	finish_var(vDvSurprise_, vEdgeDvSurprise_);
	finish_var(vShare_, vEdgeShare_);
	finish_var(vShareSurprise_, vEdgeShareSurprise_);
	finish_var(vMercatorTrade_, vEdgeMercatorTrade_);
	finish_var(vMercatorTradeSurprise_, vEdgeMercatorTradeSurprise_);
	finish_var(vShare1s_, vEdgeShare1s_);
	finish_var(vShare60s_, vEdgeShare60s_);
	finish_var(vShare3600s_, vEdgeShare3600s_);
}

void TradeQuantileInfo::finish_var(vector<float>& v, vector<float>& vEdge)
{
	sort(begin(v), end(v));

	vEdge = vector<float>(nBins_ + 1);
	vEdge[0] = -max_float_;
	vEdge[nBins_] = max_float_;
	int nTot = v.size();
	for(int i = 1; i < nBins_; ++i)
		vEdge[i] = v[i * nTot / nBins_];

	vector<float>().swap(v);
}

int TradeQuantileInfo::getQuantile(const MercatorTrade& trade, const string& condVar) const
{
	int iQ = -1;
	if(condVar == "dayVolat")
		iQ = getQuantile(trade.cv.dayVolat, vEdgeDayVolat_);
	else if(condVar == "dayVolatSurprise")
		iQ = getQuantile(trade.cv.dayVolatSurprise, vEdgeDayVolatSurprise_);
	else if(condVar == "dayVolume")
		iQ = getQuantile(trade.cv.dayVolume, vEdgeDayVolume_);
	else if(condVar == "dayVolumeSurprise")
		iQ = getQuantile(trade.cv.dayVolumeSurprise, vEdgeDayVolumeSurprise_);
	else if(condVar == "volat")
		iQ = getQuantile(trade.cv.volat, vEdgeVolat_);
	else if(condVar == "volatSurprise")
		iQ = getQuantile(trade.cv.volatSurprise, vEdgeVolatSurprise_);
	else if(condVar == "dv")
		iQ = getQuantile(trade.cv.dv, vEdgeDv_);
	else if(condVar == "dvSurprise")
		iQ = getQuantile(trade.cv.dvSurprise, vEdgeDvSurprise_);
	else if(condVar == "share")
		iQ = getQuantile(trade.cv.share, vEdgeShare_);
	else if(condVar == "shareSurprise")
		iQ = getQuantile(trade.cv.shareSurprise, vEdgeShareSurprise_);
	else if(condVar == "mercatorTrade")
		iQ = getQuantile(trade.cv.mercatorTrade, vEdgeMercatorTrade_);
	else if(condVar == "mercatorTradeSurprise")
		iQ = getQuantile(trade.cv.mercatorTradeSurprise, vEdgeMercatorTradeSurprise_);
	else if(condVar == "share1s")
		iQ = getQuantile(trade.cv.share1s, vEdgeShare1s_);
	else if(condVar == "share60s")
		iQ = getQuantile(trade.cv.share60s, vEdgeShare60s_);
	else if(condVar == "share3600s")
		iQ = getQuantile(trade.cv.share3600s, vEdgeShare3600s_);
	return iQ;
}

int TradeQuantileInfo::getQuantile(float val, const vector<float>& vEdge) const
{
	static int cnt = 0;
	auto it = lower_bound(begin(vEdge), end(vEdge), val);
	auto it2 = upper_bound(begin(vEdge), end(vEdge), val);
	int span = distance(it, it2) + 1;
	int iQ = (span > 0) ? distance(begin(vEdge), it + cnt++ % span) : distance(begin(vEdge), it);
	return iQ;
}
