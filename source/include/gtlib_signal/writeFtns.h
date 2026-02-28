#ifndef __writeFtns__
#define __writeFtns__
#include <vector>
#include <unordered_map>
#include <fstream>
#include <MFramework.h>
#include <gtlib_model/hff.h>

namespace gtlib {

void write_npz(const std::string& npPath, const std::string& npTxtPath, std::vector<hff::SigC>* pSig,
		const hff::LinearModel& linMod, std::unordered_map<std::string, std::string> mTickerUid);

// Headers.
void write_bin_header(const std::string& sigType, std::ofstream& bin, std::ofstream& binTxt,
		const hff::LinearModel& linMod, const hff::NonLinearModel& nonLinMod);
void write_omBin_header(std::ofstream& omBin, std::ofstream& omBinTxt,
		const hff::LinearModel& linMod, const hff::NonLinearModel& nonLinMod);
void write_tmBin_header(std::ofstream& tmBin, std::ofstream& tmBinTxt,
		const hff::LinearModel& linMod, const hff::NonLinearModel& nonLinMod);

// Contents.
int write_bin_evt(const std::string& sigType, std::ofstream& bin, std::ofstream& binTxt, const hff::SigC& sig,
		const std::string& uid, const std::string& ticker, int minMsecs, int maxMsecs, int sampleType,
		const hff::LinearModel& linMod, const hff::NonLinearModel& nonLinMod);
int write_omBin_evt(std::ofstream& omBin, std::ofstream& omBintxt, const hff::SigC& sig,
		const std::string& uid, const std::string& ticker, int minMsecs, int maxMsecs, int sampleType,
		const hff::LinearModel& linMod, const hff::NonLinearModel& nonLinMod);
int write_tmBin_evt(std::ofstream& tmBin, std::ofstream& tmBintxt, const hff::SigC& sig,
		const std::string& uid, const std::string& ticker, int minMsecs, int maxMsecs, int sampleType,
		const hff::LinearModel& linMod, const hff::NonLinearModel& nonLinMod);

// ncols.
void update_ncols(std::ofstream& ofs, int cnt);

} // namespace gtlib

#endif
