#include <TGFrame.h> 
 
class TGWindow; 
class TRootEmbeddedCanvas; 
 
class MyMainFrame : public TGMainFrame { 
	private: 
		TRootEmbeddedCanvas *fEcanvas; 
	public: 
		MyMainFrame(const TGWindow *p,UInt_t w,UInt_t h); 
		virtual ~MyMainFrame(); 
		void DoDraw(); 
		ClassDef(MyMainFrame, 0)
};
