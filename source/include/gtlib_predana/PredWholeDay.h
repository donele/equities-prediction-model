#ifndef __gtlib_predana_PredWholeDay__
#define __gtlib_predana_PredWholeDay__
#include <gtlib_sigread/SignalBuffer.h>
#include <unordered_map>
#include <map>
#include <string>

// Reads predictions.
// Do not read signals.

namespace gtlib {

struct PredData {
	float target;
	float bmpred;
	float pred;
	float sprd;
	float price;
	float intraTar;
	int bidSize;
	int askSize;
	char bidEx;
	char askEx;
	std::vector<float> predMulti;
	float getBidPrice() const;
	float getAskPrice() const;
	void setWgt(float wgt);
	PredData& operator+=(const PredData& rhs);
	const PredData operator+(const PredData& rhs) const;
};

class PredWholeDay {
public:
	PredWholeDay(){}
	PredWholeDay(SignalBuffer* pSigBuf, float wgt = 1.);
	~PredWholeDay(){}

	void merge(const PredWholeDay* rhs);
	void swap(PredWholeDay& rhs);
	bool exist(const std::string& ticker) const;
	bool exist(const std::string& ticker, int msecs) const;
	std::vector<std::string> getTickers() const;
	std::vector<int> getMsecs(const std::string& ticker) const;
	std::map<std::string, std::vector<int>> getTickerMsecs() const;
	float getTarget(const std::string& ticker, int msecs) const ;
	float getBmpred(const std::string& ticker, int msecs) const ;
	float getPred(const std::string& ticker, int msecs) const ;
	float getPred(const std::string& ticker, int msecs, int predSub) const;
	std::vector<int> getPredSubs() const;
	std::vector<float> getPreds(const std::string& ticker, int msecs) const;
	float getSprd(const std::string& ticker, int msecs) const;
	float getPrice(const std::string& ticker, int msecs) const;
	float getIntraTar(const std::string& ticker, int msecs) const;
	float getBidPrice(const std::string& ticker, int msecs) const;
	float getAskPrice(const std::string& ticker, int msecs) const;
	int getBidSize(const std::string& ticker, int msecs) const;
	int getAskSize(const std::string& ticker, int msecs) const;
	char getBidEx(const std::string& ticker, int msecs) const;
	char getAskEx(const std::string& ticker, int msecs) const;

private:
	std::unordered_map<std::string, std::map<int, PredData>> mmData_;
	std::vector<int> vPredSub_;
	const PredData* getData(const std::string& ticker, int msecs) const;
	friend PredWholeDay mergePredWholeDay(const PredWholeDay* lhs, const PredWholeDay* rhs);
};

} // namespace gtlib

#endif
