#include <gtlib_predana/PredWholeDayMultTar.h>
#include <cassert>
#include <algorithm>
using namespace std;

namespace gtlib {

PredWholeDayMultTar::PredWholeDayMultTar(SignalBuffer* pSigBuf)
{
	if( pSigBuf->is_open() )
	{
		vvPredSub_.push_back(pSigBuf->getPredSubs());
		while( pSigBuf->next() )
		{
			const string& ticker = pSigBuf->getTicker();
			int msecs = pSigBuf->getMsecs();
			PredDataMultTar predData;
			predData.target = pSigBuf->getTarget();
			predData.bmpred = pSigBuf->getBmpred();
			predData.pred = pSigBuf->getPred();
			predData.predMulti.push_back(pSigBuf->getPreds());
			predData.sprd = pSigBuf->getSprd();
			predData.price = pSigBuf->getPrice();
			mmData_[ticker][msecs] = predData;
		}
	}
	delete pSigBuf;
}

void PredWholeDayMultTar::merge(const PredWholeDayMultTar* rhs)
{
	PredWholeDayMultTar pwd = mergePredWholeDayMultTar(this, rhs);
	this->swap(pwd);
	delete rhs;
}

void PredWholeDayMultTar::swap(PredWholeDayMultTar& rhs)
{
	this->vvPredSub_.swap(rhs.vvPredSub_);
	this->mmData_.swap(rhs.mmData_);
}

PredWholeDayMultTar mergePredWholeDayMultTar(const PredWholeDayMultTar* lhs, const PredWholeDayMultTar* rhs)
{
	PredWholeDayMultTar pwd;
	pwd.vvPredSub_.push_back(lhs->vvPredSub_[0]);
	pwd.vvPredSub_.push_back(rhs->vvPredSub_[0]);
	for( auto& kv : lhs->mmData_ )
	{
		const string& ticker = kv.first;
		if( rhs->exist(ticker) )
		{
			for( auto& kv2 : kv.second )
			{
				int msecs = kv2.first;
				if( rhs->exist(ticker, msecs) )
				{
					auto obj1 = kv2.second;
					auto obj2 = rhs->mmData_.find(ticker)->second.find(msecs)->second;
					pwd.mmData_[ticker][msecs] = obj1 + obj2;
				}
			}
		}
	}
	return pwd;
}

bool PredWholeDayMultTar::exist(const string& ticker) const
{
	auto it1 = mmData_.find(ticker);
	if( it1 != end(mmData_) )
		return true;
	return false;
}

bool PredWholeDayMultTar::exist(const string& ticker, int msecs) const
{
	auto it1 = mmData_.find(ticker);
	if( it1 != end(mmData_) )
	{
		auto it2 = it1->second.find(msecs);
		if( it2 != it1->second.end() )
			return true;
	}
	return false;
}

vector<string> PredWholeDayMultTar::getTickers() const
{
	vector<string> tickers;
	for( auto& kv : mmData_ )
		tickers.push_back(kv.first);
	return tickers;
}

vector<int> PredWholeDayMultTar::getMsecs(const string& ticker) const
{
	vector<int> msecs;
	auto itT = mmData_.find(ticker);
	if( itT != mmData_.end() )
	{
		for( auto& kv : itT->second )
			msecs.push_back(kv.first);
	}
	return msecs;
}

map<string, vector<int>> PredWholeDayMultTar::getTickerMsecs() const
{
	map<string, vector<int>> tickerMsecs;
	for( auto& kv : mmData_ )
	{
		const string& ticker = kv.first;
		vector<int> msecs;
		for( auto& kv2 : kv.second )
			msecs.push_back(kv2.first);
		tickerMsecs.insert(make_pair(ticker, msecs));
	}
	return tickerMsecs;
}

const PredDataMultTar* PredWholeDayMultTar::getData(const string& ticker, int msecs) const
{
	auto it1 = mmData_.find(ticker);
	if( it1 != end(mmData_) )
	{
		auto it2 = it1->second.find(msecs);
		if( it2 != end(it1->second) )
			return &it2->second;
	}
	assert(0); // User must check the existence.
	return nullptr;
}

float PredWholeDayMultTar::getTarget(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->target;
}

float PredWholeDayMultTar::getBmpred(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->bmpred;
}

float PredWholeDayMultTar::getPred(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->pred;
}

float PredWholeDayMultTar::getPred(const string& ticker, int msecs, int iPred, int predSub) const
{
	auto p = getData(ticker, msecs);
	return p->predMulti[iPred][predSub];
}

vector<vector<int>> PredWholeDayMultTar::getPredSubs() const
{
	return vvPredSub_;
}

vector<vector<float>> PredWholeDayMultTar::getPreds(const std::string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->predMulti;
}

float PredWholeDayMultTar::getSprd(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->sprd;
}

float PredWholeDayMultTar::getPrice(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->price;
}

PredDataMultTar& PredDataMultTar::operator+=(const PredDataMultTar& rhs)
{
	target += rhs.target;
	bmpred += rhs.bmpred;
	pred += rhs.pred;
	predMulti.push_back(rhs.predMulti[0]);
	return *this;
}

const PredDataMultTar PredDataMultTar::operator+(const PredDataMultTar& rhs) const
{
	return PredDataMultTar(*this) += rhs;
}

} // namespace gtlib
