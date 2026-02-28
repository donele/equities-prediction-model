#include <string>
#include <map>
#include <vector>
#include <TStyle.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TRootEmbeddedCanvas.h>
#include <RQ_OBJECT.h>

////////////////////////////////////////////////////////////////////////////////
// Class BidAskFrame
////////////////////////////////////////////////////////////////////////////////
class BidAskFrame {
  RQ_OBJECT("BidAskFrame")

public:
  BidAskFrame();
  BidAskFrame(const TGWindow *p, UInt_t w, UInt_t h);
  virtual ~BidAskFrame();

private:
  TGMainFrame *fMain;
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
  vector<TString> fTicker;
  vector<TString> fDate;
  vector<TString> fMarket;
  vector<TString> fMarketCode;
  vector<TString> fPath;

  TString* fName1;
  vector<TString> fNameAlph;
  vector<TString> fNameLiqd;

  void draw(int id);
  void draw_next();
  void draw_before();
  void draw_bidask();
  void draw_trade();
  void set_market(int id);
  void set_date(int id);
  void update_symbol_box(int iord);
  void draw2(int iord);
};

BidAskFrame::BidAskFrame(){}

BidAskFrame::BidAskFrame(const TGWindow *p, UInt_t w, UInt_t h)
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

// Create an upper canvas widget
  fEcanvasDist = new TRootEmbeddedCanvas("EcanvasDist",fMain,800,500);
  fMain->AddFrame(fEcanvasDist, new TGLayoutHints(kLHintsExpandX| kLHintsExpandY,10,10,10,1));

// hframe1
  TGHorizontalFrame *hframe1 = new TGHorizontalFrame(fMain,800,80);

// Combobox market
  fComboMarket1 = new TGComboBox(hframe1,50);
  for( int i=0; i<nmarkets; ++i )
    fComboMarket1->AddEntry(fMarket[i].Data(), i+1);
  fComboMarket1->Resize(140,20);
  fComboMarket1->Connect("Selected(Int_t)","BidAskFrame",this,"set_market(Int_t)");

// Combobox date
  fComboDate1 = new TGComboBox(hframe1,50);
  fComboDate1->Resize(120,20);
  fComboDate1->Connect("Selected(Int_t)","BidAskFrame",this,"set_date(Int_t)");

// Radio Buttons order by
  fBG1 = new TGButtonGroup(hframe1, "Tickers Order by", kVerticalFrame);
  fROrder[0] = new TGRadioButton(fBG1, new TGHotString("&Alph"));
  fROrder[1] = new TGRadioButton(fBG1, new TGHotString("&Liqd"));
  fROrder[0]->SetState(kButtonDown);
  fBG1->SetLayoutHints(new TGLayoutHints(kLHintsCenterX,5,5,1,1));
  fBG1->Connect("Clicked(Int_t)", "BidAskFrame", this, "update_symbol_box(Int_t)");

// Radio Buttons bid ask trade
  fBG2 = new TGButtonGroup(hframe1, "bid-ask/trade", kVerticalFrame);
  fROrder2[0] = new TGRadioButton(fBG2, new TGHotString("&Bidask"));
  fROrder2[1] = new TGRadioButton(fBG2, new TGHotString("&Trade"));
  fROrder2[0]->SetState(kButtonDown);
  fBG2->SetLayoutHints(new TGLayoutHints(kLHintsCenterX,5,5,1,1));
  fBG2->Connect("Clicked(Int_t)", "BidAskFrame", this, "draw2(Int_t)");

// Exit Button
  TGTextButton *exit = new TGTextButton(hframe1,"&Exit","gApplication->Terminate(0)");

// Combobox Symbol
  fComboName1 = new TGComboBox(hframe1,50);
  fComboName1->Resize(140,20);
  fComboName1->Connect("Selected(Int_t)","BidAskFrame",this,"draw(Int_t)");

// Draw Next Button Symbol
  TGTextButton *bDrawNext = new TGTextButton(hframe1," Next > ");
  bDrawNext->Connect("Clicked()","BidAskFrame",this,"draw_next()");

// Draw Before Button Symbol
  TGTextButton *bDrawBefore = new TGTextButton(hframe1," < Back ");
  bDrawBefore->Connect("Clicked()","BidAskFrame",this,"draw_before()");

// End hframe1
  hframe1->AddFrame(fComboMarket1, new TGLayoutHints(kLHintsCenterX,5,5,5,5));
  hframe1->AddFrame(fComboDate1, new TGLayoutHints(kLHintsCenterX,5,5,5,5));
  hframe1->AddFrame(fComboName1, new TGLayoutHints(kLHintsCenterX,5,5,5,5));
  hframe1->AddFrame(bDrawBefore, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  hframe1->AddFrame(bDrawNext, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  hframe1->AddFrame(fBG1, new TGLayoutHints(kLHintsCenterX,5,5,5,5));
  hframe1->AddFrame(fBG2, new TGLayoutHints(kLHintsCenterX,5,25,5,5));

  hframe1->AddFrame(exit, new TGLayoutHints(kLHintsRight,5,5,3,4));

  fMain->AddFrame(hframe1, new TGLayoutHints(kLHintsCenterX,2,2,2,2));

  fComboMarket1->Select(1);

//
// Main Frame...
//
  fMain->SetWindowName("Bid Ask");
  fMain->MapSubwindows();
  fMain->Resize(fMain->GetDefaultSize());
  fMain->MapWindow();
}

BidAskFrame::~BidAskFrame() {
  fMain->Cleanup();
  delete fMain;
}

void BidAskFrame::InitVars() {
  fPlots = new TObjArray();

  ifstream ifs("bidask.conf");
  TString market = "";
  TString feed = "";
  TString city = "";
  TString level = "";
  TString dir = "";

  TString basedir = "";
  ifs >> basedir;
  while( ifs >> market >> feed >> city >> level >> dir )
  {
    ++nmarkets;
    fMarketCode.push_back(market);
    if( level.Length() > 1 )
      fMarket.push_back(feed + " " + city + " " + level);
    else
      fMarket.push_back(feed + " " + city );
    fPath.push_back(basedir + dir);
  }

  return;
}

void BidAskFrame::draw(int iname)
{
  iname_ = iname;
  if( fBG2->GetButton(1)->IsOn() )
    draw_bidask();
  else if( fBG2->GetButton(2)->IsOn() )
    draw_trade();
  return;
}

void BidAskFrame::draw_bidask()
{
  fPlots->Delete();
  fPlots = new TObjArray();

  int iday = fComboDate1->GetSelectedEntry()->EntryId() - 1;

  TFile* f = new TFile( path + market + "_mon_" + fdate + ".root" );
  if( f->IsOpen() )
  {
    f->cd("bid_ask");

    TString ticker = fName1[iname_-1];
    TString name_bid = "gr_bid_" + ticker + "_" + fdate;
    TString name_ask = "gr_ask_" + ticker + "_" + fdate;
    TString name_spd = "gr_spd_" + ticker + "_" + fdate;

    TGraph* grB = (TGraph*)gDirectory->Get( name_bid );
    TGraph* grA = (TGraph*)gDirectory->Get( name_ask );
    TGraph* grS = (TGraph*)gDirectory->Get( name_spd );
    grB->GetXaxis()->SetTimeDisplay(1);
    grA->GetXaxis()->SetTimeDisplay(1);
    grS->GetXaxis()->SetTimeDisplay(1);

    if( grB != 0 && grA != 0 && grS != 0 )
    {
      fPlots->Add(grB);
      fPlots->Add(grA);
      fPlots->Add(grS);

      TString title1 = "Bid Ask " + market + " " + fdate + " " + ticker;
      gStyle->SetTitleX(0.16);
      gStyle->SetOptStat(0);
      grB->SetTitle(title1);
      grB->SetLineColor(2);
      grA->SetLineColor(4);
      TString title2 = "Spread " + market + " " + fdate + " " + ticker;
      grS->SetTitle(title2);

      TCanvas *fCanvas1 = fEcanvasDist->GetCanvas();
      fCanvas1->Clear();
      fCanvas1->Divide(1,2);

      fCanvas1->cd(1);
      gPad->SetGrid();
      grB->Draw("al");
      grA->Draw("l");

      fCanvas1->cd(2);
      gPad->SetGrid();
      grS->Draw("al");

      fCanvas1->cd();
      fCanvas1->Update();
    }
    else
    {
      TCanvas *fCanvas1 = fEcanvasDist->GetCanvas();
      fCanvas1->Clear();
      fCanvas1->Update();
    }
    f->Close();
  }
  delete f;

  return;
}

void BidAskFrame::draw_trade()
{
  fPlots->Delete();
  fPlots = new TObjArray();

  int iday = fComboDate1->GetSelectedEntry()->EntryId() - 1;

  TFile* f = new TFile( path + market + "_mon_" + fdate + ".root" );
  if( f->IsOpen() )
  {
    f->cd("bid_ask");

    TString ticker = fName1[iname_-1];
    TString name_bid = "gr_bid_" + ticker + "_" + fdate;
    TString name_ask = "gr_ask_" + ticker + "_" + fdate;
    TString name_tprc = "gr_tprc_" + ticker + "_" + fdate;
    TString name_tsze = "gr_tsze_" + ticker + "_" + fdate;

    TGraph* grB = (TGraph*)gDirectory->Get( name_bid );
    TGraph* grA = (TGraph*)gDirectory->Get( name_ask );
    TGraph* grTP = (TGraph*)gDirectory->Get( name_tprc );
    TGraph* grTS = (TGraph*)gDirectory->Get( name_tsze );
    grB->GetXaxis()->SetTimeDisplay(1);
    grTS->GetXaxis()->SetTimeDisplay(1);

    if( grB != 0 && grA != 0 && grTP != 0 && grTS != 0 )
    {
      int npts = grTS->GetN();
      double* x = grTS->GetX();
      double* y = grTS->GetY();
      double hlow = (int)(x[0]-1);
      double hhigh = (int)(x[npts-1]+1);
      int nx = hhigh - hlow;

      TH1F* hTS = new TH1F("hTS", "hTS", nx, hlow, hhigh);
      TH1F* hTSA = new TH1F("hTSA", "hTSA", nx, hlow, hhigh);
      hTS->SetDirectory(0);
      hTSA->SetDirectory(0);
      for( int i=0; i<npts; ++i )
        hTS->Fill(x[i], y[i]);
      for( int i=0; i<nx; ++i )
      {
        double acc = hTSA->GetBinContent(i) + hTS->GetBinContent(i+1);
        hTSA->SetBinContent(i+1, acc);
      }

      fPlots->Add(grB);
      fPlots->Add(grA);
      fPlots->Add(grTP);
      fPlots->Add(grTS);
      fPlots->Add(hTS);
      fPlots->Add(hTSA);

      TString title1 = "Bid Ask Trade " + market + " " + fdate + " " + ticker;
      gStyle->SetTitleX(0.16);
      gStyle->SetOptStat(0);
      grB->SetTitle(title1);
      grB->SetLineColor(2);
      grA->SetLineColor(4);
      grTP->SetMarkerStyle(24);
      grTP->SetMarkerSize(0.4);
      grTP->SetLineStyle(3);
      TString title2 = "Volume " + market + " " + fdate + " " + ticker;
      grTS->SetTitle(title2);

      TCanvas *fCanvas1 = fEcanvasDist->GetCanvas();
      fCanvas1->Clear();
      fCanvas1->Divide(1,2);

      fCanvas1->cd(1);
      gPad->SetGrid();
      grB->Draw("al");
      grA->Draw("l");
      grTP->Draw("pl");

      fCanvas1->cd(2);
      gPad->SetGrid();
      grTS->Draw("ap");
      hTS->SetLineColor(4);
      hTS->SetFillColor(4);
      hTS->Draw("same");
      gPad->Update();

      // integral of volume
      double rightMax = 1.1 * hTSA->GetMaximum();
      double scale = gPad->GetUymax() / rightMax;
      hTSA->SetLineColor(2);
      hTSA->Scale(scale);
      hTSA->Draw("same");

      // second axis
      TGaxis* axis = new TGaxis(gPad->GetUxmax(), gPad->GetUymin(),
                                gPad->GetUxmax(), gPad->GetUymax(), 0, rightMax, 510, "+L");
      axis->SetLineColor(2);
      axis->Draw();

      fCanvas1->cd();
      fCanvas1->Update();
    }
    else
    {
      TCanvas *fCanvas1 = fEcanvasDist->GetCanvas();
      fCanvas1->Clear();
      fCanvas1->Update();
    }
    f->Close();
  }
  delete f;

  return;
}

void BidAskFrame::set_date(int id)
{
  fdate = fDate[id-1];
  fNameAlph.clear();
  fNameLiqd.clear();

  vector<string> temp_name;
  map<int, int> m_nq_isym;
  map<int, int> m_nt_isym;

  TFile* f = new TFile( path + market + "_mon_" + fdate + ".root" );
  if( f->IsOpen() )
  {
    f->cd("bid_ask");
    TKey* key;
    TIter next( gDirectory->GetListOfKeys() );
    int isym = 0;
    int headerLen = 8;
    while( (key = (TKey*)next()) )
    {
      string name = key->GetName();
      int tickerLen = name.find("_", headerLen) - headerLen;
      if( name.size() > headerLen && name.substr(3,4) == "tprc" )
      {
        TGraph* temp_gr = (TGraph*)gDirectory->Get(name.c_str());
        if( temp_gr != 0 )
        {
          temp_name.push_back(name.substr(headerLen, tickerLen).c_str());
          int nt = temp_gr->GetN();
          delete temp_gr;
          m_nt_isym[nt] = isym++;
        }
      }
    }
  }
  f->Close();
  delete f;

  int name_size = temp_name.size();
  if( !temp_name.empty() )
  {
    for( int i=0; i<name_size; ++i )
      fNameAlph.push_back(temp_name[i].c_str());
    int x = 0;
    for( map<int, int>::reverse_iterator it = m_nt_isym.rbegin(); it != m_nt_isym.rend() && x < name_size; ++it, ++x )
      fNameLiqd.push_back(temp_name[it->second].c_str());
  }

  int iord = 1;
  if( fBG1->GetButton(1)->IsOn() )
    iord = 1;
  else if( fBG1->GetButton(2)->IsOn() )
    iord = 2;

  update_symbol_box(iord);
  return;
}

void BidAskFrame::update_symbol_box(int iord)
{
  fComboName1->RemoveAll();

  if( iord == 1 )
  {
    fName1 = &fNameAlph[0];
    int name_size = fNameAlph.size();
    for( int i=0; i<name_size; ++i )
      fComboName1->AddEntry( fName1[i], i+1 );
  }
  else if( iord == 2 )
  {
    fName1 = &fNameLiqd[0];
    int name_size = fNameLiqd.size();
    for( int i=0; i<name_size; ++i )
      fComboName1->AddEntry( fName1[i], i+1 );
  }

  return;
}

void BidAskFrame::draw2(int iord)
{
  if( 1 == iord )
    draw_bidask();
  else if( 2 == iord )
    draw_trade();
  return;
}

void BidAskFrame::set_market(int id)
{
  fComboDate1->RemoveAll();
  fComboName1->RemoveAll();
  market = fMarketCode[id-1];
  path = fPath[id-1];

  char cmd[100];
  sprintf( cmd, "get_date_list.py %s %s", market.Data(), path.Data() );
  system( cmd );

  ifstream ifs( "temp_dates.txt" );
  if( ifs.is_open() )
  {
    ifs >> ndays;

    if( ndays > 0 )
    {
      fDate.clear();

      TString date;
      for( int i=0; i<ndays; ++i )
      {
        ifs >> date;
        fDate.push_back(date);
        fComboDate1->AddEntry( fDate[i], i+1 );
      }
      fComboDate1->Resize(100,20);
      //fComboDate1->Select(ndays);
    }
    ifs.close();
    system( "del temp_dates.txt" );
  }

  return;
}

void BidAskFrame::draw_next()
{
  int nEntries = fComboName1->GetNumberOfEntries();
  int newName = min(fComboName1->GetSelectedEntry()->EntryId() + 1, nEntries);
  fComboName1->Select(newName);

  //int id = fComboName1->GetSelected();
  //draw(id);

  return;
}

void BidAskFrame::draw_before()
{
  int newName = max(1, fComboName1->GetSelectedEntry()->EntryId() - 1);
  fComboName1->Select(newName);

  //int id = fComboName1->GetSelected();
  //draw(id);

  return;
}
