#ifndef __gtlib_sigread_SignalBufferDecorator__
#define __gtlib_sigread_SignalBufferDecorator__
#include <gtlib_sigread/SignalBuffer.h>
#include <fstream>
#include <vector>
#include <string>

namespace gtlib {

class SignalBufferDecorator: public SignalBuffer {
public:
	SignalBufferDecorator(){}
	SignalBufferDecorator(SignalBuffer* pSigBuf);
	virtual ~SignalBufferDecorator();

	virtual bool is_open();
	virtual bool next();
	virtual const std::vector<std::string>& getLabels();
	virtual const std::vector<std::string>& getInputNames();
	virtual int getNInputs();
	virtual float getInput(int i);
	virtual float getInput(const std::string& name);
	virtual std::vector<float> getInputs();

	virtual float getTarget();
	virtual float getBmpred();
	virtual float getPred();
	virtual float getPred(int i);
	virtual std::vector<int> getPredSubs();
	virtual std::vector<float> getPreds();
	virtual float getSprd();
	virtual float getPrice();
	virtual float getIntraTar();
	virtual int getBidSize();
	virtual int getAskSize();
	virtual char getBidEx();
	virtual char getAskEx();

	virtual const std::string& getTicker();
	virtual const std::string& getUid();
	virtual int getMsecs();
protected:
	SignalBuffer* pSigBuf_;
};

} // namespace gtlib

#endif
