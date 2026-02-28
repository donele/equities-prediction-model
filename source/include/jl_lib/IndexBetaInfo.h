#ifndef __IndexBetaInfo__
#define __IndexBetaInfo__
#include <string>
#include <unordered_map>
#include <iostream>
#include <string>

class IndexBetaInfo {
	public:
		IndexBetaInfo(){}
		IndexBetaInfo(std::string indexName);
		void add(const std::string& uid, double beta);
		double getBeta(const std::string& uid) const;
	private:
		std::string indexName_;
		std::unordered_map<std::string, float> mUidBeta_;
	friend std::ostream& operator <<(std::ostream& os, const IndexBetaInfo& obj);
	friend std::istream& operator >>(std::istream& is, IndexBetaInfo& obj);
};

#endif
