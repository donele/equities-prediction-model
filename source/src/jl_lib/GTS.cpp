#include <jl_lib/GTS.h>
#include <jl_lib/mto.h>
#include <string>
#include <vector>
using namespace std;

GTS* GTS::instance_ = 0;

GTS* GTS::Instance()
{
	static GTS::Cleaner cleaner;
	if( instance_ == 0 )
		instance_ = new GTS();
	return instance_;
}

GTS::Cleaner::~Cleaner() {
	for( map<string, TickerSector*>::iterator it = GTS::Instance()->m_.begin(); it != GTS::Instance()->m_.end(); ++it )
		delete it->second;
	delete GTS::instance_;
	GTS::instance_ = 0;
}

GTS::GTS()
{}

TickerSector* GTS::get(const string& market, int idate)
{
	if( !m_.count(market)  )
		m_[market] = new TickerSector(market, idate);
	else if( m_[market]->get_idate() != idate )
	{
		delete m_[market];
		m_[market] = new TickerSector(market, idate);
	}

	return m_[market];
}
