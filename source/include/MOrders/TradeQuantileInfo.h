#ifndef __TradeQuantileInfo__
#define __TradeQuantileInfo__
#include <vector>
#include <jl_lib/MercatorTrade.h>

class TradeQuantileInfo {
public:
	TradeQuantileInfo(){}
	TradeQuantileInfo(int nBins);
	void addTrades(const std::vector<MercatorTrade>& trades);
	void finish(bool debug = false);
	void finish_var(std::vector<float>& v, std::vector<float>& vEdge);
	int getQuantile(const MercatorTrade& trade, const std::string& condVar) const;
private:
	int nBins_;

	std::vector<float> vDayVolat_;
	std::vector<float> vDayVolatSurprise_;
	std::vector<float> vDayVolume_;
	std::vector<float> vDayVolumeSurprise_;
	std::vector<float> vVolat_;
	std::vector<float> vVolatSurprise_;
	std::vector<float> vDv_;
	std::vector<float> vDvSurprise_;
	std::vector<float> vShare_;
	std::vector<float> vShareSurprise_;
	std::vector<float> vMercatorTrade_;
	std::vector<float> vMercatorTradeSurprise_;
	std::vector<float> vShare1s_;
	std::vector<float> vShare60s_;
	std::vector<float> vShare3600s_;

	std::vector<float> vEdgeDayVolat_;
	std::vector<float> vEdgeDayVolatSurprise_;
	std::vector<float> vEdgeDayVolume_;
	std::vector<float> vEdgeDayVolumeSurprise_;
	std::vector<float> vEdgeVolat_;
	std::vector<float> vEdgeVolatSurprise_;
	std::vector<float> vEdgeDv_;
	std::vector<float> vEdgeDvSurprise_;
	std::vector<float> vEdgeShare_;
	std::vector<float> vEdgeShareSurprise_;
	std::vector<float> vEdgeMercatorTrade_;
	std::vector<float> vEdgeMercatorTradeSurprise_;
	std::vector<float> vEdgeShare1s_;
	std::vector<float> vEdgeShare60s_;
	std::vector<float> vEdgeShare3600s_;

	int getQuantile(float val, const std::vector<float>& vEdge) const;
};

#endif
