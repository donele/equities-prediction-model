#include <gtlib_sigread/SignalBufferDecoratorHeader.h>
#include <gtlib_fitting/fittingFtns.h>
using namespace std;

namespace gtlib {

SignalBufferDecoratorHeader::SignalBufferDecoratorHeader(SignalBuffer* pSigBuf, const string& binPath)
	:SignalBufferDecorator(pSigBuf),
	sigHeader_(getTxtPath(binPath))
{
}

bool SignalBufferDecoratorHeader::is_open()
{
	return sigHeader_.is_open() && SignalBufferDecorator::is_open();
}

bool SignalBufferDecoratorHeader::next()
{
	return sigHeader_.next() && SignalBufferDecorator::next();
}

const string& SignalBufferDecoratorHeader::getTicker()
{
	return sigHeader_.getTicker();
}

const string& SignalBufferDecoratorHeader::getUid()
{
	return sigHeader_.getUid();
}

int SignalBufferDecoratorHeader::getMsecs()
{
	return sigHeader_.getMsecs();
}

char SignalBufferDecoratorHeader::getBidEx()
{
	return sigHeader_.getBidEx();
}

char SignalBufferDecoratorHeader::getAskEx()
{
	return sigHeader_.getAskEx();
}

} // namespace gtlib

