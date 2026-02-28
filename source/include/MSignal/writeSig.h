#ifndef __writeSig__
#define __writeSig__
#include <vector>
#include <fstream>
#include <MFramework.h>

namespace writeSig {
// Headers.
void write_bin_header(const std::string& sigType, std::ofstream& bin, std::ofstream& binTxt);
void write_omBin_header(std::ofstream& omBin, std::ofstream& omBinTxt);
void write_tmBin_header(std::ofstream& tmBin, std::ofstream& tmBinTxt);

// Content for each ticker.
int write_omBin(std::ofstream& omBin, std::ofstream& omBintxt, const hff::SigC& sig,
		const std::string& uid, const std::string& ticker, int minMsecs, int maxMsecs, bool debug_sprd);
int write_tmBin(std::ofstream& tmBin, std::ofstream& tmBintxt, const hff::SigC& sig,
		const std::string& uid, const std::string& ticker, int minMsecs, int maxMsecs, bool debug_sprd);

void update_ncols(std::ofstream& ofs, int cnt);
};

#endif

