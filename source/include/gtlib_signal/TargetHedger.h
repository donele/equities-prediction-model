#ifndef __gtlib_signal_TargetHedger__
#define __gtlib_signal_TargetHedger__

namespace gtlib {

class TargetHedger {
public:
	TargetHedger();
	virtual double getHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const = 0;
	virtual double getHedgedTarget11Close(double unHedged, int sec) const {return 0.;}
	virtual double getHedgedTarget71Close(double unHedged, int sec) const {return 0.;}
	virtual double getHedgedTargetClose(double unHedged, int sec) const {return 0.;}
	virtual double getHedgedTarON(double unHedged) const {return 0.;}
	virtual double getHedgedTarClcl(double unHedged) const {return 0.;}
	virtual double getMultHedgedTarget(double unHedged, int sec, int iT, int len, int lag) const {return 0.;}
	virtual double getNorthBP() const = 0;
	virtual double getNorthTRD() const = 0;
	virtual double getNorthRST() const = 0;
};

} // namespace gtlib

#endif
