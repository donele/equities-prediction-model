#include <gtlib_sigread/SignalBufferDecorator.h>
using namespace std;

namespace gtlib {

SignalBufferDecorator::SignalBufferDecorator(SignalBuffer* pSigBuf)
	:pSigBuf_(pSigBuf)
{
}

SignalBufferDecorator::~SignalBufferDecorator()
{
	delete pSigBuf_;
}

bool SignalBufferDecorator::is_open()
{
	return pSigBuf_->is_open();
}

const vector<string>& SignalBufferDecorator::getLabels()
{
	return pSigBuf_->getLabels();
}

const vector<string>& SignalBufferDecorator::getInputNames()
{
	return pSigBuf_->getInputNames();
}

bool SignalBufferDecorator::next()
{
	return pSigBuf_->next();
}

int SignalBufferDecorator::getNInputs()
{
	return pSigBuf_->getNInputs();
}

float SignalBufferDecorator::getInput(int i)
{
	return pSigBuf_->getInput(i);
}

float SignalBufferDecorator::getInput(const string& name)
{
	return pSigBuf_->getInput(name);
}

vector<float> SignalBufferDecorator::getInputs()
{
	return pSigBuf_->getInputs();
}

float SignalBufferDecorator::getTarget()
{
	return pSigBuf_->getTarget();
}

float SignalBufferDecorator::getBmpred()
{
	return pSigBuf_->getBmpred();
}

float SignalBufferDecorator::getPred()
{
	return pSigBuf_->getPred();
}

float SignalBufferDecorator::getPred(int i)
{
	return pSigBuf_->getPred(i);
}

vector<int> SignalBufferDecorator::getPredSubs()
{
	return pSigBuf_->getPredSubs();
}

vector<float> SignalBufferDecorator::getPreds()
{
	return pSigBuf_->getPreds();
}

float SignalBufferDecorator::getSprd()
{
	return pSigBuf_->getSprd();
}

float SignalBufferDecorator::getPrice()
{
	return pSigBuf_->getPrice();
}

float SignalBufferDecorator::getIntraTar()
{
	return pSigBuf_->getIntraTar();
}

int SignalBufferDecorator::getBidSize()
{
	return pSigBuf_->getBidSize();
}

int SignalBufferDecorator::getAskSize()
{
	return pSigBuf_->getAskSize();
}

char SignalBufferDecorator::getBidEx()
{
	return pSigBuf_->getBidEx();
}

char SignalBufferDecorator::getAskEx()
{
	return pSigBuf_->getAskEx();
}

const string& SignalBufferDecorator::getTicker()
{
	return pSigBuf_->getTicker();
}

const string& SignalBufferDecorator::getUid()
{
	return pSigBuf_->getUid();
}

int SignalBufferDecorator::getMsecs()
{
	return pSigBuf_->getMsecs();
}

} // namespace gtlib

