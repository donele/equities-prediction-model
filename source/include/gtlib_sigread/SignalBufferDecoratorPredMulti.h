#ifndef __gtlib_sigread_SignalBufferDecoratorPredMulti__
#define __gtlib_sigread_SignalBufferDecoratorPredMulti__
#include <gtlib_sigread/SignalBuffer.h>
#include <gtlib_sigread/SignalBufferDecorator.h>
#include <gtlib_sigread/SignalBufferPred.h>
#include <fstream>
#include <vector>
#include <string>

// 20160223: This class is not used anywhere.

namespace gtlib {

class SignalBufferDecoratorPredMulti: public SignalBufferDecorator {
public:
	SignalBufferDecoratorPredMulti(){}
	SignalBufferDecoratorPredMulti(SignalBuffer* pSigBuf, const std::string& predPath);
	virtual ~SignalBufferDecoratorPredMulti(){}

	virtual bool is_open();
	virtual bool next();

	virtual float getTarget();
	virtual float getBmpred();
	virtual float getPred();
	virtual float getPred(int predSub);
	virtual std::vector<int> getPredSubs();
	virtual float getSprd();
	virtual float getPrice();
	virtual float getIntraTar();
	virtual int getBidSize();
	virtual int getAskSize();
private:
	SignalBufferPredMulti sigPred_;
};

} // namespace gtlib

#endif
