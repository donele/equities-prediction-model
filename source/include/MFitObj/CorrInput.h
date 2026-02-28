#ifndef __CorrInput__
#define __CorrInput__
//#include <MFitObj/DTBase.h>
#include <vector>
#include <map>
#include <jl_lib/jlutil.h>
#include <jl_lib/crossCompile.h>

class CLASS_DECLSPEC CorrInput {
	public:
		CorrInput();
		CorrInput(std::vector<std::vector<float> >* pvvInputTarget, std::vector<std::vector<float> >* pvvSpectator, bool debug_ticker);
		void stat(std::string dir, int udate, std::vector<std::string>& inputNames, std::vector<int>& testDates, std::map<int, std::pair<int, int> >& mOffset, std::string model, std::string sigtype, std::string fitdesc);

	private:
		bool isResTar_;
		bool debug_ticker_;
		int udate_;
		std::string statDir_;

		std::vector<std::vector<float> >* pvvInputTarget_;
		std::vector<std::vector<float> >* pvvSpectator_;

		Corr corrTicker_;
		std::map<int, std::string> mIndxTicker_;
		std::multimap<double, std::string> mCorrTicker_;

		void setDir(std::string dir, int udate);
		//void init_ticker_offset(int idate, std::string model, std::string sigtype, std::string fitdesc);
		void process_ticker(int iS, double mret60, double totTarget);
		void debug_ticker_end_day(std::ofstream& ofs, int testDate);
};

#endif
