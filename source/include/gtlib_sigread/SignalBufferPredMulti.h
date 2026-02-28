#ifndef __gtlib_sigread_SignalBufferPredMulti__
#define __gtlib_sigread_SignalBufferPredMulti__
#include <gtlib_sigread/SignalBuffer.h>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>

namespace gtlib {

class SignalBufferPredMulti: public SignalBuffer {
public:
	SignalBufferPredMulti(){}
	SignalBufferPredMulti(const std::string& predPath);
	virtual ~SignalBufferPredMulti(){}

	virtual bool is_open();
	virtual bool next();
	virtual const std::vector<std::string>& getLabels(){return std::vector<std::string>();}
	virtual const std::vector<std::string>& getInputNames(){return std::vector<std::string>();}
	virtual int getNInputs(){return 0;}
	virtual float getInput(int i){return 0.;}
	virtual float getInput(const std::string& name){return 0.;}
	virtual std::vector<float> getInputs(){return std::vector<float>();}

	virtual float getTarget();
	virtual float getBmpred();
	virtual float getPred();
	virtual float getPred(int predSub);
	virtual std::vector<int> getPredSubs();
	virtual std::vector<float> getPreds();
	virtual float getSprd();
	virtual float getPrice();
	virtual float getIntraTar();
	virtual int getBidSize();
	virtual int getAskSize();

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
	std::map<int, int> mPredIndex_;
	std::vector<std::string> labels_;
	std::vector<float> rawInput_;

	int getIndex(const std::string& label);
	void getPredIndex();
};

} // namespace gtlib

#endif
