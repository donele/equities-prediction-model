#ifndef __gtlib_sigread_SignalWholeDay__
#define __gtlib_sigread_SignalWholeDay__
#include <gtlib_sigread/SignalBuffer.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>

using MapofMapofFloat = std::unordered_map<std::string, std::unordered_map<int, float>>;
using MapofMapofVectorofFloat = std::unordered_map<std::string, std::unordered_map<int, std::vector<float>>>;

namespace gtlib {

class SignalWholeDay {

public:
	SignalWholeDay(){}
	SignalWholeDay(SignalBuffer* pSigBuf, bool simpleTicker = false);
	~SignalWholeDay(){}

	void merge(const SignalWholeDay* rhs);
	void swap(SignalWholeDay& rhs);
	bool exist(const std::string& ticker) const;
	bool exist(const std::string& ticker, int msecs) const;
	std::vector<std::string> getTickers() const;
	std::vector<int> getMsecs(const std::string& ticker) const;
	std::map<std::string, std::vector<int>> getTickerMsecs() const;
	int getNInputs() const;
	const std::vector<float>& getInputs(const std::string& ticker, int msecs) const;
	float getInput(const std::string& ticker, int msecs, const std::string& inputName) const;
	float getTarget(const std::string& ticker, int msecs) const ;
	float getBmpred(const std::string& ticker, int msecs) const ;
	float getPred(const std::string& ticker, int msecs) const ;

private:
	std::map<std::string, int> mInputNameIndex_;
	MapofMapofVectorofFloat mmInput_;
	MapofMapofFloat mmTarget_;
	MapofMapofFloat mmBmpred_;
	MapofMapofFloat mmPred_;

	friend SignalWholeDay mergeSignalWholeDay(const SignalWholeDay* lhs, const SignalWholeDay* rhs);
	friend void printVar(const SignalWholeDay& lhs, const SignalWholeDay& rhs, int varIndex);
	friend void printBmpred(const SignalWholeDay& lhs, const SignalWholeDay& rhs);
	friend void printPred(const SignalWholeDay& lhs, const SignalWholeDay& rhs);
	friend void printPred(const SignalWholeDay& rhs, const MapofMapofFloat& mmPred,
			std::function<float(std::string, int)> fun, const std::string& filename);

	friend std::vector<Corr> compInput(const SignalWholeDay& lhs, const SignalWholeDay& rhs);
	friend void compTarget(const gtlib::SignalWholeDay& lhs, const gtlib::SignalWholeDay& rhs, Corr& corrTarget);
	friend void compPred(const SignalWholeDay& lhs, const SignalWholeDay& rhs, Corr& corrPred, Corr& corrTargetPred1, Corr& corrTargetPred2);
	friend void compBmpred(const SignalWholeDay& lhs, const SignalWholeDay& rhs, Corr& corrPred, Corr& corrTargetPred1, Corr& corrTargetPred2);
	friend void compPredCommon(const SignalWholeDay& lhs, const SignalWholeDay& rhs, Corr& corrPred, Corr& corrTargetPred1, Corr& corrTargetPred2,
			const MapofMapofFloat& mmPred, std::function<float(std::string, int)> fun);

	friend std::map<std::string, std::vector<Corr>> compInputByTicker(const SignalWholeDay& lhs, const SignalWholeDay& rhs);
	friend void compPredByTicker(const SignalWholeDay& lhs, const SignalWholeDay& rhs, std::map<std::string, Corr>& mCorrPred,
			std::map<std::string, Corr>& mCorrTargetPred1, std::map<std::string, Corr>& mCorrTargetPred2);
	friend void compBmpredByTicker(const SignalWholeDay& lhs, const SignalWholeDay& rhs, std::map<std::string, Corr>& mCorrPred,
			std::map<std::string, Corr>& mCorrTargetPred1, std::map<std::string, Corr>& mCorrTargetPred2);
	friend void compPredByTickerCommon(const SignalWholeDay& lhs, const SignalWholeDay& rhs, std::map<std::string, Corr>& mCorrPred,
			std::map<std::string, Corr>& mCorrTargetPred1, std::map<std::string, Corr>& mCorrTargetPred2,
			const MapofMapofFloat& mmPred, std::function<float(std::string, int)> fun);

};

} // namespace gtlib

#endif
