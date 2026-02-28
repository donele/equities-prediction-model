#include <gtlib_signal/HedgeNFadj.h>
#include <algorithm>
using namespace std;

namespace gtlib {

HedgeNFadj::HedgeNFadj(int idate)
	:idate_(idate)
{
}

const TargetHedger* HedgeNFadj::getTargetHedger(const string& ticker)
{
	auto it = mTargetHedger_.find(ticker);
	if( it != mTargetHedger_.end() )
		return &(it->second);
	return nullptr;
}

void HedgeNFadj::calculateHedge(int n1sec)
{
	sort(vSigH_.begin(), vSigH_.end(), hff::SigHLess());
	NorthfieldHedge nfh = NorthfieldHedge(idate_, "equityData");

	vector<int> vIndx;
	int totN = vSigH_.size();
	for( int i = 0; i < totN; ++i )
	{
		if( vSigH_[i].inUnivFit == 1 )
		{
			nfh.AddToUniverse(vSigH_[i].ticker);
			vIndx.push_back(i);
		}
	}
	int hedgeN = vIndx.size();

	nfh.CalcHedgeMatrices(2, true);
	getNFSignals(nfh, n1sec);

	vector<double> signals(hedgeN);
	vector<double> hedgedSignals(hedgeN);

	// Hedge 1 second targets.
	for( int k = 0; k < n1sec - 1; ++k )
	{
		for( int i = 0; i < hedgeN; ++i )
		{
			int indx = vIndx[i];
			signals[i] = vSigH_[indx].sIH[k].target[0];
		}

		nfh.CalcHedgeAdjustment(&signals[0], &hedgedSignals[0]);

		// Old way.
		for( int i = 0; i < hedgeN; ++i )
			vSigH_[vIndx[i]].sIH[k].target[0] -= hedgedSignals[i];

		// New way.
		for( int i = 0; i < hedgeN; ++i )
		{
			const string& ticker = vSigH_[vIndx[i]].ticker;
			auto it = mTargetHedger_.find(ticker);
			if( it == mTargetHedger_.end() )
			{
				mTargetHedger_[ticker] = TargetHedgerNFadj(n1sec);
				it = mTargetHedger_.find(ticker);
			}
			it->second.setHedgedTarget(k, hedgedSignals[i]);
		}
	}
}

void HedgeNFadj::getNFSignals(NorthfieldHedge& nfh, int n1sec)
{
	vector<int> missingIndx;
	double sumnorthbp = 0.;
	double sumnorthtrd = 0.;
	double sumnorthrst = 0.;
	int nok = 0;
	int totN = vSigH_.size();
	for( int i = 0; i < totN; ++i )
	{
		hff::SigH& sigh = vSigH_[i];

		NFStockParams nfParams;
		if( nfh.GetParams(sigh.ticker, &nfParams) && nfParams.sector >= 0 )
		{
			double northBP = nfParams.northFactors[2];
			double northTRD = nfParams.northFactors[4];
			double northRST = nfParams.northFactors[5];
			sigh.northBP = northBP;
			sigh.northTRD = northTRD;
			sigh.northRST = northRST;

			mTargetHedger_[sigh.ticker] = TargetHedgerNFadj(n1sec, northBP, northTRD, northRST);

			sumnorthbp += nfParams.northFactors[2];
			sumnorthtrd += nfParams.northFactors[4];
			sumnorthrst += nfParams.northFactors[5];
			++nok;
		}
		else
			missingIndx.push_back(i);
	}

	double northbp = 0.;
	double northtrd = 0.;
	double northrst = 0.;
	if( nok > 0 )
	{
		northbp = sumnorthbp / nok;
		northtrd = sumnorthtrd / nok;
		northrst = sumnorthrst / nok;
	}

	int missingN = missingIndx.size();
	for( int i = 0; i < missingN; ++i )
	{
		int indx = missingIndx[i];
		hff::SigH& sigh = vSigH_[indx];
		{
			sigh.northBP = northbp;
			sigh.northTRD = northtrd;
			sigh.northRST = northrst;
			mTargetHedger_[sigh.ticker] = TargetHedgerNFadj(n1sec, northbp, northtrd, northrst);
		}
	}
}

} // namespace gtlib
