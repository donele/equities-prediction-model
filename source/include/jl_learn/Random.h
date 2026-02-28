#ifndef __Random__
#define __Random__
#include <string>
#include <vector>
#include <boost/thread.hpp>
#include <boost/random.hpp>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC Random {
public:
	static Random* Instance();
	~Random();
	double next(double range = 1.0);

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
