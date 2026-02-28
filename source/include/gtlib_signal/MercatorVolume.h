#ifndef __gtlib_signal_MercatorVolume__
#define __gtlib_signal_MercatorVolume__
#include <map>
#include <vector>
#include <string>

namespace gtlib {

class MercatorVolume {
public:
	MercatorVolume();
	~MercatorVolume(){}
	void beginDay(const std::string& model, int idate);
	int getSampleInterval(const std::string& ticker, int stepSec) const;
private:
	int nq_;
	int yyyymm_;
	float norm_;
	std::vector<float> vBinVol_;
	std::map<std::string, int> mTickerIBin_;

	int getPrevMonth(int idate);
	void read_db(const std::string& model);
	void getTickerBinMap(const std::vector<std::string>& vTicker, const std::vector<float>& vVol);
	void getVBinVol(const std::vector<std::string>& vTicker, std::vector<float> vVol);
};

} // namespace gtlib

#endif
