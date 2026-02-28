#include <gtlib_signal/HedgeMarket.h>
#include <vector>
using namespace std;

namespace gtlib {

const TargetHedger* HedgeMarket::getTargetHedger(const string& ticker)
{
	return &targetHedger_;
}

void HedgeMarket::calculateHedge(int n1sec)
{
	targetHedger_ = TargetHedgerMarket(n1sec);

	// Number of tickers used for hedging.
	vector<int> vIndx;
	int totN = vSigH_.size();
	for( int i = 0; i < totN; ++i )
	{
		if( vSigH_[i].inUnivFit == 1 )
			vIndx.push_back(i);
	}
	int hedgeN = vIndx.size();

	if( hedgeN > 0 )
	{
		// k = 0: target from t = 0 sec to t = 1 sec.
		for( int k = 0; k < n1sec - 1; ++k )
		{
			// Get mean.
			double sum = 0.;
			int n = 0;
			for( int i = 0; i < hedgeN; ++i )
			{
				if( vSigH_[vIndx[i]].sIH[k].gptOK == 1 )
				{
					sum += vSigH_[vIndx[i]].sIH[k].target[0];
					++n;
				}
			}

			if( n > 0 )
			{
				// Hedge.
				double mean = sum / n;

				// Old way.
				for( int i = 0; i < hedgeN; ++i )
					vSigH_[vIndx[i]].sIH[k].target[0] -= mean;

				// New way.
				for( int i = 0; i < hedgeN; ++i )
					targetHedger_.setMarketRet(k, mean);
			}
		}
	}
}

} // namespace gtlib
