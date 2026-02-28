#ifndef __SigSet__
#define __SigSet__
#include <gtlib_model/hff.h>
#include <vector>
#include <boost/thread.hpp>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC SigSet {
public:
	static SigSet* Instance();

	void resize(int nThread);
	hff::SigC& get_sig_object(int& iSig);
	hff::SigC& get_sig_object(int& iSig, int n);
	hff::SigC& get_sig_object(int& iSig, const std::vector<int>& vMsecs);
	void free_sig_object(int iSig);

private:
	static SigSet* instance_;
	SigSet();

	boost::mutex sig_mutex_;
	int nThreads_;
	std::vector<int> vSigInUse_;
	std::vector<hff::SigC> vSig_;
};

#endif
