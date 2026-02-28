#include <HOrders/HaltSched.h>
using namespace std;

void HaltSched::addHalt(int msecs)
{
	vStopMsecs_.push_back(msecs);
}

bool HaltSched::isHalted(int msecs)
{
	for( int stopMsecs : vStopMsecs_ )
	{
		if( msecs >= stopMsecs && msecs < stopMsecs + haltLenSec_ * 1000 )
			return true;
	}
	return false;
}

int HaltSched::getNhalts()
{
	return vStopMsecs_.size();
}
