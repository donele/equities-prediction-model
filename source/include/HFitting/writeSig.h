#ifndef __writeSig__
#define __writeSig__
#include <vector>
#include <fstream>
#include <HLib.h>

namespace writeSig {

	// Paths.
	std::string get_bmTxt_path(std::string model, int idate);
	std::string get_omTxt_path(std::string model, int idate);
	std::string get_omBin_path(std::string model, int idate);
	std::string get_omBintxt_path(std::string model, int idate);
	std::string get_tmTxt_path(std::string model, int idate);
	std::string get_tmBin_path(std::string model, int idate);
	std::string get_tmBinTxt_path(std::string model, int idate);

	// Headers.
	void write_bmTxt_header(std::ofstream& bmTxt, std::string model, int idate);
	void write_omTxt_header(std::ofstream& omTxt, std::string model, int idate);
	void write_omBin_header(std::ofstream& omBin, std::ofstream& omBinTxt, std::string model, int idate);
	void write_tmTxt_header(std::ofstream& tmTxt, std::string model, int idate);
	void write_tmBin_header(std::ofstream& tmBin, std::ofstream& tmBinTxt, std::string model, int idate);

	// Content for each ticker.
	void write_bmTxt(std::ofstream& omTxt, hff::SigC& sig, std::string uid, std::string ticker);
	void write_omTxt(std::ofstream& omTxt, hff::SigC& sig, std::string uid, std::string ticker);
	void write_omBin(std::ofstream& omBin, std::ofstream& omBintxt, hff::SigC& sig, std::string uid, std::string ticker, int& cnt);
	void write_tmTxt(std::ofstream& tmTxt, hff::SigC& sig, std::string uid, std::string ticker);
	void write_tmBin(std::ofstream& tmBin, std::ofstream& tmBintxt, hff::SigC& sig, std::string uid, std::string ticker, int& cnt);

	void update_ncols(std::ofstream& ofs, int cnt);
};

#endif
 
