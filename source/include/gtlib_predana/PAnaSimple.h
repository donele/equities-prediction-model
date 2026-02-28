#ifndef __gtlib_predana_PAnaSimple__
#define __gtlib_predana_PAnaSimple__
#include <gtlib_predana/GsprStatDef.h>
#include <iostream>
#include <vector>

namespace gtlib {

class PAnaSimple {
public:
	PAnaSimple(){}
	PAnaSimple(const std::vector<float>* vControl,
			const std::vector<int>* vMsecs, const std::vector<float>* vSprdAdj,
			const std::vector<float>* vPred, const std::vector<float>* vTarget,
			const std::vector<float>* vIntraTar, const std::vector<float>* vFee,
			const std::vector<float>* vBid, const std::vector<float>* vAsk,
			const std::vector<float>* vClosePrc, const std::vector<int>& vIndx, bool debug = false);
	~PAnaSimple(){}
	void calculate(float thres = 0., float brk = 0., float maxPos = 0.);
	void calculateThres(float thres);
	void calculateBreak(float brk);
	void calculateMaxPos(float maxPos);
	void print(std::ostream& os);
	void printThres(std::ostream& os);
	void printBreak(std::ostream& os);
	void printMaxPos(std::ostream& os);
private:
	bool debug_;
	float thres_;
	float brk_;
	float maxPos_;
	const std::vector<float>* vControl_;
	const std::vector<float>* vSprdAdj_;
	const std::vector<float>* vPred_;
	const std::vector<float>* vTarget_;
	const std::vector<float>* vIntraTar_;
	const std::vector<float>* vFee_;
	const std::vector<float>* vBid_;
	const std::vector<float>* vAsk_;
	const std::vector<float>* vClosePrc_;
	const std::vector<int>* vMsecs_;
	std::vector<int> vIndx_;
	BasicStat bStat_;
	PredStat pStat_;
	TopBottomStat tbStat_;
	TradableStat trdStat_;
	GsprStat gStat_;
	GsprStat gStatIntra_;
	GsprStat gStatPrice_;
	GsprStat gStatIntraPrice_;
	GsprStat gStatExactIntra_;
};

} // namespace gtlib

#endif
