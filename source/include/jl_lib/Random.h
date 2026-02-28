#ifndef __Random__
#define __Random__
#include <string>
#include <vector>
#include <jl_lib/crossCompile.h>
#include <boost/thread.hpp>
#include <boost/random.hpp>

class CLASS_DECLSPEC Random {
public:
	static Random* Instance();
	~Random();
	double next(double range = 1.0);
	double next(double min, double max);
	int next_int(int max_int);
	bool select(double prob);

private:
	static Random* instance_;
	struct Cleaner { ~Cleaner(); };
	friend struct Cleaner;
	Random();

	boost::mt19937 gen_;
	boost::variate_generator<boost::mt19937&, boost::uniform_real<> > variateGen_;
	boost::mutex gen_mutex_;
};

#endif
