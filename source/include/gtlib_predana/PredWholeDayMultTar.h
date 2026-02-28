#ifndef __gtlib_predana_PredWholeDayMultTar__
#define __gtlib_predana_PredWholeDayMultTar__
#include <gtlib_sigread/SignalBuffer.h>
#include <unordered_map>
#include <map>
#include <string>

// Read predicitons from multiple targets (fits).
// Do not read signals.

namespace gtlib {

struct PredDataMultTar {
	float target;
	float bmpred;
	float pred;
	float sprd;
	float price;
	std::vector<std::vector<float>> predMulti;
	PredDataMultTar& operator+=(const PredDataMultTar& rhs);
	const PredDataMultTar operator+(const PredDataMultTar& rhs) const;
};

class PredWholeDayMultTar {
public:
	PredWholeDayMultTar(){}
	PredWholeDayMultTar(SignalBuffer* pSigBuf);
	~PredWholeDayMultTar(){}

	void merge(const PredWholeDayMultTar* rhs);
	void swap(PredWholeDayMultTar& rhs);
	bool exist(const std::string& ticker) const;
	bool exist(const std::string& ticker, int msecs) const;
	std::vector<std::string> getTickers() const;
	std::vector<int> getMsecs(const std::string& ticker) const;
	std::map<std::string, std::vector<int>> getTickerMsecs() const;
	float getTarget(const std::string& ticker, int msecs) const ;
	float getBmpred(const std::string& ticker, int msecs) const ;
	float getPred(const std::string& ticker, int msecs) const ;
	float getPred(const std::string& ticker, int msecs, int iPred, int predSub) const;
	std::vector<std::vector<int>> getPredSubs() const;
	std::vector<std::vector<float>> getPreds(const std::string& ticker, int msecs) const;
	float getSprd(const std::string& ticker, int msecs) const;
	float getPrice(const std::string& ticker, int msecs) const;

private:
	std::unordered_map<std::string, std::map<int, PredDataMultTar>> mmData_;
	std::vector<std::vector<int>> vvPredSub_;
	const PredDataMultTar* getData(const std::string& ticker, int msecs) const;
	friend PredWholeDayMultTar mergePredWholeDayMultTar(const PredWholeDayMultTar* lhs, const PredWholeDayMultTar* rhs);
};

} // namespace gtlib

#endif
