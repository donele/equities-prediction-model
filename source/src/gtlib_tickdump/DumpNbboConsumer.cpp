#include <gtlib_tickdump/DumpNbboConsumer.h>
#include <gtlib_tickdump/DumpFtns.h>
#include <vector>
using namespace std;

namespace gtlib {

DumpNbboConsumer::DumpNbboConsumer(int nPrint)
	:nPrint_(nPrint),
	cnt_(0)
{
}

void DumpNbboConsumer::NbboCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	if( cnt_ == 0 )
		cout << getDumpHeader<QuoteInfo>() << '\n';

	if( cnt_++ < nPrint_ || nPrint_ == 0 )
		cout << getDumpContent(provider->Nbbo()) << '\n';
}

} // namespace gtlib

