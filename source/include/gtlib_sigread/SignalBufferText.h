#ifndef __gtlib_sigread_SignalBufferText__
#define __gtlib_sigread_SignalBufferText__
#include <gtlib_sigread/SignalBuffer.h>
#include <fstream>
#include <vector>
#include <map>
#include <string>

namespace gtlib {

class SignalBufferText: public SignalBuffer {
public:
	SignalBufferText(const std::string& sigBase, const std::vector<std::string>& inputNames, int openMsecs = 0);
	virtual ~SignalBufferText(){}

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
	virtual float getPred(int i){return 0.;}
	virtual std::vector<int> getPredSubs(){return std::vector<int>();}
	virtual std::vector<float> getPreds() {return std::vector<float>();}
	virtual float getSprd(){return 0.;}
	virtual float getPrice(){return 0.;}
	virtual float getIntraTar(){return 0.;}
	virtual int getBidSize(){return 0.;}
	virtual int getAskSize(){return 0.;}

	virtual const std::string& getTicker();
	virtual const std::string& getUid();
	virtual int getMsecs();
private:
	int nInputFields_;
	int openMsecs_;
	std::ifstream ifs_;
	std::vector<int> indexMap_;
	std::map<std::string, int> nameMap_;
	std::vector<std::string> labels_;
	std::vector<std::string> inputNames_;
	std::vector<std::string> rawInputString_;
	int targetIndex_;
	int bmpredIndex_;
	int predIndex_;
	int tickerIndex_;
	int uidIndex_;
	int msecsIndex_;

	void readLabels();
	void setDesiredInputs();
	int findIndex(const std::string& label);
};

} // namespace gtlib

#endif
