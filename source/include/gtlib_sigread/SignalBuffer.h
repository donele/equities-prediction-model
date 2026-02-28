#ifndef __gtlib_sigread_SignalBuffer__
#define __gtlib_sigread_SignalBuffer__
#include <vector>
#include <string>

namespace gtlib {

class SignalBuffer {
public:
	SignalBuffer();
	virtual ~SignalBuffer(){}

	virtual bool is_open() = 0;
	virtual bool next() = 0;
	virtual const std::vector<std::string>& getLabels() = 0;
	virtual const std::vector<std::string>& getInputNames() = 0;
	virtual int getNInputs() = 0;
	virtual float getInput(int i) = 0;
	virtual float getInput(const std::string& name) = 0;
	virtual std::vector<float> getInputs() = 0;

	virtual float getTarget() = 0;
	virtual float getBmpred() = 0;
	virtual float getPred() = 0;
	virtual float getPred(int i) = 0;
	virtual std::vector<int> getPredSubs() = 0;
	virtual std::vector<float> getPreds() = 0;
	virtual float getSprd() = 0;
	virtual float getPrice() = 0;
	virtual float getIntraTar() = 0;
	virtual int getBidSize() = 0;
	virtual int getAskSize() = 0;
	virtual char getBidEx() {return 0.;}
	virtual char getAskEx() {return 0.;}

	virtual const std::string& getTicker() = 0;
	virtual const std::string& getUid() = 0;
	virtual int getMsecs() = 0;
private:
};

} // namespace gtlib

#endif
