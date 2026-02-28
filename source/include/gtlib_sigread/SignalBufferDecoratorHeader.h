#ifndef __gtlib_sigread_SignalBufferDecoratorHeader__
#define __gtlib_sigread_SignalBufferDecoratorHeader__
#include <gtlib_sigread/SignalBuffer.h>
#include <gtlib_sigread/SignalBufferDecorator.h>
#include <gtlib_sigread/SignalHeader.h>
#include <fstream>
#include <vector>
#include <string>

namespace gtlib {

class SignalBufferDecoratorHeader: public SignalBufferDecorator {
public:
	SignalBufferDecoratorHeader(){}
	SignalBufferDecoratorHeader(SignalBuffer* pSigBuf, const std::string& binPath);
	virtual ~SignalBufferDecoratorHeader(){}

	virtual bool is_open();
	virtual bool next();

	virtual const std::string& getTicker();
	virtual const std::string& getUid();
	virtual int getMsecs();
	virtual char getBidEx();
	virtual char getAskEx();
private:
	SignalHeader sigHeader_;
};

} // namespace gtlib

#endif
