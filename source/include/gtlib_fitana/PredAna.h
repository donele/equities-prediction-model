#ifndef __gtlib_fitana_PredAna__
#define __gtlib_fitana_PredAna__

namespace gtlib {

class PredAna {
public:
	PredAna();
	virtual ~PredAna();
	virtual void analyze() = 0;
	virtual void write() = 0;
private:
};

} // namespace gtlib

#endif
