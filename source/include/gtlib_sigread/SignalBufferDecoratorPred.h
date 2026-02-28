#ifndef __gtlib_sigread_SignalBufferDecoratorPred__
#define __gtlib_sigread_SignalBufferDecoratorPred__
#include <gtlib_sigread/SignalBuffer.h>
#include <gtlib_sigread/SignalBufferDecorator.h>
#include <gtlib_sigread/SignalBufferPred.h>
#include <fstream>
#include <vector>
#include <string>

namespace gtlib {

class SignalBufferDecoratorPred: public SignalBufferDecorator {
public:
	SignalBufferDecoratorPred(){}
	SignalBufferDecoratorPred(SignalBuffer* pSigBuf, const std::string& predPath);
	virtual ~SignalBufferDecoratorPred(){}

	virtual bool is_open();
	virtual bool next();

	virtual float getTarget();
	virtual float getBmpred();
	virtual float getPred();
	virtual float getSprd();
	virtual float getPrice();
	virtual float getIntraTar();
	virtual int getBidSize();
	virtual int getAskSize();
private:
	SignalBufferPred sigPred_;
};

} // namespace gtlib

#endif
