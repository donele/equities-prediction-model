#include <gtlib_sigread/SignalBufferDecoratorPredMulti.h>
#include <gtlib_fitting/fittingFtns.h>
using namespace std;

namespace gtlib {

SignalBufferDecoratorPredMulti::SignalBufferDecoratorPredMulti(SignalBuffer* pSigBuf,
		const string& predPath)
	:SignalBufferDecorator(pSigBuf),
	sigPred_(predPath)
{
}

bool SignalBufferDecoratorPredMulti::is_open()
{
	return sigPred_.is_open() && SignalBufferDecorator::is_open();
}

bool SignalBufferDecoratorPredMulti::next()
{
	return sigPred_.next() && SignalBufferDecorator::next();
}

float SignalBufferDecoratorPredMulti::getTarget()
{
	return sigPred_.getTarget();
}

float SignalBufferDecoratorPredMulti::getBmpred()
{
	return sigPred_.getBmpred();
}

float SignalBufferDecoratorPredMulti::getPred()
{
	return sigPred_.getPred();
}

float SignalBufferDecoratorPredMulti::getPred(int predSub)
{
	return sigPred_.getPred(predSub);
}

vector<int> SignalBufferDecoratorPredMulti::getPredSubs()
{
	return sigPred_.getPredSubs();
}

float SignalBufferDecoratorPredMulti::getSprd()
{
	return sigPred_.getSprd();
}

float SignalBufferDecoratorPredMulti::getPrice()
{
	return sigPred_.getPrice();
}

float SignalBufferDecoratorPredMulti::getIntraTar()
{
	return sigPred_.getIntraTar();
}

int SignalBufferDecoratorPredMulti::getBidSize()
{
	return sigPred_.getBidSize();
}

int SignalBufferDecoratorPredMulti::getAskSize()
{
	return sigPred_.getAskSize();
}

} // namespace gtlib

