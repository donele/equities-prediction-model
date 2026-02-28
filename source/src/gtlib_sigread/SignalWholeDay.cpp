#include <gtlib_sigread/SignalWholeDay.h>
#include <vector>
#include <string>
#include <functional>
using namespace std;
using namespace std::placeholders;

namespace gtlib {

SignalWholeDay::SignalWholeDay(SignalBuffer* pSigBuf, bool simpleTicker)
{
	int cnt = 0;
	for( auto inputName : pSigBuf->getInputNames() )
		mInputNameIndex_[inputName] = cnt++;

	int nInputs = pSigBuf->getNInputs();
	if( pSigBuf->is_open() )
	{
		while( pSigBuf->next() )
		{
			string ticker = simpleTicker ? base_name(pSigBuf->getTicker()) : pSigBuf->getTicker();
			int msecs = pSigBuf->getMsecs();
			vector<float> inputs = pSigBuf->getInputs();
			if( inputs.size() == nInputs )
			{
				mmInput_[ticker][msecs] = inputs;
				mmTarget_[ticker][msecs] = pSigBuf->getTarget();
				mmBmpred_[ticker][msecs] = pSigBuf->getBmpred();
				mmPred_[ticker][msecs] = pSigBuf->getPred();
			}
		}
	}
	delete pSigBuf;
}

void SignalWholeDay::merge(const SignalWholeDay* rhs)
{
	SignalWholeDay swd = mergeSignalWholeDay(this, rhs);
	this->swap(swd);
	delete rhs;
}

void SignalWholeDay::swap(SignalWholeDay& rhs)
{
	this->mInputNameIndex_.swap(rhs.mInputNameIndex_);
	this->mmInput_.swap(rhs.mmInput_);
	this->mmTarget_.swap(rhs.mmTarget_);
	this->mmBmpred_.swap(rhs.mmBmpred_);
	this->mmPred_.swap(rhs.mmPred_);
}

SignalWholeDay mergeSignalWholeDay(const SignalWholeDay* lhs, const SignalWholeDay* rhs)
{
	SignalWholeDay swd;
	swd.mInputNameIndex_ = lhs->mInputNameIndex_;
	for( auto& kv : lhs->mmInput_ )
	{
		const string& ticker = kv.first;
		if( rhs->exist(ticker) )
		{
			for( auto& kv2 : kv.second )
			{
				int msecs = kv2.first;
				if( rhs->exist(ticker, msecs) )
				{
					swd.mmInput_[ticker][msecs] = kv2.second;
					swd.mmTarget_[ticker][msecs] = lhs->getTarget(ticker, msecs) + rhs->getTarget(ticker, msecs);
					swd.mmBmpred_[ticker][msecs] = lhs->getBmpred(ticker, msecs) + rhs->getBmpred(ticker, msecs);
					swd.mmPred_[ticker][msecs] = lhs->getPred(ticker, msecs) + rhs->getPred(ticker, msecs);
				}
			}
		}
	}
	return swd;
}

bool SignalWholeDay::exist(const string& ticker) const
{
	auto it1 = mmInput_.find(ticker);
	if( it1 != end(mmInput_) )
		return true;
	return false;
}

bool SignalWholeDay::exist(const string& ticker, int msecs) const
{
	auto it1 = mmInput_.find(ticker);
	if( it1 != end(mmInput_) )
	{
		auto it2 = it1->second.find(msecs);
		if( it2 != it1->second.end() )
			return true;
	}
	return false;
}

vector<string> SignalWholeDay::getTickers() const
{
	vector<string> tickers;
	for( auto& kv : mmInput_ )
		tickers.push_back(kv.first);
	return tickers;
}

vector<int> SignalWholeDay::getMsecs(const string& ticker) const
{
	vector<int> msecs;
	auto itT = mmInput_.find(ticker);
	if( itT != mmInput_.end() )
	{
		for( auto& kv : itT->second )
			msecs.push_back(kv.first);
	}
	return msecs;
}

map<string, vector<int>> SignalWholeDay::getTickerMsecs() const
{
	map<string, vector<int>> tickerMsecs;
	for( auto& kv : mmInput_ )
	{
		const string& ticker = kv.first;
		vector<int> msecs;
		for( auto& kv2 : kv.second )
			msecs.push_back(kv2.first);
		tickerMsecs.insert(make_pair(ticker, msecs));
	}
	return tickerMsecs;
}

int SignalWholeDay::getNInputs() const
{
	if( !mmInput_.empty() && !begin(mmInput_)->second.empty() )
		return begin(begin(mmInput_)->second)->second.size();
	return 0;
}

const vector<float>& SignalWholeDay::getInputs(const string& ticker, int msecs) const
{
	auto itT = mmInput_.find(ticker);
	if( itT != mmInput_.end() )
	{
		auto itM = itT->second.find(msecs);
		if( itM != itT->second.end() )
			return itM->second;
	}
	return vector<float>();
}

float SignalWholeDay::getInput(const string& ticker, int msecs, const string& inputName) const
{
	auto itm = mInputNameIndex_.find(inputName);
	if( itm != end(mInputNameIndex_) )
		return getInputs(ticker, msecs)[itm->second];
	cerr << "ERROR:SignalSholeDay::getInput(): " << inputName << " was not found.\n";
	return 0.;
}

float SignalWholeDay::getTarget(const string& ticker, int msecs) const
{
	auto it1 = mmTarget_.find(ticker);
	if( it1 != end(mmTarget_) )
	{
		auto it2 = it1->second.find(msecs);
		if( it2 != it1->second.end() )
			return it2->second;
	}
	return 0.;
}

float SignalWholeDay::getBmpred(const string& ticker, int msecs) const
{
	auto it1 = mmBmpred_.find(ticker);
	if( it1 != end(mmBmpred_) )
	{
		auto it2 = it1->second.find(msecs);
		if( it2 != it1->second.end() )
			return it2->second;
	}
	return 0.;
}

float SignalWholeDay::getPred(const string& ticker, int msecs) const
{
	auto it1 = mmPred_.find(ticker);
	if( it1 != end(mmPred_) )
	{
		auto it2 = it1->second.find(msecs);
		if( it2 != it1->second.end() )
			return it2->second;
	}
	return 0.;
}

} // namespace gtlib
