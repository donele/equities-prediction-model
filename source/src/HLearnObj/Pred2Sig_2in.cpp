#include <HLearnObj/Pred2Sig_2in.h>
#include <HLearnObj/InputInfo.h>
#include <HLib/HEvent.h>
#include <math.h>
#include <iostream>
#include <fstream>
using namespace std;

namespace hnn {
	Pred2Sig_2in::Pred2Sig_2in()
	{
		get_iPred();
	}

	Pred2Sig_2in::Pred2Sig_2in(const Pred2Sig_2in & rhs)
	:NNBase(rhs)
	{
		get_iPred();
	}

	Pred2Sig_2in::Pred2Sig_2in(vector<int>& N, double learn, double wgtDecay, double maxWgt, int verbose, double biasOut/*, double costMult*/)
	:NNBase(N, learn, wgtDecay, maxWgt, verbose, biasOut, false)
	{
		get_iPred();
	}

	Pred2Sig_2in::~Pred2Sig_2in()
	{}

	void Pred2Sig_2in::get_iPred()
	{
		const vector<string>* pin = static_cast<const vector<string>*>(HEvent::Instance()->get("", "inputNames"));
		int i = 0;
		if( pin != 0 )
		{
			for( vector<string>::const_iterator it = pin->begin(); it != pin->end(); ++it, ++i )
			{
				string name = *it;
				if( "pred41m" == name )
					iPred41m_ = i;
			}
		}
	}

	vector<double> Pred2Sig_2in::train(const vector<float>& input, const QuoteInfo& quote, const double target)
	{
		vector<double> output;
		return output;
	}

	vector<double> Pred2Sig_2in::trading_signal(const vector<float>& input, const QuoteInfo& quote, const double lastPos)
	{
		double pred41m = input[iPred41m_];

		//const hnn::InputInfo* pnii = static_cast<const hnn::InputInfo*>(HEvent::Instance()->get("", "input_info"));
		//pred41m = pred41m * pnii->stdev[iPred41m_];

		double pred = pred41m / 10000.;
		vector<double> output = vector<double>(1, pred);
		return output;
	}

	std::ostream& Pred2Sig_2in::write(ostream& str) const
	{
		return str;
	}

	std::istream& Pred2Sig_2in::read(istream& str)
	{
		return str;
	}

	vector<double> Pred2Sig_2in::forward(const vector<float>& input, const QuoteInfo& quote)
	{
		vector<double> output;
		return output;
	}

	void Pred2Sig_2in::backprop(double error)
	{
		return;
	}
}