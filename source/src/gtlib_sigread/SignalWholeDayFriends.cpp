#include <gtlib_sigread/SignalWholeDay.h>
#include <vector>
#include <string>
using namespace std;
using namespace std::placeholders;

namespace gtlib {

void printVar(const SignalWholeDay& lhs, const SignalWholeDay& rhs, int varIndex)
{
	char filename[100];
	sprintf(filename, "varComp_%d.txt", varIndex);
	ofstream ofs(filename);

	char buf[100];
	sprintf(buf, "%6s %8s %13s %13s %13s %13s\n", "ticker", "msecs", "val1", "val2", "pred1", "pred2");
	ofs << buf;
	for( auto& kv : lhs.mmInput_ )
	{
		const string& ticker = kv.first;
		if( rhs.exist(ticker) )
		{
			for( auto& kv2 : kv.second )
			{
				int msecs = kv2.first;
				if( rhs.exist(ticker, msecs) )
				{
					float val1 = kv2.second[varIndex];
					float val2 = rhs.getInputs(ticker, msecs)[varIndex];
					float pred1 = lhs.getPred(ticker, msecs);
					float pred2 = rhs.getPred(ticker, msecs);
					sprintf(buf, "%6s %6d %13.8f %13.8f %13.4f %13.4f\n", ticker.c_str(), msecs, val1, val2, pred1, pred2);
					ofs << buf;
				}
			}
		}
	}
}

void printBmpred(const SignalWholeDay& lhs, const SignalWholeDay& rhs)
{
	auto fun = std::bind(&SignalWholeDay::getBmpred, rhs, _1, _2);
	string filename = "comp_bmpred.txt";
	printPred(rhs, lhs.mmBmpred_, fun, filename);
}

void printPred(const SignalWholeDay& lhs, const SignalWholeDay& rhs)
{
	auto fun = std::bind(&SignalWholeDay::getPred, rhs, _1, _2);
	string filename = "comp_pred.txt";
	printPred(rhs, lhs.mmPred_, fun, filename);
}

void printPred(const SignalWholeDay& rhs, const MapofMapofFloat& mmPred,
		std::function<float(string, int)> fun, const string& filename)
{
	ofstream ofs(filename.c_str());
	char buf[100];
	sprintf(buf, "%6s %8s %13s %13s\n", "ticker", "msecs", "pred1", "pred2");
	ofs << buf;
	for( auto& kv : mmPred )
	{
		const string& ticker = kv.first;
		if( rhs.exist(ticker) )
		{
			for( auto& kv2 : kv.second )
			{
				const int& msecs = kv2.first;
				if( rhs.exist(ticker, msecs) )
				{
					float pred1 = kv2.second;
					float pred2 = fun(ticker, msecs);
					sprintf(buf, "%6s %6d %13.4f %13.4f\n", ticker.c_str(), msecs, pred1, pred2);
					ofs << buf;
				}
			}
		}
	}
}

vector<Corr> compInput(const SignalWholeDay& lhs, const SignalWholeDay& rhs)
{
	int nInputs = lhs.getNInputs();
	vector<Corr> vCorr(nInputs);
	for( auto& kv : lhs.mmInput_ )
	{
		const string& ticker = kv.first;
		if( rhs.exist(ticker) )
		{
			for( auto& kv2 : kv.second )
			{
				const int& msecs = kv2.first;

				if( rhs.exist(ticker, msecs) )
				{
					const vector<float>& inputs1 = kv2.second;
					const vector<float>& inputs2 = rhs.getInputs(ticker, msecs);
					if( inputs1.size() == nInputs && !inputs2.empty() && inputs2.size() == nInputs )
					{
						for( int i = 0; i < nInputs; ++i )
							vCorr[i].add(inputs1[i], inputs2[i]);
					}
				}
			}
		}
	}
	return vCorr;
}

void compTarget(const SignalWholeDay& lhs, const SignalWholeDay& rhs, Corr& corrTarget)
{
	for( const auto& kv : lhs.mmPred_ )
	{
		const string& ticker = kv.first;
		if( rhs.exist(ticker) )
		{
			for( const auto& kv2 : kv.second )
			{
				const int& msecs = kv2.first;
				float target1 = lhs.getTarget(ticker, msecs);
				if( rhs.exist(ticker, msecs) )
				{
					float target2 = rhs.getPred(ticker, msecs);
					corrTarget.add(target1, target2);
				}
			}
		}
	}
}

void compPred(const SignalWholeDay& lhs, const SignalWholeDay& rhs, Corr& corrPred, Corr& corrTargetPred1, Corr& corrTargetPred2)
{
	auto fun = std::bind(&SignalWholeDay::getPred, rhs, _1, _2);
	compPredCommon(lhs, rhs, corrPred, corrTargetPred1, corrTargetPred2, lhs.mmPred_, fun);
}

void compBmpred(const SignalWholeDay& lhs, const SignalWholeDay& rhs, Corr& corrPred, Corr& corrTargetPred1, Corr& corrTargetPred2)
{
	auto fun = std::bind(&SignalWholeDay::getBmpred, rhs, _1, _2);
	compPredCommon(lhs, rhs, corrPred, corrTargetPred1, corrTargetPred2, lhs.mmBmpred_, fun);
}

void compPredCommon(const SignalWholeDay& lhs, const SignalWholeDay& rhs, Corr& corrPred, Corr& corrTargetPred1, Corr& corrTargetPred2,
		const MapofMapofFloat& mmPred, std::function<float(string, int)> fun)
{
	for( const auto& kv : mmPred )
	{
		const string& ticker = kv.first;
		if( rhs.exist(ticker) )
		{
			for( const auto& kv2 : kv.second )
			{
				const int& msecs = kv2.first;
				float target = lhs.getTarget(ticker, msecs);
				float pred1 = kv2.second;
				if( rhs.exist(ticker, msecs) )
				{
					float pred2 = fun(ticker, msecs);
					corrPred.add(pred1, pred2);
					corrTargetPred1.add(pred1, target);
					corrTargetPred2.add(pred2, target);
				}
			}
		}
	}
}

map<string, vector<Corr>> compInputByTicker(const SignalWholeDay& lhs, const SignalWholeDay& rhs)
{
	int nInputs = lhs.getNInputs();
	map<string, vector<Corr>> mvCorr;

	for( auto& kv : lhs.mmInput_ )
	{
		const string& ticker = kv.first;
		if( rhs.exist(ticker) )
		{
			vector<Corr> vCorr(nInputs);
			for( auto& kv2 : kv.second )
			{
				const int& msecs = kv2.first;
				const vector<float>& inputs1 = kv2.second;
				if( rhs.exist(ticker, msecs) )
				{
					const vector<float>& inputs2 = rhs.getInputs(ticker, msecs);
					if( inputs1.size() == nInputs && inputs2.size() == nInputs )
					{
						for( int i = 0; i < nInputs; ++i )
							vCorr[i].add(inputs1[i], inputs2[i]);
					}
				}
			}
			mvCorr[ticker] = vCorr;
		}
	}

	return mvCorr;
}

void compPredByTicker(const SignalWholeDay& lhs, const SignalWholeDay& rhs, map<string, Corr>& mCorrPred,
		map<string, Corr>& mCorrTargetPred1, map<string, Corr>& mCorrTargetPred2)
{
	auto fp = std::bind(&SignalWholeDay::getPred, rhs, _1, _2);
	compPredByTickerCommon(lhs, rhs, mCorrPred, mCorrTargetPred1, mCorrTargetPred2, lhs.mmPred_, fp);
}

void compBmpredByTicker(const SignalWholeDay& lhs, const SignalWholeDay& rhs, map<string, Corr>& mCorrPred,
		map<string, Corr>& mCorrTargetPred1, map<string, Corr>& mCorrTargetPred2)
{
	auto fp = std::bind(&SignalWholeDay::getBmpred, rhs, _1, _2);
	compPredByTickerCommon(lhs, rhs, mCorrPred, mCorrTargetPred1, mCorrTargetPred2, lhs.mmBmpred_, fp);
}

void compPredByTickerCommon(const SignalWholeDay& lhs, const SignalWholeDay& rhs, map<string, Corr>& mCorrPred,
		map<string, Corr>& mCorrTargetPred1, map<string, Corr>& mCorrTargetPred2,
		const MapofMapofFloat& mmPred, std::function<float(string, int)> fun)
{
	for( auto& kv : mmPred )
	{
		const string& ticker = kv.first;
		if( rhs.exist(ticker) )
		{
			Corr corrPred;
			Corr corrTargetPred1;
			Corr corrTargetPred2;
			for( auto& kv2 : kv.second )
			{
				const int& msecs = kv2.first;
				float target = lhs.getTarget(ticker, msecs);
				float pred1 = kv2.second;
				if( rhs.exist(ticker, msecs) )
				{
					float pred2 = fun(ticker, msecs);
					corrPred.add(pred1, pred2);
					corrTargetPred1.add(pred1, target);
					corrTargetPred2.add(pred2, target);
				}
			}
			mCorrPred[ticker] = corrPred;
			mCorrTargetPred1[ticker] = corrTargetPred1;
			mCorrTargetPred2[ticker] = corrTargetPred2;
		}
	}
}

} // namespace gtlib
