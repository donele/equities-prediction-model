#include <gtlib_sigread/SignalBufferDecoratorPredFromSig.h>
#include <gtlib_fitting/fittingFtns.h>
using namespace std;

namespace gtlib {

SignalBufferDecoratorPredFromSig::SignalBufferDecoratorPredFromSig(SignalBuffer* pSigBuf,
		const string& predPath, const vector<string>& targetReplacements)
	:SignalBufferDecorator(pSigBuf),
	sigPred_(predPath),
	targetReplacements_(targetReplacements)
{
}

bool SignalBufferDecoratorPredFromSig::is_open()
{
	return sigPred_.is_open() && SignalBufferDecorator::is_open();
}

bool SignalBufferDecoratorPredFromSig::next()
{
	return sigPred_.next() && SignalBufferDecorator::next();
}

float SignalBufferDecoratorPredFromSig::getTarget()
{
	float target = 0.;
	if( !targetReplacements_.empty() )
	{
		for( auto& name : targetReplacements_ )
			target += pSigBuf_->getInput(name);
	}
	return target;
}

float SignalBufferDecoratorPredFromSig::getBmpred()
{
	return sigPred_.getBmpred();
}

float SignalBufferDecoratorPredFromSig::getPred()
{
	return sigPred_.getPred();
}

float SignalBufferDecoratorPredFromSig::getSprd()
{
	return sigPred_.getSprd();
}

float SignalBufferDecoratorPredFromSig::getPrice()
{
	return sigPred_.getPrice();
}

float SignalBufferDecoratorPredFromSig::getIntraTar()
{
	return sigPred_.getIntraTar();
}

} // namespace gtlib

