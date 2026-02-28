#include <iostream>
#include <jl_lib/GFee.h>
#include <optionlibs/TickData.h>
using namespace std;

int main() {
	QuoteInfo nbbo;
	nbbo.bidEx = 'Z';
	nbbo.bidSize = 1;
	nbbo.bid = .627;
	nbbo.ask = .629;
	nbbo.askSize = 5857;
	nbbo.askEx = 'G';
	auto fees = GFee::Instance().feeBptBidAsk("EZ", 'Z', nbbo);
	cout << fees[0] << '\t' << fees[1] << endl;
}

