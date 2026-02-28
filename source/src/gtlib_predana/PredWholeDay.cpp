#include <gtlib_predana/PredWholeDay.h>
#include <jl_lib/jlutil.h>
#include <cassert>
#include <algorithm>
#include <iostream>
using namespace std;

namespace gtlib {

PredWholeDay::PredWholeDay(SignalBuffer* pSigBuf, float wgt)
{
	if( pSigBuf->is_open() )
	{
		vPredSub_ = pSigBuf->getPredSubs();
		while( pSigBuf->next() )
		{
			const string& ticker = pSigBuf->getTicker();
			int msecs = pSigBuf->getMsecs();
			PredData predData;
			predData.target = pSigBuf->getTarget();
			predData.bmpred = pSigBuf->getBmpred();
			predData.pred = pSigBuf->getPred();
			predData.predMulti = pSigBuf->getPreds();
			predData.sprd = pSigBuf->getSprd();
			predData.price = pSigBuf->getPrice();
			predData.intraTar = pSigBuf->getIntraTar();
			predData.bidSize = pSigBuf->getBidSize();
			predData.askSize = pSigBuf->getAskSize();
			predData.bidEx = pSigBuf->getBidEx();
			predData.askEx = pSigBuf->getAskEx();

			predData.setWgt(wgt);

			mmData_[ticker][msecs] = predData;
		}
	}
	delete pSigBuf;
}

void PredWholeDay::merge(const PredWholeDay* rhs)
{
	PredWholeDay pwd = mergePredWholeDay(this, rhs);
	this->swap(pwd);
	delete rhs;
}

void PredWholeDay::swap(PredWholeDay& rhs)
{
	this->mmData_.swap(rhs.mmData_);
}

PredWholeDay mergePredWholeDay(const PredWholeDay* lhs, const PredWholeDay* rhs)
{
	PredWholeDay pwd;
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

bool PredWholeDay::exist(const string& ticker) const
{
	auto it1 = mmData_.find(ticker);
	if( it1 != end(mmData_) )
		return true;
	return false;
}

bool PredWholeDay::exist(const string& ticker, int msecs) const
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

vector<string> PredWholeDay::getTickers() const
{
	vector<string> tickers;
	for( auto& kv : mmData_ )
		tickers.push_back(kv.first);
	return tickers;
}

vector<int> PredWholeDay::getMsecs(const string& ticker) const
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

map<string, vector<int>> PredWholeDay::getTickerMsecs() const
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

const PredData* PredWholeDay::getData(const string& ticker, int msecs) const
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

float PredWholeDay::getTarget(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->target;
}

float PredWholeDay::getBmpred(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->bmpred;
}

float PredWholeDay::getPred(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->pred;
}

float PredWholeDay::getPred(const string& ticker, int msecs, int predSub) const
{
	auto p = getData(ticker, msecs);
	auto it = find(begin(vPredSub_), end(vPredSub_), predSub);
	int i = -1;
	if( it != end(vPredSub_) )
		i = it - begin(vPredSub_);
	else
	{
		cerr << "PredWholeDay::getPred() ERROR: pred" << predSub << " not found.\n";
		exit(31);
	}
	return p->predMulti[i];
}

vector<int> PredWholeDay::getPredSubs() const
{
	return vPredSub_;
}

vector<float> PredWholeDay::getPreds(const std::string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->predMulti;
}

float PredWholeDay::getSprd(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->sprd;
}

float PredWholeDay::getPrice(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->price;
}

float PredWholeDay::getIntraTar(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->intraTar;
}

float PredWholeDay::getBidPrice(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->getBidPrice();
}

float PredWholeDay::getAskPrice(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->getAskPrice();
}

int PredWholeDay::getBidSize(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->bidSize;
}

int PredWholeDay::getAskSize(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->askSize;
}

char PredWholeDay::getBidEx(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->bidEx;
}

char PredWholeDay::getAskEx(const string& ticker, int msecs) const
{
	auto p = getData(ticker, msecs);
	return p->askEx;
}

float PredData::getBidPrice() const
{
	float bid = price - .5 * price * sprd / basis_pts_;
	return bid;
}

float PredData::getAskPrice() const
{
	float ask = price + .5 * price * sprd / basis_pts_;
	return ask;
}

void PredData::setWgt(float wgt)
{
	pred *= wgt;
	for(float& p : predMulti)
		p *= wgt;
}

PredData& PredData::operator+=(const PredData& rhs)
{
	target += rhs.target;
	bmpred += rhs.bmpred;
	pred += rhs.pred;
	std::transform(begin(predMulti), end(predMulti), begin(rhs.predMulti),
			begin(predMulti), std::plus<float>());
	return *this;
}

const PredData PredData::operator+(const PredData& rhs) const
{
	return PredData(*this) += rhs;
}

} // namespace gtlib
