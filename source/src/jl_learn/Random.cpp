#include <jl_learn/Random.h>
using namespace std;

Random* Random::instance_ = 0;

Random* Random::Instance()
{
	static Random::Cleaner cleaner;
	if( instance_ == 0 )
		instance_ = new Random();
	return instance_;
}

Random::Cleaner::~Cleaner() {
	delete Random::instance_;
	Random::instance_ = 0;
}

Random::Random()
:variateGen_(boost::variate_generator<boost::mt19937&, boost::uniform_real<> >(gen_, boost::uniform_real<>(-1, 1)))
{
	gen_.seed(static_cast<unsigned int>(std::time(0)));
	boost::uniform_real<> uni_dist(-1, 1);
	//boost::variate_generator<boost::mt19937&, boost::uniform_real<> > variateGen(gen_, uni_dist);
	//variateGen_ = boost::variate_generator<boost::mt19937&, boost::uniform_real<> >(gen_, uni_dist);
}

Random::~Random()
{}

double Random::next(double range)
{
	boost::mutex::scoped_lock lock(gen_mutex_);
	return variateGen_() * range;
}