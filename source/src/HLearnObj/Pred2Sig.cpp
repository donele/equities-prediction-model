#include <HLearnObj/Pred2Sig.h>
#include <HLearnObj/InputInfo.h>
#include <HLib/HEvent.h>
#include <math.h>
#include <iostream>
#include <fstream>
using namespace std;

namespace hnn {
	Pred2Sig::Pred2Sig()
	{
		get_iPred();
	}

	Pred2Sig::Pred2Sig(const Pred2Sig & rhs)
	:NNBase(rhs)
	{
		get_iPred();
	}

	Pred2Sig::Pred2Sig(vector<int>& N, double learn, double wgtDecay, double maxWgt, int verbose, double biasOut/*, double costMult*/)
	:NNBase(N, learn, wgtDecay, maxWgt, verbose, biasOut, false)
	{
		get_iPred();
	}

	Pred2Sig::~Pred2Sig()
	{}

	void Pred2Sig::get_iPred()
	{
		const vector<string>* pin = static_cast<const vector<string>*>(HEvent::Instance()->get("", "inputNames"));
		int i = 0;
		if( pin != 0 )
		{
			for( vector<string>::const_iterator it = pin->begin(); it != pin->end(); ++it, ++i )
			{
				string name = *it;
				if( "pred1m" == name )
					iPred1m_ = i;
				else if( "pred40m" == name )
					iPred40m_ = i;
			}
		}
	}

	vector<double> Pred2Sig::train(const vector<float>& input, const QuoteInfo& quote, const double target)
	{
		vector<double> output;
		return output;
	}

	vector<double> Pred2Sig::trading_signal(const vector<float>& input, const QuoteInfo& quote, const double lastPos)
	{
		double pred1m = input[iPred1m_];
		double pred40m = input[iPred40m_];

		const hnn::InputInfo* pnii = static_cast<const hnn::InputInfo*>(HEvent::Instance()->get("", "input_info"));
		pred1m = pred1m * pnii->stdev[iPred1m_] + pnii->mean[iPred1m_];
		pred40m = pred40m * pnii->stdev[iPred40m_] + pnii->mean[iPred40m_];

		double pred = (pred1m + pred40m) / 10000.0;
		vector<double> output = vector<double>(1, pred);
		return output;
	}

	std::ostream& Pred2Sig::write(ostream& str) const
	{
		return str;
	}

	std::istream& Pred2Sig::read(istream& str)
	{
		return str;
	}

	vector<double> Pred2Sig::forward(const vector<float>& input, const QuoteInfo& quote)
	{
		vector<double> output;
		return output;
	}

	void Pred2Sig::backprop(double error)
	{
		return;
	}
}