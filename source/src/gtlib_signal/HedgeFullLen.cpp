#include <gtlib_signal/HedgeFullLen.h>
#include <gtlib_model/hff.h>
#include <gtlib_model/mFtns.h>
#include <jl_lib/jlutil.h>
using namespace std;

namespace gtlib {

const TargetHedger* HedgeFullLen::getTargetHedger(const string& ticker)
{
	return &targetHedger_;
}

void HedgeFullLen::calculateHedge(int n1sec)
{
	targetHedger_ = TargetHedgerFullLen(nTarget_, n1sec);

	// Get hedgeN.
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
		for( int k = 0; k < n1sec - 1; ++k )
		{
			// Hedge targets.
			for( int iT = 0; iT < nTarget_; ++iT )
			{
				// Get mean.
				double sum = 0.;
				int n = 0;
				for( int i = 0; i < hedgeN; ++i )
				{
					if( vSigH_[vIndx[i]].sIH[k].gptOK == 1 )
					{
						sum += vSigH_[vIndx[i]].sIH[k].target[iT];
						++n;
					}
				}

				if( n > 0 )
				{
					// Hedge.
					double mean = sum / n;

					for( int i = 0; i < hedgeN; ++i )
						vSigH_[vIndx[i]].sIH[k].target[iT] -= mean;

					for( int i = 0; i < hedgeN; ++i )
						targetHedger_.setMarketRet(iT, k, mean);
				}
			}

			// Target from 11 to close.
			{
				// Get mean.
				double sum = 0.;
				int n = 0;
				for( int i = 0; i < hedgeN; ++i )
				{
					if( vSigH_[vIndx[i]].sIH[k].gptOK == 1 )
					{
						sum += vSigH_[vIndx[i]].sIH[k].target11Close;
						++n;
					}
				}

				if( n > 0 )
				{
					// Hedge.
					double mean = sum / n;
					for( int i = 0; i < hedgeN; ++i )
						vSigH_[vIndx[i]].sIH[k].target11Close -= mean;
					targetHedger_.setMarketRet11Close(k, mean);
				}
			}

			// Target from 71 to close.
			{
				// Get mean.
				double sum = 0.;
				int n = 0;
				for( int i = 0; i < hedgeN; ++i )
				{
					if( vSigH_[vIndx[i]].sIH[k].gptOK == 1 )
					{
						sum += vSigH_[vIndx[i]].sIH[k].target71Close;
						++n;
					}
				}

				if( n > 0 )
				{
					// Hedge.
					double mean = sum / n;
					for( int i = 0; i < hedgeN; ++i )
						vSigH_[vIndx[i]].sIH[k].target71Close -= mean;
					targetHedger_.setMarketRet71Close(k, mean);
				}
			}

			// Target to close.
			{
				// Get mean.
				double sum = 0.;
				int n = 0;
				for( int i = 0; i < hedgeN; ++i )
				{
					if( vSigH_[vIndx[i]].sIH[k].gptOK == 1 )
					{
						sum += vSigH_[vIndx[i]].sIH[k].targetClose;
						++n;
					}
				}

				if( n > 0 )
				{
					// Hedge.
					double mean = sum / n;
					for( int i = 0; i < hedgeN; ++i )
						vSigH_[vIndx[i]].sIH[k].targetClose -= mean;
					targetHedger_.setMarketRetClose(k, mean);
				}
			}
		}

		// TarON.
		{
			// Get mean.
			double sum = 0.;
			int n = 0;
			for( int i = 0; i < hedgeN; ++i )
			{
				sum += vSigH_[vIndx[i]].tarON;
				++n;
			}

			if( n > 0 )
			{
				// Hedge.
				double mean = sum / n;
				for( int i = 0; i < hedgeN; ++i )
					vSigH_[vIndx[i]].tarON -= mean;
				targetHedger_.setMarketRetON(mean);
			}
		}

		// TarClcl.
		{
			// Get mean.
			double sum = 0.;
			int n = 0;
			for( int i = 0; i < hedgeN; ++i )
			{
				sum += vSigH_[vIndx[i]].tarClcl;
				++n;
			}

			if( n > 0 )
			{
				// Hedge.
				double mean = sum / n;
				for( int i = 0; i < hedgeN; ++i )
					vSigH_[vIndx[i]].tarClcl -= mean;
				targetHedger_.setMarketRetClcl(mean);
			}
		}
	}
	else
	{
		cerr << "HedgeFullLen::calculateHedge() ERROR: no ticker to hedge." << endl;
	}
}

} // namespace gtlib
