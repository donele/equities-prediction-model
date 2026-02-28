#ifndef __gtlib_sigread_SignalBufferBin__
#define __gtlib_sigread_SignalBufferBin__
#include <gtlib_sigread/SignalBuffer.h>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>

namespace gtlib {

class SignalBufferBin: public SignalBuffer{
public:
	SignalBufferBin(){}
	SignalBufferBin(const std::string& binPath, const std::vector<std::string>& inputNames);
	virtual ~SignalBufferBin(){}

	virtual bool is_open();
	virtual bool next();
	virtual const std::vector<std::string>& getLabels();
	virtual const std::vector<std::string>& getInputNames();
	virtual int getNInputs();
	virtual float getInput(int i);
	virtual float getInput(const std::string& name);
	virtual std::vector<float> getInputs();

	virtual float getTarget() {return 0.;}
	virtual float getBmpred() {return 0.;}
	virtual float getPred() {return 0.;}
	virtual float getPred(int i) {return 0.;}
	virtual std::vector<int> getPredSubs() {return std::vector<int>();}
	virtual std::vector<float> getPreds() {return std::vector<float>();}
	virtual float getSprd() {return 0.;}
	virtual float getPrice() {return 0.;}
	virtual float getIntraTar() {return 0.;}
	virtual int getBidSize() {return 0;}
	virtual int getAskSize() {return 0;}

	virtual const std::string& getTicker() {return "";}
	virtual const std::string& getUid() {return "";}
	virtual int getMsecs() {return 0;}
private:
	int nInputFields_;
	std::ifstream ifs_;
	std::vector<int> indexMap_;
	std::map<std::string, int> nameMap_;
	std::vector<std::string> labels_;
	std::vector<std::string> inputNames_;
	std::vector<float> rawInput_;

	void readHeader();
	void setDesiredInputs();
};

} // namespace gtlib

#endif
