#include <string>
#include <vector>
#include <TStyle.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TRootEmbeddedCanvas.h>
#include <RQ_OBJECT.h>
#include "PlotSeries.h"
#include "DrawNtickers.h"
#include "DrawNquotes.h"

////////////////////////////////////////////////////////////////////////////////
// Class TickMonFrame
////////////////////////////////////////////////////////////////////////////////
class TickMonFrame {
  RQ_OBJECT("TickMonFrame")

public:
  TickMonFrame();
  TickMonFrame(const TGWindow *p, UInt_t w, UInt_t h);
  virtual ~TickMonFrame();

  void draw_nquote();
  void draw_ntrade();
  void draw_nticker();

private:
  TGMainFrame *fMain;
  TRootEmbeddedCanvas *fEcanvasDist;  // for intra-day distribution

  TGComboBox *fComboMarket1;
  TGComboBox *fComboDate1;
  TGComboBox *fComboNpast1;

  TObjArray *fPlots;

  int nvars;
  int ndays;
  int nmarkets;
  int nnpast;
  int useOK_;

  TString market;
  TString path;
  TString pathComp;
  vector<TString> fNpast;
  vector<TString> fTicker;
  vector<TString> fDate;
  vector<TString> fRecentDate;
  vector<TString> fMarket;
  vector<TString> fMarketCode;
  vector<TString> fPath;
  vector<TString> fPathComp;
  vector<int> fUseOK;

  void draw_nquote_ntrade(TString var);
  TString find_name(TDirectory* dir, TString name0);
  void set_market(int id);
  void set_date(int id);
};

TickMonFrame::TickMonFrame(){}

TickMonFrame::TickMonFrame(const TGWindow *p, UInt_t w, UInt_t h)
:nmarkets(0)
{
  gROOT->SetStyle("Plain");
  gStyle->SetOptStat(0);
  gStyle->SetPadTopMargin(0.2);
  gStyle->SetPadBottomMargin(0.16);
  gStyle->SetPadLeftMargin(0.12);
  gStyle->SetTitleX(0.2);
  gStyle->SetTitleH(0.14);

// Initialize the class

  InitVars();

// ----------------------------------------------------------------
// Part I
// ----------------------------------------------------------------

// Create a main frame
  fMain = new TGMainFrame(p,w,h);

// Create an upper canvas widget
  fEcanvasDist = new TRootEmbeddedCanvas("EcanvasDist",fMain,800,800);
  fMain->AddFrame(fEcanvasDist, new TGLayoutHints(kLHintsExpandX| kLHintsExpandY,10,10,10,1));

// hframe1
  TGHorizontalFrame *hframe1 = new TGHorizontalFrame(fMain,800,80);

// group hframe1g1
  TGGroupFrame *hframe1g1 = new TGGroupFrame(hframe1,"Draw", kHorizontalFrame);
  hframe1g1->SetTitlePos(TGGroupFrame::kLeft);

// Combobox nnpast
  fComboNpast1 = new TGComboBox(hframe1, 50);
  for( int i=0; i<nnpast; ++i )
    fComboNpast1->AddEntry(fNpast[i].Data(), i+1);
  fComboNpast1->Resize(40, 20);
  fComboNpast1->Select(1);

// Combobox market
  fComboMarket1 = new TGComboBox(hframe1,50);
  for( int i=0; i<nmarkets; ++i )
    fComboMarket1->AddEntry(fMarket[i].Data(), i+1);
  fComboMarket1->Resize(180,20);
  fComboMarket1->Connect("Selected(Int_t)","TickMonFrame",this,"set_market(Int_t)");

// Combobox date
  fComboDate1 = new TGComboBox(hframe1,50);
  fComboDate1->Resize(120,20);
  fComboDate1->Connect("Selected(Int_t)","TickMonFrame",this,"set_date(Int_t)");

// Exit Button
  TGTextButton *exit = new TGTextButton(hframe1,"&Exit","gApplication->Terminate(0)");

// Draw Button Quote
  TGTextButton *bDrawQuote = new TGTextButton(hframe1g1," #quotes ");
  bDrawQuote->Connect("Clicked()","TickMonFrame",this,"draw_nquote()");

// Draw Button Trade
  TGTextButton *bDrawTrade = new TGTextButton(hframe1g1," #trades ");
  bDrawTrade->Connect("Clicked()","TickMonFrame",this,"draw_ntrade()");

//// Draw Button Cross
  //TGTextButton *bDrawCross = new TGTextButton(hframe1g1," crosses ");
  //bDrawCross->Connect("Clicked()","TickMonFrame",this,"draw_cross()");

// Draw Button Ticker
  TGTextButton *bDrawTicker = new TGTextButton(hframe1g1," #tickers ");
  bDrawTicker->Connect("Clicked()","TickMonFrame",this,"draw_nticker()");

// End hframe1
  hframe1->AddFrame(fComboMarket1, new TGLayoutHints(kLHintsCenterX,5,5,5,5));
  hframe1->AddFrame(fComboDate1, new TGLayoutHints(kLHintsCenterX,5,5,5,5));
  hframe1->AddFrame(fComboNpast1, new TGLayoutHints(kLHintsCenterX,5,45,5,5));

  hframe1g1->AddFrame(bDrawQuote, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  hframe1g1->AddFrame(bDrawTrade, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  hframe1g1->AddFrame(bDrawTicker, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  hframe1->AddFrame(hframe1g1, new TGLayoutHints(kLHintsCenterX,5,45,3,4));

  hframe1->AddFrame(exit, new TGLayoutHints(kLHintsRight,5,5,3,4));

  fMain->AddFrame(hframe1, new TGLayoutHints(kLHintsCenterX,2,2,2,2));

  fComboMarket1->Select(1);
  //fComboDate1->Select(ndays);

//
// Main Frame...
//
  fMain->SetWindowName("TickData Monitor");
  fMain->MapSubwindows();
  fMain->Resize(fMain->GetDefaultSize());
  fMain->MapWindow();
}

void TickMonFrame::InitVars() {
  fPlots = new TObjArray();

  ifstream ifs("mon.conf");
  TString market = "";
  TString feed = "";
  TString city = "";
  TString level = "";
  TString dir = "";
  TString dirComp = "";
  int useOK = 0;
  while( ifs >> market >> feed >> city >> level >> dir >> dirComp >> useOK )
  {
    ++nmarkets;
    fMarketCode.push_back(market);
    if( level.Length() > 1 )
      fMarket.push_back(feed + " " + city + " " + level);
    else
      fMarket.push_back(feed + " " + city );
    fPath.push_back("\\\\smrc-ltc-mrct16\\data00\\tickMon\\mon\\" + dir);
    fPathComp.push_back("\\\\smrc-ltc-mrct16\\data00\\tickMon\\bboComp\\" + dirComp);
    fUseOK.push_back(useOK);
  }

  fNpast.push_back("5");
  fNpast.push_back("10");
  fNpast.push_back("15");
  fNpast.push_back("20");
  fNpast.push_back("40");
  nnpast = fNpast.size();

  return;
}

void TickMonFrame::draw_nquote_ntrade(TString var)
{
  fPlots->Delete();
  fPlots = new TObjArray();

  int inpast = fComboNpast1->GetSelectedEntry()->EntryId() - 1;
  int npast = fNpast[inpast].Atoi();
  if( npast > 10 )
    npast = 10;

  TH1F** hist = new TH1F*[npast];
  TH1F** histGap = new TH1F*[npast];

  int histMax = 0;
  for( int i=0; i<npast; ++i )
  {
    hist[i] = 0;
    TString fdate = fRecentDate[i];
    TFile* f = new TFile( path + market + "_mon_" + fdate + ".root" );
    if( f->IsOpen() )
    {
      //TString name = "h_" + var + "_10sec_" + fdate;
      TString name = find_name(f, "h_" + var + "_");
      TH1F* h = (TH1F*)f->Get( name.Data() );
      if( h != 0 )
      {
        hist[i] = h;
        hist[i]->SetDirectory(0);
        hist[i]->GetXaxis()->SetTimeDisplay(1);
        if( hist[i]->GetMaximum() > histMax )
          histMax = hist[i]->GetMaximum();
      }
      else
        hist[i] = new TH1F("hbad_" + fdate, "Data not found [" + fdate + "]", 10, 0, 1);

      f->Close();
    }
    else
    {
      hist[i] = new TH1F("hbad_" + fdate, "Data not found [" + fdate + "]", 10, 0, 1);
    }
    delete f;
  }

  TCanvas *fCanvas1 = fEcanvasDist->GetCanvas();
  fCanvas1->Clear();
  DrawNquotes dnq(fCanvas1, hist, histGap, npast, histMax);
  for( int i=0; i<npast; ++i )
  {
    fPlots->Add(hist[i]);
    fPlots->Add(histGap[i]);
  }
  fCanvas1->Update();

  return;
}

TString TickMonFrame::find_name(TDirectory* dir, TString name0)
{
  TObject *obj;
  TKey *key;
  TIter next( dir->GetListOfKeys() );
  while( key = (TKey*)next() )
  {
    TString name = key->GetName();
    if( name.Contains(name0) )
      return name;
  }
  return "";
}

void TickMonFrame::draw_nquote()
{
  draw_nquote_ntrade("quote");
  return;
}

void TickMonFrame::draw_ntrade()
{
  draw_nquote_ntrade("trade");
  return;
}

void TickMonFrame::draw_nticker() {
  fPlots->Delete();
  fPlots = new TObjArray();

  int inpast = fComboNpast1->GetSelectedEntry()->EntryId() - 1;
  int npast = fNpast[inpast].Atoi();

  PlotSeries ps;
  DrawNtickers dn(ps, npast, fRecentDate, path, pathComp, market, useOK_);

  int nhist = ps.get_nhist();
  TH1F** hist = new TH1F* [nhist];
  for( int i=0; i<nhist; ++i )
  {
    hist[i] = (TH1F*)ps.get_hist(i)->Clone();
    hist[i]->GetXaxis()->SetLabelSize(0.1);
    hist[i]->GetYaxis()->SetLabelSize(0.1);
    fPlots->Add(hist[i]);
  }

  TCanvas *fCanvas1 = fEcanvasDist->GetCanvas();
  dn.draw_canvas(fCanvas1, hist);

  return;
}

void TickMonFrame::set_date(int id)
{
  int npastMax = 0;
  if( !fNpast.empty() )
    npastMax = fNpast[fNpast.size()-1].Atoi();

  char cmd[100];
  sprintf( cmd, "get_recent_dates.py %d %s", npastMax, fDate[id - 1].Data() );
  system(cmd);

  int dummy = 0;
  ifstream ifs( "temp_recent_dates.txt" );
  if( ifs.is_open() )
  {
    ifs >> dummy;
    fRecentDate.clear();
    for( int i=0; i<npastMax; ++i )
    {
      TString date;
      ifs >> date;
      fRecentDate.push_back(date);
    }
    ifs.close();
    system( "del temp_recent_dates.txt" );
  }
}

void TickMonFrame::set_market(int id)
{
  fComboDate1->RemoveAll();
  market = fMarketCode[id-1];
  path = fPath[id-1];
  pathComp = fPathComp[id-1];
  useOK_ = fUseOK[id-1];

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
      fComboDate1->Select(ndays);
    }
    ifs.close();
    system( "del temp_dates.txt" );
  }

  return;
}

TickMonFrame::~TickMonFrame() {
  fMain->Cleanup();
  delete fMain;
}
