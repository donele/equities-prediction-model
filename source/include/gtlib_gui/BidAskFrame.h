#ifndef __BidAskFrame__
#define __BidAskFrame__
#include <vector>
#include <TStyle.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TGButton.h>
#include <TGButtonGroup.h>
#include <TGFrame.h>
#include <TGComboBox.h>
#include <TRootEmbeddedCanvas.h>

////////////////////////////////////////////////////////////////////////////////
// Class BidAskFrame
////////////////////////////////////////////////////////////////////////////////
class BidAskFrame: public TGMainFrame {

public:
  BidAskFrame(const TGWindow *p, UInt_t w, UInt_t h);
  virtual ~BidAskFrame();

//private:
  //TGMainFrame *fMain;
  TRootEmbeddedCanvas *fEcanvasDist;  // for intra-day distribution

  TGComboBox *fComboMarket1;
  TGComboBox *fComboDate1;
  TGComboBox *fComboName1;
  TGButtonGroup *fBG1;
  TGButtonGroup *fBG2;
  TGRadioButton* fROrder[2];
  TGRadioButton* fROrder2[2];

  TObjArray *fPlots;

  int ndays;
  int nmarkets;
  int iname_;

  TString market;
  TString path;
  TString fdate;
  std::vector<TString> fTicker;
  std::vector<TString> fDate;
  std::vector<TString> fMarket;
  std::vector<TString> fMarketCode;
  std::vector<TString> fPath;

  TString* fName1;
  std::vector<TString> fNameAlph;
  std::vector<TString> fNameLiqd;

  void DoDraw();
  void InitVars();
  void draw(int id);
  void draw_next();
  void draw_before();
  void draw_bidask();
  void draw_trade();
  void set_market(int id);
  void set_date(int id);
  void update_symbol_box(int iord);
  void draw2(int iord);
  ClassDef(BidAskFrame, 0)
};

#endif
