#ifndef __writeSig_at__
#define __writeSig_at__
#include <vector>
#include <fstream>
#include <MFramework.h>

namespace writeSig {

int write_bin_evt(const std::string& sigType, std::ofstream& bin, std::ofstream& binTxt, const hff::SigC& sig,
		const std::string& uid, const std::string& ticker, int minMsecs, int maxMsecs, int sampleType);
int write_omBin_evt(std::ofstream& omBin, std::ofstream& omBintxt, const hff::SigC& sig,
		const std::string& uid, const std::string& ticker, int minMsecs, int maxMsecs, int sampleType);
int write_tmBin_evt(std::ofstream& tmBin, std::ofstream& tmBintxt, const hff::SigC& sig,
		const std::string& uid, const std::string& ticker, int minMsecs, int maxMsecs, int sampleType);

} // namespace writeSig

#endif

