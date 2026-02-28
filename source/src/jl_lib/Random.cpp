#include <jl_lib/Random.h>
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
}

Random::~Random()
{}

double Random::next(double range)
{
	boost::mutex::scoped_lock lock(gen_mutex_);
	return variateGen_() * range;
}

double Random::next(double min, double max)
{
	boost::mutex::scoped_lock lock(gen_mutex_);
	return (variateGen_() + 1.) / 2. * (max - min) + min;
}

int Random::next_int(int max_int)
{
	boost::mutex::scoped_lock lock(gen_mutex_);
	return (int)((variateGen_() + 1.0) / 2.0 * max_int);
}

bool Random::select(double prob)
{
	if( prob <= 0. )
		return false;
	else if( prob >= 1. )
		return true;
	else
	{
		boost::mutex::scoped_lock lock(gen_mutex_);
		double rnd = variateGen_();
		return (rnd + 1.0) / 2.0 < prob;
	}
	return false;
}
