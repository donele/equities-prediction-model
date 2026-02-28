#ifndef __gtlib_predana_GsprFtns__
#define __gtlib_predana_GsprFtns__
#include <gtlib_predana/GsprStatDef.h>
#include <iostream>
#include <vector>

namespace gtlib {

// ---------------------------------------------------------------------
// Subset given by vIndx.
// ---------------------------------------------------------------------

BasicStat calculateBasicStat(const std::vector<float>& vControl, const std::vector<int>& vIndx);
PredStat calculatePredStat(const std::vector<float>& vPred, const std::vector<float>& vTarget, const std::vector<int>& vIndx);
TopBottomStat calculateTopBottomStat(const std::vector<float>& vPred, const std::vector<float>& vTarget, const std::vector<int>& vIndx);
TradableStat calculateTradableStatSubset(const std::vector<int>& vMsecs, const std::vector<float>& vPred, const std::vector<float>& vTarget,
		const std::vector<float>& vSprdAdj, const std::vector<int>& vIndx, float thres = 0., float brk = 0., float maxPos = 0.);
TradableStat calculateTradableStatSubset(const std::vector<int>& vMsecs, const std::vector<float>& vPred, const std::vector<float>& vTotPred,
		const std::vector<float>& vTarget, const std::vector<float>& vSprdAdj, const std::vector<int>& vIndx,
		float thres = 0., float brk = 0., float maxPos = 0.);
GsprStat calculateGsprStatSubset(const std::vector<int>& vMsecs, const std::vector<float>& vPred, const std::vector<float>& vTarget,
		const std::vector<float>& vSprdAdj, const std::vector<int>& vIndx, float thres = 0., float brk = 0., float maxPos = 0., bool debug=false);
GsprStat calculateGsprStatSubset(const std::vector<int>& vMsecs, const std::vector<float>& vPred, const std::vector<float>& vTarget,
		const std::vector<float>& vSprdAdj, const std::vector<float>& vBid, const std::vector<float>& vAsk,
		const std::vector<int>& vIndx, float thres = 0., float brk = 0., float maxPos = 0., bool debug = false);
GsprStat calculateGsprStatExactSubset(const std::vector<int>& vMsecs, const std::vector<float>& vPred, const std::vector<float>& vTarget,
		const std::vector<float>& vSprdAdj,
		const std::vector<float>& vFee, const std::vector<float>& vBid, const std::vector<float>& vAsk, const std::vector<float>& vClosePrc,
		const std::vector<int>& vIndx, float thres = 0., float brk = 0., float maxPos = 0., bool debug = false);

// ---------------------------------------------------------------------
// Original functions.
// ---------------------------------------------------------------------

BasicStat calculateBasicStat(const std::vector<float>& vControl);
PredStat calculatePredStat(const std::vector<float>& vPred, const std::vector<float>& vTarget);
TopBottomStat calculateTopBottomStat(const std::vector<float>& vPred, const std::vector<float>& vTarget);
TradableStat calculateTradableStat(const std::vector<float>& vPred, const std::vector<float>& vTarget,
		const std::vector<float>& vSprdAdj, double thres = 0.);
TradableStat calculateTradableStat(const std::vector<float>& vPred, const std::vector<float>& vTotPred,
		const std::vector<float>& vTarget, const std::vector<float>& vSprdAdj,
		double thres = 0.);
TradableStat calculateTradableStat(const std::vector<float>& vPred, const std::vector<float>& vTotPred,
		const std::vector<float>& vTarget, const std::vector<float>& vSprdAdj,
		const std::vector<float>& vThres);
GsprStat calculateGsprStat(const std::vector<float>& vPred, const std::vector<float>& vTarget,
		const std::vector<float>& vSprdAdj, double thres = 0., bool debug = false);
GsprStat calculateGsprStat(const std::vector<float>& vPred, const std::vector<float>& vTarget,
		const std::vector<float>& vSprdAdj, const std::vector<float>& vThres, bool debug = false);
GsprStat calculateGsprStat(const std::vector<float>& vPred, const std::vector<float>& vTarget,
		const std::vector<float>& vSprdAdj, const std::vector<float>& vBid, const std::vector<float>& vAsk,
		double thres = 0., bool debug = false);
GsprStat calculateGsprStat(const std::vector<float>& vPred, const std::vector<float>& vTarget,
		const std::vector<float>& vSprdAdj, const std::vector<float>& vBid, const std::vector<float>& vAsk,
		const std::vector<float>& vThres, bool debug = false);

// ---------------------------------------------------------------------
// Simple print.
// ---------------------------------------------------------------------

void gsprPrintSimple(std::ostream& os, const BasicStat& bStat, const PredStat& pStat,
		const TopBottomStat& tbStat, const TradableStat& trdStat, const GsprStat& gStat);

void printHeaderQuantile(std::ostream& os);
void printContentQuantile(std::ostream& os, const BasicStat& bStat, const PredStat& pStat,
		const TopBottomStat& tbStat, const TradableStat& trdStat, const GsprStat& gStat);

void printHeaderNTree(std::ostream& os);
void printContentNTree(std::ostream& os, const BasicStat& bStat, const PredStat& pStat,
		const TopBottomStat& tbStat, const TradableStat& trdStat, const GsprStat& gStat);

// ---------------------------------------------------------------------
// Extend print.
// ---------------------------------------------------------------------

void gsprPrintExtend(std::ostream& os, const BasicStat& bStat, const PredStat& pStat,
		const TopBottomStat& tbStat, const TradableStat& trdStat, const GsprStat& gStat,
		const GsprStat& gStatIntra, const GsprStat& gStatPrice, const GsprStat& gStatIntraPrice, const GsprStat& gStatExactIntra);
void printHeaderExtendQuantile(std::ostream& os);
void printContentExtendQuantile(std::ostream& os, const BasicStat& bStat, const PredStat& pStat,
		const TopBottomStat& tbStat, const TradableStat& trdStat, const GsprStat& gStat,
		const GsprStat& gStatIntra, const GsprStat& gStatPrice, const GsprStat& gStatIntraPrice, const GsprStat& gStatExactIntra);
// ---------------------------------------------------------------------
// gsprPrintSimpleThres
// ---------------------------------------------------------------------
void gsprPrintSimpleThres(std::ostream& os, float thres, const BasicStat& bStat,
		const TradableStat& trdStat, const GsprStat& gStat);
void printHeaderQuantileSimpleThres(std::ostream& os);
void printContentQuantileSimpleThres(std::ostream& os, float thres, const BasicStat& bStat,
		const TradableStat& trdStat, const GsprStat& gStat);

// ---------------------------------------------------------------------
// gsprPrintExtendThres
// ---------------------------------------------------------------------
void gsprPrintThres(std::ostream& os, float thres, const BasicStat& bStat,
		const TradableStat& trdStat, const GsprStat& gStat,
		const GsprStat& gStatIntra, const GsprStat& gStatPrice, const GsprStat& gStatIntraPrice);
void printHeaderQuantileThres(std::ostream& os);
void printContentQuantileThres(std::ostream& os, float thres, const BasicStat& bStat,
		const TradableStat& trdStat, const GsprStat& gStat, const GsprStat& gStatIntra,
		const GsprStat& gStatPrice, const GsprStat& gStatIntraPrice);

// ---------------------------------------------------------------------
// MultTar print.
// ---------------------------------------------------------------------

void gsprPrintMultTar(std::ostream& os, const BasicStat& bStat,
		const PredStat& pStatTot, const PredStat& pStatOm, const PredStat& pStatTm,
		const TopBottomStat& tbStatTot,
		const TradableStat& trdStatTot, const TradableStat& trdStatOm, const TradableStat& trdStatTm,
		const GsprStat& gStat, const GsprStat& gStatIntra, const GsprStat& gStatIntraPrice);

void printHeaderMultTarQuantile(std::ostream& os);
void printContentMultTarQuantile(std::ostream& os, const BasicStat& bStat,
		const TopBottomStat& tbStatTot,
		const PredStat& pStatTot, const PredStat& pStatOm, const PredStat& pStatTm,
		const TradableStat& trdStatTot, const TradableStat& trdStatOm, const TradableStat& trdStatTm,
		const GsprStat& gStat, const GsprStat& gStatIntra, const GsprStat& gStatIntraPrice);

void printHeaderMultTarNTree(std::ostream& os);
void printContentMultTarNTree(std::ostream& os, const BasicStat& bStat,
		const TopBottomStat& tbStatTot,
		const PredStat& pStatTot, const PredStat& pStatOm, const PredStat& pStatTm,
		const TradableStat& trdStatTot, const TradableStat& trdStatOm, const TradableStat& trdStatTm,
		const GsprStat& gStat, const GsprStat& gStatIntra, const GsprStat& gStatIntraPrice);

// ---------------------------------------------------------------------
// gsprPrintMultTarThres
// ---------------------------------------------------------------------

void gsprPrintMultTarThres(std::ostream& os, double thres, const BasicStat& bStat,
		const TradableStat& trdStatTot, const TradableStat& trdStatOm, const TradableStat& trdStatTm,
		const GsprStat& gStat, const GsprStat& gStatIntra, const GsprStat& gStatIntraPrice);

void printHeaderMultTarNTreeThres(std::ostream& os);
void printContentMultTarNTreeThres(std::ostream& os, double thres, const BasicStat& bStat,
		const TradableStat& trdStatTot, const TradableStat& trdStatOm, const TradableStat& trdStatTm,
		const GsprStat& gStat, const GsprStat& gStatIntra, const GsprStat& gStatIntraPrice);

} // namespace gtlib

#endif
