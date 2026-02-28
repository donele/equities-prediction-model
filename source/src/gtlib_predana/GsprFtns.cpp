#include <gtlib_predana/GsprFtns.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/sort_util.h>
#include <jl_lib/stat_util.h>
#include <cmath>
#include <numeric>
using namespace std;

namespace gtlib {

// ---------------------------------------------------------------------
// Subset given by vIndx.
// ---------------------------------------------------------------------

BasicStat calculateBasicStat(const vector<float>& vControl, const vector<int>& vIndx)
{
	Acc acc(vControl, vIndx);
	BasicStat bStat;
	bStat.binAvg = acc.mean();
	bStat.binStdev = acc.stdev();
	bStat.npt = acc.n;
	return bStat;
}

PredStat calculatePredStat(const vector<float>& vPred, const vector<float>& vTarget, const vector<int>& vIndx)
{
	Acc accPred(vPred, vIndx);
	Acc accTarget(vTarget, vIndx);

	PredStat pStat;
	pStat.mX = accPred.mean();
	pStat.mY = accTarget.mean();

	pStat.dX = accPred.stdev();
	pStat.dY = accTarget.stdev();

	Corr corr(vPred, vTarget, vIndx);
	pStat.corr = corr.corr();

	pStat.lev = pStat.corr * pStat.dY;

	return pStat;
}

TopBottomStat calculateTopBottomStat(const vector<float>& vPred, const vector<float>& vTarget, const vector<int>& vIndx)
{
	int nData = vIndx.size();
	vector<float> vPredCompact(nData);
	for( int i = 0; i < nData; ++i )
		vPredCompact[i] = vPred[vIndx[i]];
	vector<int> vSortedIndex(nData);
	gsl_heapsort_index(vSortedIndex, vPredCompact);

	int n = nData * 0.01;
	double sumPredTop = 0.;
	double sumPredBottom = 0.;
	double sumTargetTop = 0.;
	double sumTargetBottom = 0.;
	for( int i = 0; i < n; ++i )
	{
		int iTop = vIndx[vSortedIndex[nData - i - 1]];
		int iBot = vIndx[vSortedIndex[i]];
		sumPredTop += vPred[iTop];
		sumPredBottom += vPred[iBot];
		sumTargetTop += vTarget[iTop];
		sumTargetBottom += vTarget[iBot];
	}

	TopBottomStat tbStat;
	tbStat.frcEv1p = (sumPredTop - sumPredBottom) / 2. / n;
	tbStat.ev1p = (sumTargetTop - sumTargetBottom) / 2. / n;
	return tbStat;
}

TradableStat calculateTradableStatSubset(const vector<int>& vMsecs, const vector<float>& vPred, const vector<float>& vTarget,
		const vector<float>& vSprdAdj, const vector<int>& vIndx, float thres, float brk, float maxPos)
{
	return calculateTradableStatSubset(vMsecs, vPred, vPred, vTarget, vSprdAdj, vIndx, thres, brk, maxPos);
}

TradableStat calculateTradableStatSubset(const vector<int>& vMsecs, const vector<float>& vPred, const vector<float>& vTotPred,
		const vector<float>& vTarget, const vector<float>& vSprdAdj, const vector<int>& vIndx, float thres, float brk, float maxPos)
{
	int nData = vIndx.size();
	int cnt = 0;
	double sumPredBuy = 0.;
	double sumPredSell = 0.;
	double sumTargetBuy = 0.;
	double sumTargetSell = 0.;
	int prevTradeMsecs = 0;
	int prevMsecs = 0;
	float pos = 0.;
	for( int i = 0; i < nData; ++i )
	{
		int msecs = vMsecs[i];
		int indx = vIndx[i];
		float sprd = vSprdAdj[indx];
		float pred = vPred[indx];
		float totPred = vTotPred[indx];
		float target = vTarget[indx];

		if( msecs < prevMsecs )
			pos = 0.;
		bool validTime = msecs < prevTradeMsecs || msecs > prevTradeMsecs + brk * 1000;
		bool tradable = totPred > sprd + thres / basis_pts_ || -totPred > sprd + thres / basis_pts_;
		bool validPos = maxPos < 1e-6 || (pos < maxPos && pos > -maxPos);
		if( validTime && tradable && validPos )
		{
			++cnt;
			if( totPred > 0. )
			{
				sumPredBuy += pred;
				sumTargetBuy += target;
				pos += 1;
			}
			else if( totPred < 0. )
			{
				sumPredSell += pred;
				sumTargetSell += target;
				pos -= 1;
			}
			prevTradeMsecs = msecs;
		}
		prevMsecs = msecs;
	}
	TradableStat trdStat;
	if( cnt > 0 )
	{
		trdStat.frcEvT = (sumPredBuy - sumPredSell) / cnt;
		trdStat.evT = (sumTargetBuy - sumTargetSell) / cnt;
		if( trdStat.evT != 0. )
		{
			int relOf = ceil((trdStat.frcEvT / trdStat.evT - 1.) * 100 - 0.5);

			int relOfClip = relOf;
			if( relOfClip > 49 )
				relOfClip = 49;
			else if( relOfClip < -51 )
				relOfClip = -51;

			int relOfAdj = relOfClip;
			if( relOfAdj < 0 )
				relOfAdj += 100;
			trdStat.relOf = relOfAdj;
		}
	}
	return trdStat;
}

GsprStat calculateGsprStatSubset(const vector<int>& vMsecs, const vector<float>& vPred,
		const vector<float>& vTarget, const vector<float>& vSprdAdj, const vector<int>& vIndx,
		float thres, float brk, float maxPos, bool debug)
{
	int nData = vPred.size();
	vector<float> vPrice(nData, 1.);
	return calculateGsprStatSubset(vMsecs, vPred, vTarget, vSprdAdj, vPrice, vPrice, vIndx, thres, brk, maxPos, debug);
}

GsprStat calculateGsprStatSubset(const vector<int>& vMsecs, const vector<float>& vPred, const vector<float>& vTarget,
		const vector<float>& vSprdAdj, const vector<float>& vBid, const vector<float>& vAsk, const vector<int>& vIndx,
		float thres, float brk, float maxPos, bool debug)
{
	int nData = vIndx.size();
	int gsprCnt = 0;
	float dv = 0.;
	float predGsprSum = 0.;
	float realGsprSum = 0.;
	int prevTradeMsecs = 0;
	int prevMsecs = 0;
	float pos = 0.;
	float debugSum = 0.;
	for( int i = 0; i < nData; ++i )
	{
		int msecs = vMsecs[i];
		int indx = vIndx[i];
		float sprd = vSprdAdj[indx];
		float pred = vPred[indx];
		float target = vTarget[indx];
		float price = pred > 0. ? vAsk[indx] : vBid[indx];

		if(debug)
		{
			printf("GGG %.4f %.4f %.4f\n", pred*basis_pts_, target*basis_pts_, sprd*basis_pts_);
			debugSum += (pred>0)?(target-sprd):(-target-sprd);
		}

		if( msecs < prevMsecs )
			pos = 0.;
		//bool validTime = msecs < prevTradeMsecs || msecs > prevTradeMsecs + brk * 1000; // Unreliable without ticker info.
		bool validTime = true;
		bool tradable = pred > sprd + thres / basis_pts_ || -pred > sprd + thres / basis_pts_;
		bool validPos = maxPos < 1e-6 || (pos < maxPos && pos > -maxPos);
		if( validTime && tradable && validPos )
		{
			++gsprCnt;
			dv += price;
			if( pred > 0. )
			{
				predGsprSum += (pred - sprd) * price;
				realGsprSum += (target - sprd) * price;
				pos += 1;
			}
			else if( pred < 0. )
			{
				predGsprSum += (-pred - sprd) * price;
				realGsprSum += (-target - sprd) * price;
				pos += -1;
			}
			prevTradeMsecs = msecs;
		}
		else
		{
			int x = 0;
			++i;
		}
		prevMsecs = msecs;
	}
	if(debug)
		printf("HHH nD %d dv %.1f realGsprSum %.4f debugSum %.4f\n", nData, dv, realGsprSum, debugSum);

	GsprStat gsprStat;
	gsprStat.gsprRat = (float)gsprCnt / (float)nData * 100.;
	gsprStat.predGspr = (dv > 0) ? predGsprSum / dv : 0.;
	gsprStat.realGspr = (dv > 0) ? realGsprSum / dv : 0.;
	gsprStat.plPpt = realGsprSum / nData;
	return gsprStat;
}

GsprStat calculateGsprStatExactSubset(const vector<int>& vMsecs, const vector<float>& vPred, const vector<float>& vTarget,
		const vector<float>& vSprdAdj,
		const vector<float>& vFee, const vector<float>& vBid, const vector<float>& vAsk, const vector<float>& vClosePrc, const vector<int>& vIndx,
		float thres, float brk, float maxPos, bool debug)
{
	int nData = vIndx.size();
	int gsprCnt = 0;
	float dv = 0.;
	float predGsprSum = 0.;
	float realGsprSum = 0.;
	int prevTradeMsecs = 0;
	int prevMsecs = 0;
	float pos = 0.;
	for( int i = 0; i < nData; ++i )
	{
		int msecs = vMsecs[i];
		int indx = vIndx[i];
		float sprd = vSprdAdj[indx];
		float sprdCost = (vAsk[indx] - vBid[indx]) / 2.;
		float mid = (vAsk[indx] + vBid[indx]) / 2.;
		float pred = vPred[indx];
		float predClosePrc = mid * (pred + 1.);
		float target = vTarget[indx]; // not used in this function.
		float closePrc = vClosePrc[indx];
		float price = pred > 0. ? vAsk[indx] : vBid[indx];
		float shr = 1. / price;
		float fee_bpt = vFee[indx];

		static float sumA = 0.;
		static float sumB = 0.;
		if(debug)
		{
			printf("GGG %.4f %.4f %.4f\n", pred*basis_pts_, target*basis_pts_, sprd*basis_pts_);
			if(pred > 0.)
			{
				sumA += target - sprd;
				sumB += (closePrc / price - 1. - fee_bpt/basis_pts_);
			}
			else
			{
				sumA += -target - sprd;
				sumB += (-closePrc / price + 1. - fee_bpt/basis_pts_);
			}
		}

		if( msecs < prevMsecs )
			pos = 0.;
		bool validTime = msecs < prevTradeMsecs || msecs > prevTradeMsecs + brk * 1000;
		bool tradable = pred > sprd + thres / basis_pts_ || -pred > sprd + thres / basis_pts_;
		bool validPos = maxPos < 1e-6 || (pos < maxPos && pos > -maxPos);
		if( validTime && tradable && validPos )
		{
			++gsprCnt;
			dv += price * shr;
			if( pred > 0. )
			{
				predGsprSum += (pred - sprd) * price * shr;
				//realGsprSum += (target - sprd) * price;
				realGsprSum += (closePrc - price - fee_bpt / basis_pts_ * price) * shr;
				pos += price * shr;
			}
			else if( pred < 0. )
			{
				predGsprSum += (-pred - sprd) * price * shr;
				//realGsprSum += (-target - sprd) * price;
				realGsprSum += (price - closePrc - fee_bpt / basis_pts_ * price) * shr;
				pos -= price * shr;
			}
			prevTradeMsecs = msecs;
		}
		prevMsecs = msecs;
	}
	GsprStat gsprStat;
	gsprStat.gsprRat = (float)gsprCnt / (float)nData * 100.;
	gsprStat.predGspr = (dv > 0) ? predGsprSum / dv : 0.;
	gsprStat.realGspr = (dv > 0) ? realGsprSum / dv : 0.;
	gsprStat.plPpt = realGsprSum / nData;
	return gsprStat;
}
// ---------------------------------------------------------------------
// Original functions.
// ---------------------------------------------------------------------

BasicStat calculateBasicStat(const vector<float>& vControl)
{
	int nData = vControl.size();
	BasicStat bStat;
	bStat.binAvg = accumulate(begin(vControl), end(vControl), 0.) / nData;
	bStat.binStdev = stdev(vControl);
	bStat.npt = nData;
	return bStat;
}

PredStat calculatePredStat(const vector<float>& vPred, const vector<float>& vTarget)
{
	int nData = vPred.size();

	PredStat pStat;
	pStat.mX = accumulate(begin(vPred), end(vPred), 0.) / nData;
	pStat.mY = accumulate(begin(vTarget), end(vTarget), 0.) / nData;

	double mX2 = inner_product(begin(vPred), end(vPred), begin(vPred), 0.) / nData;
	pStat.dX = sqrt(mX2 - pStat.mX * pStat.mX);

	double mY2 = inner_product(begin(vTarget), end(vTarget), begin(vTarget), 0.) / nData;
	pStat.dY = sqrt(mY2 - pStat.mY * pStat.mY);

	double mXY = inner_product(begin(vPred), end(vPred), begin(vTarget), 0.) / nData;
	pStat.corr = (pStat.dX > 0. && pStat.dY > 0.) ?
		(mXY - pStat.mX * pStat.mY) / pStat.dX / pStat.dY : 0.;

	pStat.lev = pStat.corr * pStat.dY;

	return pStat;
}

TopBottomStat calculateTopBottomStat(const vector<float>& vPred, const vector<float>& vTarget)
{
	int nData = vPred.size();
	TopBottomStat tbStat;

	vector<int> vSortedIndex(nData);
	gsl_heapsort_index(vSortedIndex, vPred);
	int n = nData * 0.01;

	double sumPredTop = 0.;
	double sumPredBottom = 0.;
	double sumTargetTop = 0.;
	double sumTargetBottom = 0.;
	for( int i = 0; i < n; ++i )
	{
		int iTop = vSortedIndex[nData - i - 1];
		int iBot = vSortedIndex[i];
		sumPredTop += vPred[iTop];
		sumPredBottom += vPred[iBot];
		sumTargetTop += vTarget[iTop];
		sumTargetBottom += vTarget[iBot];
	}
	tbStat.frcEv1p = (sumPredTop - sumPredBottom) / 2. / n;
	tbStat.ev1p = (sumTargetTop - sumTargetBottom) / 2. / n;

	return tbStat;
}

TradableStat calculateTradableStat(const vector<float>& vPred, const vector<float>& vTarget,
		const vector<float>& vSprdAdj, double thres)
{
	return calculateTradableStat(vPred, vPred, vTarget, vSprdAdj, thres);
}

TradableStat calculateTradableStat(const vector<float>& vPred, const vector<float>& vTotPred,
		const vector<float>& vTarget, const vector<float>& vSprdAdj, double thres)
{
	int nData = vPred.size();
	int cnt = 0;
	double sumPredBuy = 0.;
	double sumPredSell = 0.;
	double sumTargetBuy = 0.;
	double sumTargetSell = 0.;
	for( int i = 0; i < nData; ++i )
	{
		float sprd = vSprdAdj[i];
		float pred = vPred[i];
		float totPred = vTotPred[i];
		float target = vTarget[i];

		if( totPred > sprd + thres / basis_pts_ || -totPred > sprd + thres / basis_pts_ )
		{
			++cnt;
			if( totPred > 0. )
			{
				sumPredBuy += vPred[i];
				sumTargetBuy += vTarget[i];
			}
			else if( totPred < 0. )
			{
				sumPredSell += vPred[i];
				sumTargetSell += vTarget[i];
			}
		}
	}
	TradableStat trdStat;
	if( cnt > 0 )
	{
		trdStat.frcEvT = (sumPredBuy - sumPredSell) / cnt;
		trdStat.evT = (sumTargetBuy - sumTargetSell) / cnt;
		if( trdStat.evT != 0. )
		{
			int relOf = ceil((trdStat.frcEvT / trdStat.evT - 1.) * 100 - 0.5);

			int relOfClip = relOf;
			if( relOfClip > 49 )
				relOfClip = 49;
			else if( relOfClip < -51 )
				relOfClip = -51;

			int relOfAdj = relOfClip;
			if( relOfAdj < 0 )
				relOfAdj += 100;
			trdStat.relOf = relOfAdj;
		}
	}
	return trdStat;
}

TradableStat calculateTradableStat(const vector<float>& vPred, const vector<float>& vTotPred,
		const vector<float>& vTarget, const vector<float>& vSprdAdj, const vector<float>& vThres)
{
	int nData = vPred.size();
	int cnt = 0;
	double sumPredBuy = 0.;
	double sumPredSell = 0.;
	double sumTargetBuy = 0.;
	double sumTargetSell = 0.;
	for( int i = 0; i < nData; ++i )
	{
		float sprd = vSprdAdj[i];
		float pred = vPred[i];
		float totPred = vTotPred[i];
		float target = vTarget[i];
		float thres = vThres[i];

		if( totPred > sprd + thres / basis_pts_ || -totPred > sprd + thres / basis_pts_ )
		{
			++cnt;
			if( totPred > 0. )
			{
				sumPredBuy += vPred[i];
				sumTargetBuy += vTarget[i];
			}
			else if( totPred < 0. )
			{
				sumPredSell += vPred[i];
				sumTargetSell += vTarget[i];
			}
		}
	}
	TradableStat trdStat;
	if( cnt > 0 )
	{
		trdStat.frcEvT = (sumPredBuy - sumPredSell) / cnt;
		trdStat.evT = (sumTargetBuy - sumTargetSell) / cnt;
		if( trdStat.evT != 0. )
		{
			int relOf = ceil((trdStat.frcEvT / trdStat.evT - 1.) * 100 - 0.5);

			int relOfClip = relOf;
			if( relOfClip > 49 )
				relOfClip = 49;
			else if( relOfClip < -51 )
				relOfClip = -51;

			int relOfAdj = relOfClip;
			if( relOfAdj < 0 )
				relOfAdj += 100;
			trdStat.relOf = relOfAdj;
		}
	}
	return trdStat;
}

GsprStat calculateGsprStat(const vector<float>& vPred, const vector<float>& vTarget, const vector<float>& vSprdAdj, double thres, bool debug)
{
	int nData = vPred.size();
	vector<float> vPrice(nData, 1.);
	return calculateGsprStat(vPred, vTarget, vSprdAdj, vPrice, vPrice, thres, debug);
}

GsprStat calculateGsprStat(const vector<float>& vPred, const vector<float>& vTarget, const vector<float>& vSprdAdj, const vector<float>& vThres, bool debug)
{
	int nData = vPred.size();
	vector<float> vPrice(nData, 1.);
	return calculateGsprStat(vPred, vTarget, vSprdAdj, vPrice, vPrice, vThres, debug);
}

GsprStat calculateGsprStat(const vector<float>& vPred, const vector<float>& vTarget, const vector<float>& vSprdAdj,
		const vector<float>& vBid, const vector<float>& vAsk, double thres, bool debug)
{
	int nData = vPred.size();
	int gsprCnt = 0;
	float dv = 0.;
	float predGsprSum = 0.;
	float realGsprSum = 0.;
	float debugSum = 0.;
	for( int i = 0; i < nData; ++i )
	{
		float sprd = vSprdAdj[i];
		float pred = vPred[i];
		float target = vTarget[i];
		float price = pred > 0. ? vAsk[i] : vBid[i];
		if( pred > sprd + thres / basis_pts_ || -pred > sprd + thres / basis_pts_ )
		{
			if(debug)
			{
				printf("FFF %.4f %.4f %.4f %d %.2f\n", pred*basis_pts_, target*basis_pts_, sprd*basis_pts_, i, price);
				debugSum += (pred>0)?(target-sprd):(-target-sprd);
			}
			gsprCnt += price;
			dv += price;
			if( pred > 0. )
			{
				predGsprSum += (pred - sprd) * price;
				realGsprSum += (target - sprd) * price;
			}
			else if( pred < 0. )
			{
				predGsprSum += (-pred - sprd) * price;
				realGsprSum += (-target - sprd) * price;
			}
		}
	}
	if(debug)
		printf("JJJ nD %d dv %.1f realGsprSum %.4f debugSum %.4f\n", nData, dv, realGsprSum, debugSum);
	GsprStat gsprStat;
	gsprStat.gsprRat = (float)gsprCnt / (float)nData * 100.;
	gsprStat.predGspr = (dv > 0) ? predGsprSum / dv : 0.;
	gsprStat.realGspr = (dv > 0) ? realGsprSum / dv : 0.;
	gsprStat.plPpt = realGsprSum / nData;
	return gsprStat;
}

GsprStat calculateGsprStat(const vector<float>& vPred, const vector<float>& vTarget, const vector<float>& vSprdAdj,
		const vector<float>& vBid, const vector<float>& vAsk, const vector<float>& vThres, bool debug)
{
	int nData = vPred.size();
	int gsprCnt = 0;
	float dv = 0.;
	float predGsprSum = 0.;
	float realGsprSum = 0.;
	float debugSum = 0.;
	for( int i = 0; i < nData; ++i )
	{
		float sprd = vSprdAdj[i];
		float pred = vPred[i];
		float target = vTarget[i];
		float price = pred > 0. ? vAsk[i] : vBid[i];
		float thres = vThres[i];
		if( pred > sprd + thres / basis_pts_ || -pred > sprd + thres / basis_pts_ )
		{
			if(debug)
			{
				printf("FFF %.4f %.4f %.4f %d %.2f\n", pred*basis_pts_, target*basis_pts_, sprd*basis_pts_, i, price);
				debugSum += (pred>0)?(target-sprd):(-target-sprd);
			}
			gsprCnt += price;
			dv += price;
			if( pred > 0. )
			{
				predGsprSum += (pred - sprd) * price;
				realGsprSum += (target - sprd) * price;
			}
			else if( pred < 0. )
			{
				predGsprSum += (-pred - sprd) * price;
				realGsprSum += (-target - sprd) * price;
			}
		}
	}
	if(debug)
		printf("JJJ nD %d dv %.1f realGsprSum %.4f debugSum %.4f\n", nData, dv, realGsprSum, debugSum);
	GsprStat gsprStat;
	gsprStat.gsprRat = (float)gsprCnt / (float)nData * 100.;
	gsprStat.predGspr = (dv > 0) ? predGsprSum / dv : 0.;
	gsprStat.realGspr = (dv > 0) ? realGsprSum / dv : 0.;
	gsprStat.plPpt = realGsprSum / nData;
	return gsprStat;
}

// ---------------------------------------------------------------------
// gsprPrintSimple
// ---------------------------------------------------------------------

void gsprPrintSimple(ostream& os, const BasicStat& bStat, const PredStat& pStat,
		const TopBottomStat& tbStat, const TradableStat& trdStat,
		const GsprStat& gStat)
{
	if( bStat.binStdev < bStat.binAvg * 1e-3 ) // NTree
	{
		if( os.tellp() == 0 )
			printHeaderNTree(os);
		printContentNTree(os, bStat, pStat, tbStat, trdStat, gStat);
	}
	else // Quantile
	{
		if( os.tellp() == 0 )
			printHeaderQuantile(os);
		printContentQuantile(os, bStat, pStat, tbStat, trdStat, gStat);
	}
}

// ---------------------------------------------------------------------
// gsprPrintExtend
// ---------------------------------------------------------------------

void gsprPrintExtend(ostream& os, const BasicStat& bStat, const PredStat& pStat,
		const TopBottomStat& tbStat, const TradableStat& trdStat,
		const GsprStat& gStat, const GsprStat& gStatIntra, const GsprStat& gStatPrice,
		const GsprStat& gStatIntraPrice, const GsprStat& gStatExactIntra)
{
	if( os.tellp() == 0 )
		printHeaderExtendQuantile(os);
	printContentExtendQuantile(os, bStat, pStat, tbStat, trdStat, gStat, gStatIntra, gStatPrice, gStatIntraPrice, gStatExactIntra);
}

void printHeaderExtendQuantile(ostream& os)
{
	char buf[1000];
	sprintf(buf, "%7s %s "
			//"%9s %9s %9s %9s %6s %7s "
			"%6s %6s "
			"%6s %6s "
			"%7s %6s %6s %9s "
			"%9s %9s %9s %6s %9s\n",
			"binAvg", "npt",
			//"mX", "mY", "dX", "dY", "corr", "lev",
			"ev1p", "of1p",
			"evT", "ofT",
			"gsprRat", "mev", "mBias", "plPpt",
			"plPptIntra", "plPptPrice", "plPptIntraPrice", "mevEx", "plPptExactIntra");
	os << buf;
}

void printContentExtendQuantile(ostream& os, const BasicStat& bStat, const PredStat& pStat,
		const TopBottomStat& tbStat, const TradableStat& trdStat, const GsprStat& gStat,
		const GsprStat& gStatIntra, const GsprStat& gStatPrice, const GsprStat& gStatIntraPrice,
		const GsprStat& gStatExactIntra)
{
	char buf[1000];
	sprintf(buf, "%7.5f %d "
			//"%9.2e %9.2e %9.2e %9.2e %6.4f %7.5f "
			"%6.2f %6.2f "
			"%6.2f %6.2f "
			"%7.4f %6.2f %6.2f %9.2e "
			"%9.2e %9.2e %9.2e %6.2f %9.2e\n",
			bStat.binAvg, bStat.npt,
			//pStat.mX, pStat.mY, pStat.dX, pStat.dY, pStat.corr, pStat.lev,
			tbStat.ev1p * basis_pts_, (tbStat.frcEv1p - tbStat.ev1p) * basis_pts_,
			trdStat.evT * basis_pts_, (trdStat.frcEvT - trdStat.evT) * basis_pts_,
			gStat.gsprRat, gStat.realGspr * basis_pts_, (gStat.predGspr - gStat.realGspr) * basis_pts_, gStat.plPpt,
			//gStatIntra.plPpt, gStatIntraPrice.realGspr * basis_pts_, gStatIntraPrice.plPpt);
			gStatIntra.plPpt, gStatPrice.plPpt, gStatIntraPrice.plPpt, gStatExactIntra.realGspr*basis_pts_, gStatExactIntra.plPpt);
	os << buf;
}

void printHeaderQuantile(ostream& os)
{
	char buf[1000];
	sprintf(buf, "%7s %s "
			//"%9s %9s %9s %9s %6s %7s "
			"%6s %6s "
			"%6s %6s "
			"%7s %6s %6s %9s\n",
			"binAvg", "npt",
			//"mX", "mY", "dX", "dY", "corr", "lev",
			"ev1p", "of1p",
			"evT", "ofT",
			"gsprRat", "mev", "mBias", "plPpt");
	os << buf;
}

void printContentQuantile(ostream& os, const BasicStat& bStat, const PredStat& pStat,
		const TopBottomStat& tbStat, const TradableStat& trdStat, const GsprStat& gStat)
{
	char buf[1000];
	sprintf(buf, "%7.5f %d "
			//"%9.2e %9.2e %9.2e %9.2e %6.4f %7.5f "
			"%6.2f %6.2f "
			"%6.2f %6.2f "
			"%7.4f %6.2f %6.2f %9.2e\n",
			bStat.binAvg, bStat.npt,
			//pStat.mX, pStat.mY, pStat.dX, pStat.dY, pStat.corr, pStat.lev,
			tbStat.ev1p * basis_pts_, (tbStat.frcEv1p - tbStat.ev1p) * basis_pts_,
			trdStat.evT * basis_pts_, (trdStat.frcEvT - trdStat.evT) * basis_pts_,
			gStat.gsprRat, gStat.realGspr * basis_pts_, (gStat.predGspr - gStat.realGspr) * basis_pts_, gStat.plPpt);
	os << buf;
}

void printHeaderNTree(ostream& os)
{
	char buf[1000];
	sprintf(buf, "%7s %s "
			//"%9s %9s %9s %9s %6s %7s "
			"%6s %6s "
			"%6s %6s "
			"%7s %6s %6s %9s\n",
			"ntree", "npt",
			//"mX", "mY", "dX", "dY", "corr", "lev",
			"ev1p", "of1p",
			"evT", "ofT",
			"gsprRat", "mev", "mBias", "plPpt");
	os << buf;
}

void printContentNTree(ostream& os, const BasicStat& bStat, const PredStat& pStat,
		const TopBottomStat& tbStat, const TradableStat& trdStat, const GsprStat& gStat)
{
	char buf[1000];
	sprintf(buf, "%7d %d "
			//"%9.2e %9.2e %9.2e %9.2e %6.4f %7.5f "
			"%6.2f %6.2f "
			"%6.2f %6.2f "
			"%7.4f %6.2f %6.2f %9.2e\n",
			static_cast<int>(bStat.binAvg), bStat.npt,
			//pStat.mX, pStat.mY, pStat.dX, pStat.dY, pStat.corr, pStat.lev,
			tbStat.ev1p * basis_pts_, (tbStat.frcEv1p - tbStat.ev1p) * basis_pts_,
			trdStat.evT * basis_pts_, (trdStat.frcEvT - trdStat.evT) * basis_pts_,
			gStat.gsprRat, gStat.realGspr * basis_pts_, (gStat.predGspr - gStat.realGspr) * basis_pts_, gStat.plPpt);
	os << buf;
}

// ---------------------------------------------------------------------
// gsprPrintSimple
// ---------------------------------------------------------------------

void gsprPrintSimpleThres(ostream& os, float thres, const BasicStat& bStat,
		const TradableStat& trdStat, const GsprStat& gStat)
{
	if( os.tellp() == 0 )
		printHeaderQuantileSimpleThres(os);
	printContentQuantileSimpleThres(os, thres, bStat, trdStat, gStat);
}

void printHeaderQuantileSimpleThres(ostream& os)
{
	char buf[1000];
	sprintf(buf, "%7s %5s "
			"%6s %6s "
			"%7s %6s %6s %9s\n",
			"binAvg", "thres",
			"evT", "ofT",
			"gsprRat", "mev", "mBias", "plPpt");
	os << buf;
}

void printContentQuantileSimpleThres(ostream& os, float thres, const BasicStat& bStat,
		const TradableStat& trdStat, const GsprStat& gStat)
{
	char buf[1000];
	sprintf(buf, "%7.5f %5.1f "
			"%6.2f %6.2f "
			"%7.4f %6.2f %6.2f %9.2e\n",
			bStat.binAvg, thres,
			trdStat.evT * basis_pts_, (trdStat.frcEvT - trdStat.evT) * basis_pts_,
			gStat.gsprRat, gStat.realGspr * basis_pts_, (gStat.predGspr - gStat.realGspr) * basis_pts_, gStat.plPpt);
	os << buf;
}

// ---------------------------------------------------------------------
// gsprPrintSimpleThres
// ---------------------------------------------------------------------

void gsprPrintThres(ostream& os, float thres, const BasicStat& bStat,
		const TradableStat& trdStat, const GsprStat& gStat,
		const GsprStat& gStatIntra, const GsprStat& gStatPrice,
		const GsprStat& gStatIntraPrice)
{
	if( os.tellp() == 0 )
		printHeaderQuantileThres(os);
	printContentQuantileThres(os, thres, bStat, trdStat, gStat, gStatIntra, gStatPrice, gStatIntraPrice);
}

void printHeaderQuantileThres(ostream& os)
{
	char buf[1000];
	sprintf(buf, "%7s %5s "
			"%6s %6s "
			"%7s %6s %6s %9s "
			"%9s %9s %9s\n",
			"binAvg", "thres",
			"evT", "ofT",
			"gsprRat", "mev", "mBias", "plPpt",
			"plPptIntra", "plPptPrice", "plPptIntraPrice");
	os << buf;
}

void printContentQuantileThres(ostream& os, float thres, const BasicStat& bStat,
		const TradableStat& trdStat, const GsprStat& gStat,
		const GsprStat& gStatIntra, const GsprStat& gStatPrice,
		const GsprStat& gStatIntraPrice)
{
	char buf[1000];
	sprintf(buf, "%7.5f %5.1f "
			"%6.2f %6.2f "
			"%7.4f %6.2f %6.2f %9.2e "
			"%9.2e %9.2e %9.2e\n",
			bStat.binAvg, thres,
			trdStat.evT * basis_pts_, (trdStat.frcEvT - trdStat.evT) * basis_pts_,
			gStat.gsprRat, gStat.realGspr * basis_pts_, (gStat.predGspr - gStat.realGspr) * basis_pts_, gStat.plPpt,
			gStatIntra.plPpt, gStatPrice.plPpt, gStatIntraPrice.plPpt);
	os << buf;
}

// ---------------------------------------------------------------------
// gsprPrintMultTar
// ---------------------------------------------------------------------

void gsprPrintMultTar(ostream& os, const BasicStat& bStat,
		const PredStat& pStatTot, const PredStat& pStatOm, const PredStat& pStatTm,
		const TopBottomStat& tbStatTot,
		const TradableStat& trdStatTot, const TradableStat& trdStatOm, const TradableStat& trdStatTm,
		const GsprStat& gStat, const GsprStat& gStatIntra, const GsprStat& gStatPrice)
{
	if( bStat.binStdev < bStat.binAvg * 1e-3 ) // NTree
	{
		if( os.tellp() == 0 )
			printHeaderMultTarNTree(os);
		printContentMultTarNTree(os, bStat, tbStatTot, pStatTot, pStatOm, pStatTm, trdStatTot, trdStatOm, trdStatTm, gStat, gStatIntra, gStatPrice);
	}
	else // Quantile
	{
		if( os.tellp() == 0 )
			printHeaderMultTarQuantile(os);
		printContentMultTarQuantile(os, bStat, tbStatTot, pStatTot, pStatOm, pStatTm, trdStatTot, trdStatOm, trdStatTm, gStat, gStatIntra, gStatPrice);
	}
}

void printHeaderMultTarQuantile(ostream& os)
{
	char buf[1000];
	sprintf(buf, "%7s %s "
			"%6s %6s %6s "
			"%6s %6s "
			"%6s %6s %6s %6s %6s %6s "
			"%7s %6s %6s %9s "
			"%9s %6s %9s\n",
			"binAvg", "npt",
			"corrTot", "corrOm", "corrTm",
			"ev1p", "of1p",
			"evTot", "ofTot", "evOm", "ofOm", "evTm", "ofTm",
			"gsprRat", "mev", "mBias", "plPpt",
			"plPptIntra", "gpt", "plPptIntraPrice");
	os << buf;
}

void printContentMultTarQuantile(ostream& os, const BasicStat& bStat,
		const TopBottomStat& tbStatTot,
		const PredStat& pStatTot, const PredStat& pStatOm, const PredStat& pStatTm,
		const TradableStat& trdStatTot, const TradableStat& trdStatOm, const TradableStat& trdStatTm,
		const GsprStat& gStat, const GsprStat& gStatIntra, const GsprStat& gStatIntraPrice)
{
	char buf[1000];
	sprintf(buf, "%7.5f %d "
			"%6.4f %6.4f %6.4f "
			"%6.2f %6.2f %6.2f %6.2f %6.2f %6.2f "
			"%7.4f %6.2f %6.2f %9.2e "
			"%9.2e %6.2f %9.2e\n",
			bStat.binAvg, bStat.npt,
			pStatTot.corr, pStatOm.corr, pStatTm.corr,
			tbStatTot.ev1p * basis_pts_, (tbStatTot.frcEv1p - tbStatTot.ev1p) * basis_pts_,
			trdStatTot.evT * basis_pts_, (trdStatTot.frcEvT - trdStatTot.evT) * basis_pts_,
			trdStatOm.evT * basis_pts_, (trdStatOm.frcEvT - trdStatOm.evT) * basis_pts_,
			trdStatTm.evT * basis_pts_, (trdStatTm.frcEvT - trdStatTm.evT) * basis_pts_,
			gStat.gsprRat, gStat.realGspr * basis_pts_, (gStat.predGspr - gStat.realGspr) * basis_pts_, gStat.plPpt,
			gStatIntra.plPpt, gStatIntraPrice.realGspr * basis_pts_, gStatIntraPrice.plPpt);
	os << buf;
}

void printHeaderMultTarNTree(ostream& os)
{
	char buf[1000];
	sprintf(buf, "%7s %s "
			"%6s %6s %6s "
			"%6s %6s "
			"%6s %6s %6s %6s %6s %6s "
			"%7s %6s %6s %9s "
			"%9s "
			"%6s %9s\n",
			"ntree", "npt",
			"corrTot", "corrOm", "corrTm",
			"ev1p", "of1p",
			"evTot", "ofTot", "evOm", "ofOm", "evTm", "ofTm",
			"gsprRat", "mev", "mBias", "plPpt",
			"plPptIntra",
			"gpt", "plPptIntraPrice");
	os << buf;
}

void printContentMultTarNTree(ostream& os, const BasicStat& bStat, const TopBottomStat& tbStatTot,
		const PredStat& pStatTot, const PredStat& pStatOm, const PredStat& pStatTm,
		const TradableStat& trdStatTot, const TradableStat& trdStatOm, const TradableStat& trdStatTm,
		const GsprStat& gStat, const GsprStat& gStatIntra, const GsprStat& gStatIntraPrice)
{
	char buf[1000];
	sprintf(buf, "%7d %d "
			"%6.4f %6.4f %6.4f "
			"%6.2f %6.2f "
			"%6.2f %6.2f %6.2f %6.2f %6.2f %6.2f "
			"%7.4f %6.2f %6.2f %9.2e "
			"%9.2e "
			"%6.2f %9.2e\n",
			static_cast<int>(bStat.binAvg), bStat.npt,
			pStatTot.corr, pStatOm.corr, pStatTm.corr,
			tbStatTot.ev1p * basis_pts_, (tbStatTot.frcEv1p - tbStatTot.ev1p) * basis_pts_,
			trdStatTot.evT * basis_pts_, (trdStatTot.frcEvT - trdStatTot.evT) * basis_pts_,
			trdStatOm.evT * basis_pts_, (trdStatOm.frcEvT - trdStatOm.evT) * basis_pts_,
			trdStatTm.evT * basis_pts_, (trdStatTm.frcEvT - trdStatTm.evT) * basis_pts_,
			gStat.gsprRat, gStat.realGspr * basis_pts_, (gStat.predGspr - gStat.realGspr) * basis_pts_, gStat.plPpt,
			gStatIntra.plPpt,
			gStatIntraPrice.realGspr * basis_pts_, gStatIntraPrice.plPpt);
	os << buf;
}

// ---------------------------------------------------------------------
// gsprPrintMultTarThres
// ---------------------------------------------------------------------

void gsprPrintMultTarThres(ostream& os, double thres, const BasicStat& bStat,
		const TradableStat& trdStatTot, const TradableStat& trdStatOm, const TradableStat& trdStatTm,
		const GsprStat& gStat, const GsprStat& gStatIntra, const GsprStat& gStatPrice)
{
	if( os.tellp() == 0 )
		printHeaderMultTarNTreeThres(os);
	printContentMultTarNTreeThres(os, thres, bStat, trdStatTot, trdStatOm, trdStatTm, gStat, gStatIntra, gStatPrice);
}

void printHeaderMultTarNTreeThres(ostream& os)
{
	char buf[1000];
	sprintf(buf, "%7s %5s "
			"%6s %6s %6s %6s %6s %6s "
			"%7s "
			"%6s %6s %9s "
			"%6s %6s %9s "
			"%6s %6s %9s\n",
			"ntree", "thres",
			"evTot", "ofTot", "evOm", "ofOm", "evTm", "ofTm",
			"gsprRat",
			"mev", "mBias", "plPpt",
			"mevIntra", "mBiasIntra", "plPptIntra",
			"gpt", "gptBias", "plPptIntraPrice");
	os << buf;
}

void printContentMultTarNTreeThres(ostream& os, double thres, const BasicStat& bStat,
		const TradableStat& trdStatTot, const TradableStat& trdStatOm, const TradableStat& trdStatTm,
		const GsprStat& gStat, const GsprStat& gStatIntra, const GsprStat& gStatIntraPrice)
{
	char buf[1000];
	sprintf(buf, "%7d %5.1f "
			"%6.2f %6.2f %6.2f %6.2f %6.2f %6.2f "
			"%7.4f "
			"%6.2f %6.2f %9.2e "
			"%6.2f %6.2f %9.2e "
			"%6.2f %6.2f %9.2e\n",
			static_cast<int>(bStat.binAvg), thres,
			trdStatTot.evT * basis_pts_, (trdStatTot.frcEvT - trdStatTot.evT) * basis_pts_,
			trdStatOm.evT * basis_pts_, (trdStatOm.frcEvT - trdStatOm.evT) * basis_pts_,
			trdStatTm.evT * basis_pts_, (trdStatTm.frcEvT - trdStatTm.evT) * basis_pts_,
			gStat.gsprRat,
			gStat.realGspr * basis_pts_, (gStat.predGspr - gStat.realGspr) * basis_pts_, gStat.plPpt,
			gStatIntra.realGspr * basis_pts_, (gStatIntra.predGspr - gStatIntra.realGspr) * basis_pts_, gStatIntra.plPpt,
			gStatIntraPrice.realGspr * basis_pts_, (gStatIntraPrice.predGspr - gStatIntraPrice.realGspr) * basis_pts_, gStatIntraPrice.plPpt);
	os << buf;
}

} // namespace gtlib
