#include <jl_lib/IndexBetaInfo.h>
#include <jl_lib/jlutil.h>
using namespace std;

IndexBetaInfo::IndexBetaInfo(string indexName)
	:indexName_(indexName)
{
}

void IndexBetaInfo::add(const string& uid, double beta)
{
	mUidBeta_[uid] = beta;
}

double IndexBetaInfo::getBeta(const std::string& uid) const
{
	auto it = mUidBeta_.find(uid);
	if( it != end(mUidBeta_) )
		return it->second;
	return 1.;
}

ostream& operator <<(ostream& os, const IndexBetaInfo& obj)
{
	os << obj.indexName_ << endl;
	for( auto& kv : obj.mUidBeta_ )
		if( !kv.first.empty() )
			os << kv.first << "\t" << kv.second << "\n";
	return os;
}

istream& operator >>(std::istream& is, IndexBetaInfo& obj)
{
	is >> obj.indexName_;
	string line;
	while( getline(is, line) )
	{
		auto sl = split(line, '\t');
		if( sl.size() == 2 )
		{
			string uid = sl[0];
			double beta = stof(sl[1]);
			obj.mUidBeta_[uid] = beta;
		}
	}
	return is;
}
