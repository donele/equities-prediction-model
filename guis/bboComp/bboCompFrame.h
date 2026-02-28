#include <string>
#include <vector>
#include <map>
#include <TStyle.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TRootEmbeddedCanvas.h>
#include <RQ_OBJECT.h>

////////////////////////////////////////////////////////////////////////////////
// Class bboCompFrame
////////////////////////////////////////////////////////////////////////////////
class bboCompFrame {
  RQ_OBJECT("bboCompFrame")

public:
  bboCompFrame();
  bboCompFrame(const TGWindow *p, UInt_t w, UInt_t h);
  virtual ~bboCompFrame();

  void draw();
  void draw_next();
  void draw_before();
  void draw_dist();
  void draw_diff();
  void draw_diffStat();

private:
  TGMainFrame *fMain;
  TRootEmbeddedCanvas *fEcanvas;

  TGComboBox *fComboMarket1;
  TGComboBox *fComboDate1;
  TGComboBox *fComboName1;
  TGButtonGroup *fBG1;
  TGButtonGroup *fBG2;
  TGRadioButton* fROrder[2];
  TGRadioButton* fRPlot[3];

  int nvars;
  int ndays;
  int nmarkets;
  int fNameAlphSize;
  int fNameLiqdSize;

  TH1F* h00;
  TH1F* h10;
  TH2F* h20;
  TH2F* h30;

  TGraph* g00;
  TGraph* g01;
  TGraph* g10;
  TGraph* g11;
  TGraph* g20;
  TGraph* g21;
  TGraph* g30;
  TGraph* g31;

  TString market;
  TString marketCode;
  TString path;
  TString fDesc1;
  TString fDesc2;
  TString* fName1;
  TString* fNameAlph;
  TString* fNameLiqd;
  TString* fDate1;
  vector<TString> fMarket1;
  vector<TString> fMarketCode1;
  vector<TString> fPath1;

  void update_symbol_list(int iday);
  void update_symbol_box(int iord);
  void update_date_box(int iday, int iord);
  void print_corr( TString subtitle, TGraph* gr1, TGraph* gr2 );
  void fill_2d( TH2F* hist, TGraph* gr1, TGraph* gr2, bool takeLog );
};

bboCompFrame::bboCompFrame(){}

bboCompFrame::bboCompFrame(const TGWindow *p, UInt_t w, UInt_t h)
:nmarkets(0)
{
  gROOT->SetStyle("Plain");
  gStyle->SetOptStat(0);
  gStyle->SetPadTopMargin(0.2);
  gStyle->SetPadBottomMargin(0.16);
  gStyle->SetPadLeftMargin(0.12);
  gStyle->SetTitleH(0.14);

// Initialize the class

  InitVars();

// ----------------------------------------------------------------
// Part I
// ----------------------------------------------------------------

// Create a main frame
  fMain = new TGMainFrame(p,w,h);

// Create a canvas widget
  fEcanvas = new TRootEmbeddedCanvas("Ecanvas",fMain,900,800);
  fMain->AddFrame(fEcanvas, new TGLayoutHints(kLHintsExpandX| kLHintsExpandY,10,10,10,1));

// ----------------------------------------------------------------
// hframe1
// ----------------------------------------------------------------

// Begin hframe
  TGHorizontalFrame *hframe1 = new TGHorizontalFrame(fMain,900,600);

// Draw Button Symbol
  TGTextButton *bDraw = new TGTextButton(hframe1," Draw ");
  bDraw->Connect("Clicked()","bboCompFrame",this,"draw()");

// Draw Next Button Symbol
  TGTextButton *bDrawNext = new TGTextButton(hframe1," Next > ");
  bDrawNext->Connect("Clicked()","bboCompFrame",this,"draw_next()");

// Draw Before Button Symbol
  TGTextButton *bDrawBefore = new TGTextButton(hframe1," < Back ");
  bDrawBefore->Connect("Clicked()","bboCompFrame",this,"draw_before()");

// Combobox Symbol
  fComboName1 = new TGComboBox(hframe1,50);
  fComboName1->Resize(140,20);
  fComboName1->Select(1);

// Combobox date
  fComboDate1 = new TGComboBox(hframe1,50);
  fComboDate1->Resize(120,20);
  fComboDate1->Connect("Selected(Int_t)","bboCompFrame",this,"update_symbol_list(Int_t)");

// Combobox market
  fComboMarket1 = new TGComboBox(hframe1,50);
  for( int i=0; i<nmarkets; ++i )
    fComboMarket1->AddEntry(fMarket1[i].Data(), i+1);
  fComboMarket1->Resize(120,20);
  fComboMarket1->Connect("Selected(Int_t)","bboCompFrame",this,"update_date_box(Int_t)");

// Exit Button
  TGTextButton *exit = new TGTextButton(hframe1,"&Exit","gApplication->Terminate(0)");

// End hframe1
  hframe1->AddFrame(fComboMarket1, new TGLayoutHints(kLHintsCenterX,5,5,5,5));
  hframe1->AddFrame(fComboDate1, new TGLayoutHints(kLHintsCenterX,5,5,5,5));
  hframe1->AddFrame(fComboName1, new TGLayoutHints(kLHintsCenterX,5,5,5,5));
  hframe1->AddFrame(bDrawBefore, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  hframe1->AddFrame(bDraw, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  hframe1->AddFrame(bDrawNext, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  hframe1->AddFrame(exit, new TGLayoutHints(kLHintsCenterX,25,5,3,4));
  fMain->AddFrame(hframe1, new TGLayoutHints(kLHintsCenterX,2,2,2,2));

// ----------------------------------------------------------------
// hframe2
// ----------------------------------------------------------------

// Begin hframe
  TGHorizontalFrame *hframe2 = new TGHorizontalFrame(fMain,900,600);

// Radio Buttons order by
  fBG1 = new TGButtonGroup(hframe2, "Tickers Order by", kHorizontalFrame);
  fROrder[0] = new TGRadioButton(fBG1, new TGHotString("&Alph"));
  fROrder[1] = new TGRadioButton(fBG1, new TGHotString("&Liqd"));
  fROrder[0]->SetState(kButtonDown);
  fBG1->SetLayoutHints(new TGLayoutHints(kLHintsCenterX,5,5,1,1));
  fBG1->Connect("Clicked(Int_t)", "bboCompFrame", this, "update_symbol_box(Int_t)");

// Radio Buttons plot
  fBG2 = new TGButtonGroup(hframe2, "Plot", kHorizontalFrame);
  fRPlot[0] = new TGRadioButton(fBG2, new TGHotString("&dist"));
  fRPlot[1] = new TGRadioButton(fBG2, new TGHotString("&diff"));
  fRPlot[2] = new TGRadioButton(fBG2, new TGHotString("diff &stat"));
  fRPlot[0]->SetState(kButtonDown);
  fBG2->SetLayoutHints(new TGLayoutHints(kLHintsCenterX,5,5,1,1));

// End hframe2
  hframe2->AddFrame(fBG1, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  hframe2->AddFrame(fBG2, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  fMain->AddFrame(hframe2, new TGLayoutHints(kLHintsCenterX,2,2,2,2));

//
// Select
//
  //fComboDate1->Select(ndays);
  //fComboMarket1->Select(nmarkets);

//
// Main Frame...
//
  fMain->SetWindowName("BBO Test");
  fMain->MapSubwindows();
  fMain->Resize(fMain->GetDefaultSize());
  fMain->MapWindow();
}

void bboCompFrame::InitVars()
{
  fDesc1 = "Series1";
  fDesc2 = "Series2";

  fDate1 = new TString[2];
  fNameAlph = new TString[2];
  fNameLiqd = new TString[2];

  ifstream ifs("bboComp.conf");
  TString feed = "";
  TString city = "";
  TString market = "";
  TString dir = "";

  while( ifs >> feed >> city >> market >> dir )
  {
    ++nmarkets;
    fMarket1.push_back(feed + " " + city);
    fMarketCode1.push_back(market);
    fPath1.push_back("\\\\smrc-ltc-mrct16\\data00\\tickMon\\bboComp\\" + dir);
  }

  h00 = new TH1F();
  h10 = new TH1F();
  h20 = new TH2F();
  h30 = new TH2F();

  g00 = new TGraph();
  g01 = new TGraph();
  g10 = new TGraph();
  g11 = new TGraph();
  g20 = new TGraph();
  g21 = new TGraph();
  g30 = new TGraph();
  g31 = new TGraph();
}

void bboCompFrame::draw()
{
  if( fBG2->GetButton(1)->IsOn() )
    draw_dist();
  else if( fBG2->GetButton(2)->IsOn() )
    draw_diff();
  else if( fBG2->GetButton(3)->IsOn() )
    draw_diffStat();

  return;
}

void bboCompFrame::draw_next()
{
  int nEntries = fComboName1->GetNumberOfEntries();
  int newName = min(fComboName1->GetSelectedEntry()->EntryId() + 1, nEntries);
  fComboName1->Select(newName);

  draw();

  return;
}

void bboCompFrame::draw_before()
{
  int newName = max(1, fComboName1->GetSelectedEntry()->EntryId() - 1);
  fComboName1->Select(newName);

  draw();

  return;
}

void bboCompFrame::draw_dist()
{
  TCanvas *fCanvas1 = fEcanvas->GetCanvas();
  fCanvas1->Clear();
  fCanvas1->Divide(2,2);
  fCanvas1->Update();

  int iday = fComboDate1->GetSelectedEntry()->EntryId() - 1;
  int iname = fComboName1->GetSelectedEntry()->EntryId() - 1;
  TString ticker = fName1[iname];

  TFile *f = new TFile( path + marketCode + "_" + fDate1[iday] + ".root" );
  f->cd(ticker);
  TString subtitle = ", " + marketCode + " " + fDate1[iday] + " " + ticker;

  TGraph* gr1B_ = (TGraph*)gDirectory->Get("gr1B");
  TGraph* gr1A_ = (TGraph*)gDirectory->Get("gr1A");
  TGraph* gr2B_ = (TGraph*)gDirectory->Get("gr2B");
  TGraph* gr2A_ = (TGraph*)gDirectory->Get("gr2A");
  TGraph* gr1S_ = (TGraph*)gDirectory->Get("gr1S");
  TGraph* gr2S_ = (TGraph*)gDirectory->Get("gr2S");
  if( gr1B_ != 0 && gr1A_ != 0 && gr2B_ != 0 && gr2A_ != 0 && gr1S_ != 0 && gr2S_ != 0 )
  {
    if( g00 != 0 )
      delete g00;
    if( g01 != 0 )
      delete g01;
    if( g10 != 0 )
      delete g10;
    if( g11 != 0 )
      delete g11;
    if( g20 != 0 )
      delete g20;
    if( g30 != 0 )
      delete g30;

    gr00 = (TGraph*)gr1B_->Clone();
    gr01 = (TGraph*)gr1A_->Clone();
    gr10 = (TGraph*)gr2B_->Clone();
    gr11 = (TGraph*)gr2A_->Clone();
    gr20 = (TGraph*)gr1S_->Clone();
    gr30 = (TGraph*)gr2S_->Clone();

    delete gr1B_;
    delete gr1A_;
    delete gr2B_;
    delete gr2A_;
    delete gr1S_;
    delete gr2S_;

    TCanvas *fCanvas1 = fEcanvas->GetCanvas();

    fCanvas1->cd(1);
    gr00->SetTitle( fDesc1 + " Price" + subtitle );
    gr00->SetLineColor(2);
    gr01->SetLineColor(4);
    gr00->Draw("al");
    gr01->Draw("l");
    TLegend* leg1 = new TLegend(0.75,0.8,0.95,0.93);
    leg1->AddEntry(gr00, "Bid", "l");
    leg1->AddEntry(gr01, "Ask", "l");
    leg1->Draw();

    fCanvas1->cd(2);
    gr10->SetTitle( fDesc2 + " Price" + subtitle );
    gr10->SetLineColor(2);
    gr11->SetLineColor(4);
    gr10->Draw("al");
    gr11->Draw("l");
    TLegend* leg2 = new TLegend(0.75,0.8,0.95,0.93);
    leg2->AddEntry(gr10, "Bid", "l");
    leg2->AddEntry(gr11, "Ask", "l");
    leg2->Draw();

    fCanvas1->cd(3);
    gr20->SetTitle( fDesc1 + " Spread" + subtitle );
    gr20->Draw("apl");

    fCanvas1->cd(4);
    gr30->SetTitle( fDesc2 + " Spread" + subtitle );
    gr30->Draw("apl");

    fCanvas1->cd();
    fCanvas1->Update();
  }
  f->Close();

  return;
}

void bboCompFrame::draw_diffStat()
{
  gStyle->SetOptStat(111110);

  TCanvas *fCanvas1 = fEcanvas->GetCanvas();
  fCanvas1->Clear();
  fCanvas1->Divide(2,2);
  fCanvas1->Update();

  int iday = fComboDate1->GetSelectedEntry()->EntryId() - 1;
  int iname = fComboName1->GetSelectedEntry()->EntryId() - 1;
  TString ticker = fName1[iname];

  TFile *f = new TFile( path + marketCode + "_" + fDate1[iday] + ".root" );
  f->cd(ticker);
  TString subtitle = ", " + marketCode + " " + fDate1[iday] + " " + ticker;

  TGraph* grDB_ = (TGraph*)gDirectory->Get("grDB");
  TGraph* grDA_ = (TGraph*)gDirectory->Get("grDA");
  TGraph* grDBS_ = (TGraph*)gDirectory->Get("grDBS");
  TGraph* grDAS_ = (TGraph*)gDirectory->Get("grDAS");
  TGraph* gr1B_ = (TGraph*)gDirectory->Get("gr1B");
  TGraph* gr1A_ = (TGraph*)gDirectory->Get("gr1A");
  TGraph* gr2B_ = (TGraph*)gDirectory->Get("gr2B");
  TGraph* gr2A_ = (TGraph*)gDirectory->Get("gr2A");
  TGraph* gr1BS_ = (TGraph*)gDirectory->Get("gr1BS");
  TGraph* gr1AS_ = (TGraph*)gDirectory->Get("gr1AS");
  TGraph* gr2BS_ = (TGraph*)gDirectory->Get("gr2BS");
  TGraph* gr2AS_ = (TGraph*)gDirectory->Get("gr2AS");
  if( grDB_ != 0 && grDA_ != 0 && grDBS_ != 0 && grDAS_ != 0 &&
      gr1B_ != 0 && gr1A_ != 0 && gr2B_ != 0 && gr2A_ != 0 &&
      gr1BS_ != 0 && gr1AS_ != 0 && gr2BS_ != 0 && gr2AS_ != 0 )
  {
    int nb = grDB_->GetN();
    int na = grDA_->GetN();
    int nbs = grDBS_->GetN();
    int nas = grDAS_->GetN();
    if( nb > 2 && na > 2 && nbs > 2 && nas > 2 )
    {
      if( h00 != 0 )
        delete h00;
      if( h10 != 0 )
        delete h10;
      if( h20 != 0 )
        delete h20;
      if( h30 != 0 )
        delete h30;

      double price = 0;
      {
        int n = gr1B_->GetN();
        double* y = gr1B_->GetY();
        price = y[n/2]/2.0;

        int m = gr1A_->GetN();
        double* z = gr1A_->GetY();
        price += z[m/2]/2.0;
      }

      TString title0 = "#Delta Best Price" + subtitle;
      TString title1 = "#Delta Best Size" + subtitle;
      TString title2 = "Price";
      TString title3 = "Size";
      h00 = new TH1F("h00", title0, 1000, -0.5, 0.5);
      h10 = new TH1F("h10", title1, 1000, -5000, 5000);
      h20 = new TH2F("h20", title2, 100, 0.98*price, 1.03*price, 100, 0.98*price, 1.03*price);
      h30 = new TH2F("h30", title3, 100, 0, 16, 100, 0, 16);
      h00->SetDirectory(0);
      h10->SetDirectory(0);
      h20->SetDirectory(0);
      h30->SetDirectory(0);

      double *xb = grDB_->GetX();
      double *yb = grDB_->GetY();
      for( int i=0; i<nb; ++i )
      {
        h00->Fill(yb[i]);
        //if( yb[i] != 0 )
          //cout << xb[i] << "\t" << yb[i] << endl;
      }

      double *ya = grDA_->GetY();
      for( int i=0; i<na; ++i )
        h00->Fill(ya[i]);

      double *ybs = grDBS_->GetY();
      for( int i=0; i<nbs; ++i )
        h10->Fill(ybs[i]);

      double *yas = grDAS_->GetY();
      for( int i=0; i<nas; ++i )
        h10->Fill(yas[i]);

      fill_2d( h20, gr1B_, gr2B_, false );
      fill_2d( h20, gr1A_, gr2A_, false );
      fill_2d( h30, gr1BS_, gr2BS_, true );
      fill_2d( h30, gr1AS_, gr2AS_, true );

      TCanvas *fCanvas1 = fEcanvas->GetCanvas();

      fCanvas1->cd(1);
      gPad->SetLogy();
      TPaveText *pt1 = new TPaveText(0.6,0.6,0.9,0.7,"NDC");
      pt1->AddText("100 sec Sampled");
      char stat1[100];
      sprintf( stat1, "%.2f correct", (double)(h00->GetBinContent(h00->GetNbinsX()/2+1))/h00->GetEntries() );
      pt1->AddText(stat1);
      pt1->SetBorderSize(0);
      pt1->SetFillColor(0);
      h00->Draw();
      pt1->Draw();

      fCanvas1->cd(2);
      gPad->SetLogy();
      TPaveText *pt2 = new TPaveText(0.6,0.6,0.9,0.7,"NDC");
      pt2->AddText("100 sec Sampled");
      char stat2[100];
      sprintf( stat2, "%.2f correct", (double)(h10->GetBinContent(h10->GetNbinsX()/2+1))/h10->GetEntries() );
      pt2->AddText(stat2);
      pt2->SetBorderSize(0);
      pt2->SetFillColor(0);
      h10->Draw();
      pt2->Draw();

      fCanvas1->cd(3);
      double c20 = h20->GetCorrelationFactor();
      char t2[200];
      sprintf( t2, "%s, Corr = %6.4f", title2.Data(), c20 );
      h20->SetTitle(t2);
      h20->Draw();

      fCanvas1->cd(4);
      double c30 = h30->GetCorrelationFactor();
      char t3[200];
      sprintf( t3, "%s, Corr = %4.2f", title3.Data(), c30 );
      h30->SetTitle(t3);
      h30->Draw();

      fCanvas1->cd();
      fCanvas1->Update();
    }
  }

  f->Close();

  gStyle->SetOptStat(0);
  return;
}

void bboCompFrame::draw_diff()
{
  TCanvas *fCanvas1 = fEcanvas->GetCanvas();
  fCanvas1->Clear();
  fCanvas1->Divide(2,2);
  fCanvas1->Update();

  int iday = fComboDate1->GetSelectedEntry()->EntryId() - 1;
  int iname = fComboName1->GetSelectedEntry()->EntryId() - 1;
  TString ticker = fName1[iname];

  TFile *f = new TFile( path + marketCode + "_" + fDate1[iday] + ".root" );
  f->cd(ticker);
  TString subtitle = ", " + marketCode + " " + fDate1[iday] + " " + ticker;

  TGraph* gr1B2_ = (TGraph*)gDirectory->Get("gr1B2");
  TGraph* gr2B2_ = (TGraph*)gDirectory->Get("gr2B2");
  TGraph* gr1A2_ = (TGraph*)gDirectory->Get("gr1A2");
  TGraph* gr2A2_ = (TGraph*)gDirectory->Get("gr2A2");
  TGraph* gr1BS_ = (TGraph*)gDirectory->Get("gr1BS");
  TGraph* gr2BS_ = (TGraph*)gDirectory->Get("gr2BS");
  TGraph* gr1AS_ = (TGraph*)gDirectory->Get("gr1AS");
  TGraph* gr2AS_ = (TGraph*)gDirectory->Get("gr2AS");
  if( gr1B2_ != 0 && gr2B2_ != 0 && gr1A2_ != 0 && gr2A2_ != 0 &&
      gr1BS_ != 0 && gr2BS_ != 0 && gr1AS_ != 0 && gr2AS_ != 0 )
  {
    if( h00 != 0 )
      delete h00;
    if( g01 != 0 )
      delete g01;
    if( g10 != 0 )
      delete g10;
    if( g11 != 0 )
      delete g11;
    if( g20 != 0 )
      delete g20;
    if( g21 != 0 )
      delete g21;
    if( g30 != 0 )
      delete g30;
    if( g31 != 0 )
      delete g31;

    gr00 = (TGraph*)gr1B2_->Clone();
    gr01 = (TGraph*)gr2B2_->Clone();
    gr10 = (TGraph*)gr1A2_->Clone();
    gr11 = (TGraph*)gr2A2_->Clone();
    gr20 = (TGraph*)gr1BS_->Clone();
    gr21 = (TGraph*)gr2BS_->Clone();
    gr30 = (TGraph*)gr1AS_->Clone();
    gr31 = (TGraph*)gr2AS_->Clone();

    delete gr1B2_;
    delete gr2B2_;
    delete gr1A2_;
    delete gr2A2_;
    delete gr1BS_;
    delete gr2BS_;
    delete gr1AS_;
    delete gr2AS_;

    TCanvas *fCanvas1 = fEcanvas->GetCanvas();

    fCanvas1->cd(1);
    gr00->SetTitle( "Bid Price" + subtitle );
    gr00->SetLineColor(2);
    gr01->SetLineColor(4);
    gr00->Draw("al");
    gr01->Draw("l");
    TLegend* leg1 = new TLegend(0.75,0.8,0.95,0.93);
    leg1->AddEntry(gr00, fDesc1, "l");
    leg1->AddEntry(gr01, fDesc2, "l");
    leg1->Draw();

    fCanvas1->cd(2);
    gr10->SetTitle( "Ask Price" + subtitle );
    gr10->SetLineColor(2);
    gr11->SetLineColor(4);
    gr10->Draw("al");
    gr11->Draw("l");
    TLegend* leg2 = new TLegend(0.75,0.8,0.95,0.93);
    leg2->AddEntry(gr10, fDesc1, "l");
    leg2->AddEntry(gr11, fDesc2, "l");
    leg2->Draw();

    fCanvas1->cd(3);
    gr20->SetTitle( "Bid Size" + subtitle );
    gr20->SetLineColor(2);
    gr21->SetLineColor(4);
    gr20->Draw("al");
    gr21->Draw("l");
    TLegend* leg3 = new TLegend(0.75,0.8,0.95,0.93);
    leg3->AddEntry(gr20, fDesc1, "l");
    leg3->AddEntry(gr21, fDesc2, "l");
    leg3->Draw();

    fCanvas1->cd(4);
    gr30->SetTitle( "Ask Size" + subtitle );
    gr30->SetLineColor(2);
    gr31->SetLineColor(4);
    gr30->Draw("al");
    gr31->Draw("l");
    TLegend* leg4 = new TLegend(0.75,0.8,0.95,0.93);
    leg4->AddEntry(gr30, fDesc1, "l");
    leg4->AddEntry(gr31, fDesc2, "l");
    leg4->Draw();

    fCanvas1->cd();
    fCanvas1->Update();
  }
  f->Close();

  return;
}

void bboCompFrame::update_symbol_list(int iday)
{
  vector<string> temp_name;
  map<int, int> m_nq_isym;

  TFile *f = new TFile( path + marketCode + "_" + fDate1[iday-1] + ".root" );
  TObject *obj;
  TKey *key;
  TIter next( gDirectory->GetListOfKeys() );
  int isym = 0;
  int headerLen = 7;
  while( (key = (TKey*)next()) )
  {
    string name = key->GetName();
    if( !name.empty() && ((name[0] >= 'A' && name[0] <= 'Z') || (name[0] >= '0' && name[0] <= '9')) )
    {
      temp_name.push_back( name );

      f->cd(name.c_str());
      int nq = ((TGraph*)(gDirectory->Get("gr1B")))->GetN();
      m_nq_isym[nq] = isym++;
    }
  }
  f->Close();

  if( !temp_name.empty() )
  {
    delete [] fNameAlph;
    delete [] fNameLiqd;

    fNameAlphSize = temp_name.size();
    fNameAlph = new TString[fNameAlphSize];
    for( int i=0; i<fNameAlphSize; ++i )
      fNameAlph[i] = temp_name[i].c_str();

    fNameLiqdSize = m_nq_isym.size();
    fNameLiqd = new TString[fNameLiqdSize];
    int x = 0;
    for(map<int,int>::reverse_iterator it = m_nq_isym.rbegin(); it != m_nq_isym.rend() && x < fNameLiqdSize; ++it, ++x )
      fNameLiqd[x] = temp_name[it->second].c_str();
  }
  int iord = 1;
  if( fBG1->GetButton(1)->IsOn() )
  {
    iord = 1;
    fName1 = fNameAlph;
  }
  else if( fBG1->GetButton(2)->IsOn() )
  {
    iord = 2;
    fName1 = fNameLiqd;
  }

  update_symbol_box(iord);

  return;
}

void bboCompFrame::update_symbol_box(int iord)
{
  fComboName1->RemoveAll();

  if( iord == 1 )
  {
    fName1 = fNameAlph;
    for( int i=0; i<fNameAlphSize; ++i )
      fComboName1->AddEntry( fName1[i], i+1 );
  }
  else if( iord == 2 )
  {
    fName1 = fNameLiqd;
    for( int i=0; i<fNameLiqdSize; ++i )
      fComboName1->AddEntry( fName1[i], i+1 );
  }

  fComboName1->Resize(140,20);
  if( fComboName1->GetNumberOfEntries() > 0 )
    fComboName1->Select(1);

  return;
}

void bboCompFrame::update_date_box(int id)
{
  market = fMarket1[id-1];
  marketCode = fMarketCode1[id-1];
  path = fPath1[id-1];

  char cmd[100];
  sprintf( cmd, "get_date_list.py %s %s", marketCode.Data(), path.Data() );
  system( cmd );

  vector<string> temp_date;
  ifstream ifs( "temp_dates.txt" );
  if( ifs.is_open() )
  {
    ifs >> ndays;
    string date;
    for( int i=0; i<ndays; ++i )
    {
      ifs >> date;
      temp_date.push_back(date);
    }
    ifs.close();
    //system( "del temp_dates.txt" );
  }

  fComboDate1->RemoveAll();
  if( ndays > 0 )
  {
    delete [] fDate1;
    fDate1 = new TString[ndays];
    for( int i=0; i<ndays; ++i )
    {
      fDate1[i] = temp_date[i].c_str();
      fComboDate1->AddEntry( fDate1[i], i+1 );
    }
  }
  else
  {
    delete [] fDate1;
    fDate1 = new TString[2];
    for( int i=0; i<2; ++i )
    {
      fDate1[i] = 0;
      fComboDate1->AddEntry( fDate1[i], i+1 );
    }
  }

  fComboDate1->Resize(100,20);
  //if( fComboDate1->GetNumberOfEntries() > 0 )
    //fComboDate1->Select(ndays);

  return;
}

bboCompFrame::~bboCompFrame() {
  fMain->Cleanup();
  delete fMain;
}

void bboCompFrame::print_corr( TString subtitle, TGraph* gr1, TGraph* gr2 )
{
  TH2F h2("h2", "h2", 100,0,16, 100,0,16);

  int N1 = gr1->GetN();
  int N2 = gr2->GetN();
  double* x1 = gr1->GetX();
  double* x2 = gr2->GetX();
  double* y1 = gr1->GetY();
  double* y2 = gr2->GetY();
  int n1 = 0;
  int n2 = 0;
  while( n1 < N1 && n2 < N2 )
  {
    while( n1 < N1 && n2 < N2 && x1[n1] < x2[n2] )
    {
      ++n1;
      double v1 = y1[n1];
      double v2 = y2[n2];
      if( v1 >= 1 && v1 < 100000 && v2 >= 1 && v2 < 100000 && x1[n1] == x2[n2] )
      {
        h2.Fill(log(v1), log(v2));
      }
    }
    while( n1 < N1 && n2 < N2 && x1[n1] > x2[n2] )
    {
      ++n2;
      double v1 = y1[n1];
      double v2 = y2[n2];
      if( v1 >= 1 && v1 < 100000 && v2 >= 1 && v2 < 100000 && x1[n1] == x2[n2] )
        h2.Fill(log(v1), log(v2));
    }
    ++n1;
    ++n2;
  }

  double corr = h2.GetCorrelationFactor();
  cout << subtitle << " " << corr << endl;
  return;
}

void bboCompFrame::fill_2d( TH2F* hist, TGraph* gr1, TGraph* gr2, bool takeLog )
{
  int N1 = gr1->GetN();
  int N2 = gr2->GetN();

  double* x1 = gr1->GetX();
  double* x2 = gr2->GetX();
  double* y1 = gr1->GetY();
  double* y2 = gr2->GetY();

  for( int secs = min(x1[0], x2[0]); secs < min(x1[N1-1], x2[N2-1]); secs += 100 )
  {
    int n1 = -1;
    for( int i = 1; i < N1; ++i )
    {
      if( x1[i] > secs )
      {
        n1 = --i;
        break;
      }
    }

    int n2 = -1;
    for( int i = 1; i < N2; ++i )
    {
      if( x2[i] > secs )
      {
        n2 = --i;
        break;
      }
    }

    if( n1 >= 0 && n2 >= 0 )
    {
      double v1 = y1[n1];
      double v2 = y2[n2];
      if( v1 > 0 && v1 < 100000000 && v2 > 0 && v2 < 100000000 )
      {
        if( takeLog )
          hist->Fill(log(v1), log(v2));
        else
          hist->Fill(v1, v2);
      }
    }
  }

  return;
}
