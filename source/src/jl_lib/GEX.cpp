#include <jl_lib/GEX.h>
#include <jl_lib/mto.h>
#include "optionlibs/TickData.h"
#include <string>
#include <vector>
using namespace std;

GEX* GEX::instance_ = 0;

GEX* GEX::Instance()
{
	static GEX::Cleaner cleaner;
	if( instance_ == 0 )
		instance_ = new GEX();
	return instance_;
}

GEX::Cleaner::~Cleaner() {
	for( map<string, Exchange*>::iterator it = GEX::Instance()->m_.begin(); it != GEX::Instance()->m_.end(); ++it )
		delete it->second;
	delete GEX::instance_;
	GEX::instance_ = 0;
}

GEX::GEX()
{}

Exchange* GEX::get(const string& market)
{
	string ex = mto::ex(market);
	if( !m_.count(ex) )
		m_[ex] = new Exchange(ex);

	return m_[ex];
}
