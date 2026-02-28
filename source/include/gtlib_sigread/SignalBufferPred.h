#ifndef __gtlib_sigread_SignalBufferPred__
#define __gtlib_sigread_SignalBufferPred__
#include <gtlib_sigread/SignalBuffer.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

namespace gtlib {

class SignalBufferPred: public SignalBuffer {
public:
	SignalBufferPred(){}
	SignalBufferPred(const std::string& predPath);
	virtual ~SignalBufferPred(){}

	virtual bool is_open();
	virtual bool next();
	virtual const std::vector<std::string>& getLabels(){return std::vector<std::string>();}
	virtual const std::vector<std::string>& getInputNames(){return std::vector<std::string>();}
	virtual int getNInputs(){return 0;}
	virtual float getInput(int i){return 0.;}
	virtual float getInput(const std::string& name){return 0.;}
	virtual std::vector<float> getInputs(){return std::vector<float>();}

	float getTarget();
	float getBmpred();
	float getPred();
	float getPred(int i){return 0.;}
	virtual std::vector<int> getPredSubs() {return std::vector<int>();}
	virtual std::vector<float> getPreds() {return std::vector<float>();}
	float getSprd();
	float getPrice();
	float getIntraTar();
	int getBidSize();
	int getAskSize();
	char getBidEx() {return 0;}
	char getAskEx() {return 0;}

	virtual const std::string& getTicker() {return "";}
	virtual const std::string& getUid() {return "";}
	virtual int getMsecs() {return 0;}
private:
	std::ifstream ifs_;
	int targetIndex_;
	int bmpredIndex_;;
	int predIndex_;;
	int sprdIndex_;;
	int priceIndex_;;
	int intraTarIndex_;;
	int bidSizeIndex_;;
	int askSizeIndex_;;
	std::vector<std::string> labels_;
	std::vector<float> rawInput_;

	int getIndex(const std::string& label);
};

} // namespace gtlib

#endif
