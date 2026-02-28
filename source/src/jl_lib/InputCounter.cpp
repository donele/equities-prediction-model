#include <jl_lib/InputCounter.h>
#include <math.h>
using namespace std;

InputCounter::InputCounter(const std::string& desc, double base)
:desc_(desc),
base_(base),
nInputs_(0),
N_(1),
p_(0),
os_(&cout)
{}

InputCounter::InputCounter(std::ostream &os, const std::string& desc, double base)
:desc_(desc),
base_(base),
nInputs_(0),
N_(1),
p_(0),
os_(&os)
{}

InputCounter::~InputCounter()
{
	print();
}

void InputCounter::operator++()
{
	if( ++nInputs_ >= N_ )
	{
		print();
		N_ = pow(base_, ++p_);
	}
	return;
}

void InputCounter::print()
{
	(*os_) << desc_ << " " << (int)N_ << endl;
	return;
}
