#include <jl_lib/GTH.h>
#include <jl_lib/mto.h>
#include <string>
#include <vector>
using namespace std;

GTH* GTH::instance_ = 0;

GTH* GTH::Instance()
{
	if( instance_ == 0 )
		instance_ = new GTH();
	return instance_;
}

GTH::Cleaner::~Cleaner() {
	for( map<string, TickerHistory*>::iterator it = GTH::Instance()->m_.begin(); it != GTH::Instance()->m_.end(); ++it )
		delete it->second;
	delete GTH::instance_;
	GTH::instance_ = 0;
}

GTH::GTH()
{}

void GTH::init(const string& market)
{
	if( !m_.count(market) )
	{
		if( "U" == mto::region(market) || "C" == mto::region(market) )
			m_[market] = new TickerHistory(mto::th(market));
		else
			m_[market] = new TickerHistory(mto::th(market), mto::code(market));
	}
}

TickerHistory* GTH::get(const string& market)
{
	if( !m_.count(market) )
	{
		cerr << "GTH::get() Use GTH::init(market) to initialize." << endl;
		exit(6);
	}

	return m_[market];
}

void GTH::renew(const string& market)
{
	if( m_.count(market) )
		delete m_[market];

	if( "U" == mto::region(market) || "C" == mto::region(market) )
		m_[market] = new TickerHistory(mto::th(market));
	else
		m_[market] = new TickerHistory(mto::th(market), mto::code(market));
}
