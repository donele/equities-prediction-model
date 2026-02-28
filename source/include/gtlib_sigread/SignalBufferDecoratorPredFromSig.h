#ifndef __gtlib_sigread_SignalBufferDecoratorPredFromSig__
#define __gtlib_sigread_SignalBufferDecoratorPredFromSig__
#include <gtlib_sigread/SignalBuffer.h>
#include <gtlib_sigread/SignalBufferDecorator.h>
#include <gtlib_sigread/SignalBufferPred.h>
#include <fstream>
#include <vector>
#include <string>

namespace gtlib {

class SignalBufferDecoratorPredFromSig: public SignalBufferDecorator {
public:
	SignalBufferDecoratorPredFromSig(){}
	SignalBufferDecoratorPredFromSig(SignalBuffer* pSigBuf, const std::string& predPath,
			const std::vector<std::string>& targetReplacements);
	virtual ~SignalBufferDecoratorPredFromSig(){}

	virtual bool is_open();
	virtual bool next();

	virtual float getTarget();
	virtual float getBmpred();
	virtual float getPred();
	virtual float getSprd();
	virtual float getPrice();
	virtual float getIntraTar();
private:
	SignalBufferPred sigPred_;
	std::vector<std::string> targetReplacements_;
};

} // namespace gtlib

#endif
