#include <jl_lib/GFee.h>
#include <jl_lib/mto.h>
#include <jl_lib/jlutil.h>
#include <string>
#include <vector>
using namespace std;

GFee& GFee::Instance()
{
	static GFee instance;
	return instance;
}

GFee::GFee()
{}

float GFee::feeBpt(const string& market, char primex, double price)
{
	return feeBpt(market, primex, price, price);
}

float GFee::feeBpt(const string& market, char primex, double bid, double ask)
{
	QuoteInfo nbbo;
	nbbo.bidEx = primex;
	nbbo.bidSize = 100;
	nbbo.bid = bid;
	nbbo.ask = ask;
	nbbo.askSize = 100;
	nbbo.askEx = primex;
	double bidFeesBpt = 0.;
	double askFeesBpt = 0.;
	ExecFees** fees = getFees(getLoc(market));
	CalcNbboFees(fees, primex, nbbo, &bidFeesBpt, &askFeesBpt);
	return basis_pts_ * (bidFeesBpt + askFeesBpt) / 2.;
}

vector<float> GFee::feeBptBidAsk(const string& market, char primex, QuoteInfo nbbo)
{
	double bidFeesBpt = 0.;
	double askFeesBpt = 0.;
	ExecFees** fees = getFees(getLoc(market));
	CalcNbboFees(fees, primex, nbbo, &bidFeesBpt, &askFeesBpt);
	return {basis_pts_ * bidFeesBpt, basis_pts_ * askFeesBpt};
}

ExecFees** GFee::getFees(const string& loc)
{
	mutex_.lock();
	if(!mfees_.count(loc))
	{
		mfees_[loc] == nullptr;
		if(loc == "US")
			InitFeesUS(&mfees_[loc]);
		else if(loc == "CA")
			InitFeesCA(&mfees_[loc]);
		else if(loc == "EU")
			InitFeesEU(&mfees_[loc]);
		else if(loc == "Asia")
			InitFeesAsia(&mfees_[loc]);
	}
	mutex_.unlock();
	return mfees_[loc];
}

string GFee::getLoc(const string& market)
{
	string loc;
	if(market[0] == 'U')
		loc = "US";
	else if(market[0] == 'E')
		loc = "EU";
	else if(market[0] == 'C')
		loc = "CA";
	else if(market[0] == 'A' || market.substr(0,2) == "KR")
		loc = "Asia";
	return loc;
}
