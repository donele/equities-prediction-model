#include <gtlib_sigread/SignalBufferDecoratorPred.h>
#include <gtlib_fitting/fittingFtns.h>
using namespace std;

namespace gtlib {

SignalBufferDecoratorPred::SignalBufferDecoratorPred(SignalBuffer* pSigBuf,
		const string& predPath)
	:SignalBufferDecorator(pSigBuf),
	sigPred_(predPath)
{
}

bool SignalBufferDecoratorPred::is_open()
{
	return sigPred_.is_open() && SignalBufferDecorator::is_open();
}

bool SignalBufferDecoratorPred::next()
{
	return sigPred_.next() && SignalBufferDecorator::next();
}

float SignalBufferDecoratorPred::getTarget()
{
	return sigPred_.getTarget();
}

float SignalBufferDecoratorPred::getBmpred()
{
	return sigPred_.getBmpred();
}

float SignalBufferDecoratorPred::getPred()
{
	return sigPred_.getPred();
}

float SignalBufferDecoratorPred::getSprd()
{
	return sigPred_.getSprd();
}

float SignalBufferDecoratorPred::getPrice()
{
	return sigPred_.getPrice();
}

float SignalBufferDecoratorPred::getIntraTar()
{
	return sigPred_.getIntraTar();
}

int SignalBufferDecoratorPred::getBidSize()
{
	return sigPred_.getBidSize();
}

int SignalBufferDecoratorPred::getAskSize()
{
	return sigPred_.getAskSize();
}

} // namespace gtlib

