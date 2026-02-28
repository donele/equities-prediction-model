#include <string>
#include <vector>
#include "FileTest.C"
#include "PlotSeries.h"
#include "DrawNtickers.h"
#include "DrawNquotes.h"
const int nEU = 51;
const int nCA = 15;
const int nAS = 10;
const int nMA = 2;
const int nSA = 2;
const int offsetCA = nEU;
const int offsetAS = nEU + nCA;
const int offsetMA = nEU + nCA + nAS;
const int offsetSA = nEU + nCA + nAS + nMA;
const int nMAX = nEU;
const int nRegion = 5;
int fHtmlOrder[nRegion][nMAX]
 = { {0,  nEU/3, nEU*2/3,
      1,  nEU/3 + 1, nEU*2/3+1,
      2,  nEU/3 + 2, nEU*2/3+2,
      3,  nEU/3 + 3, nEU*2/3+3,
      4,  nEU/3 + 4, nEU*2/3+4,
      5,  nEU/3 + 5, nEU*2/3+5,
      6,  nEU/3 + 6, nEU*2/3+6,
      7,  nEU/3 + 7, nEU*2/3+7,
      8,  nEU/3 + 8, nEU*2/3+8,
      9,  nEU/3 + 9, nEU*2/3+9,
      10, nEU/3 + 10, nEU*2/3+10,
      11, nEU/3 + 11, nEU*2/3+11,
      12, nEU/3 + 12, nEU*2/3+12,
      13, nEU/3 + 13, nEU*2/3+13,
      14, nEU/3 + 14, nEU*2/3+14,
      15, nEU/3 + 15, nEU*2/3+15,
      16, nEU/3 + 16, nEU*2/3+16},
     {offsetCA,     offsetCA + 1, offsetCA+2, 
      offsetCA + 3, offsetCA + 4, offsetCA+5,
      offsetCA + 6, offsetCA + 7, offsetCA+8,
      offsetCA + 9, offsetCA + 10, offsetCA+11,
      offsetCA + 12, offsetCA + 13, offsetCA+14},
     {offsetAS,     offsetAS + nAS/2,
      offsetAS + 1, offsetAS + nAS/2 + 1,
      offsetAS + 2, offsetAS + nAS/2 + 2,
      offsetAS + 3, offsetAS + nAS/2 + 3,
      offsetAS + 4, offsetAS + nAS/2 + 4},
     {offsetMA,     offsetMA + nMA/2}, 
     {offsetSA,     offsetSA + nSA/2} };
int nMarketsRegion[nRegion] = {nEU, nCA, nAS, nMA, nSA};

class WebMon {
public:
  WebMon();
  WebMon(string region);
  WebMon(int imarket, int itype);
  ~WebMon();

  void initialize();
  void draw(int imarket, int itype);
  int get_nmarkets();

private:
  TObjArray* fPlots;

  int fNmarkets;
  int fNdates;
  int fNdatesMin;
  int fNdatesMax;

  int fPadWidth;
  int fPadHeight;
  int fPadWidthComp;
  int fPadHeightComp;
  int fPadWidthSmall;
  int fPadHeightSmall;
  vector<TString> fContinents;
  TString homepage;

  vector<TString> fDate1;
  vector<TString> fPath1;
  vector<TString> fPathComp1;
  vector<TString> fPathNBBO1;
  vector<TString> fMarketCodeOut;
  vector<TString> fMarketCodeIn;
  vector<TString> fMarketDesc;
  vector<TString> fCity;
  vector<TString> fDesc;
  vector<TString> fDescDesc;
  vector<TString> fBodyTypes;
  vector<int> fUseOK;

  void InitVars();
  void draw_nquote(int imarket, int ndays, int padWidth);
  void draw_nticker(int imarket);
  void draw_bidask(int imarket);
  void draw_comp(int imarket);
  void get_dates();
  void update_html();
  void write_html_head(ofstream& ofs, string now, int nbody = 0);
  void write_html_body(int i, int j, string now);
  void write_html_body(ofstream& ofs, int i, TString btype);
  TString get_body_file(int i, TString btype);
  void write_html_bottom(ofstream& ofs, int nbody = 0);
  void write_html_home(ofstream& ofs);
  void write_html_menu(ofstream& ofs, int nbody);
  void write_html_home_market(ofstream& ofs, int m1, int m2);
  void write_html_home_market(ofstream& ofs, int m1, int m2, int m3);
  void write_html_market_stat(ofstream& ofs, int m1);
  void write_html_stat_column(ofstream& ofs, vector<string>& vs );
  void link_a_market(ofstream& ofs, int m1, int m2, int nbody);
  void link_a_market(ofstream& ofs, int m1, int m2, int m3, int nbody);
  bool exist_nbbo(int m1, int m2, int m3);
  TLegend* getLegend(TString desc, TGraph* gr1B, TGraph* gr2B);
};

// Do one region.
WebMon::WebMon( string region )
{
  initialize();
  fPlots = new TObjArray();
  InitVars();

  get_dates();
  for( int im=0; im<fNmarkets; ++im )
  {
    string market = fMarketCodeIn[im];
    string this_region = market.substr(0,1);
    if( region == this_region )
    {
      cout << " market " << market << endl;
      draw_nquote(im, fNdatesMin, fPadWidthSmall, fPadHeightSmall);
      draw_nquote(im, fNdatesMax, fPadWidth, fPadHeight);
      draw_nticker(im);
      draw_bidask(im);
      draw_comp(im);
    }
  }
  update_html();
}

// Do all the markets.
WebMon::WebMon()
{
  initialize();
  fPlots = new TObjArray();
  InitVars();

  get_dates();
  
  for( int im=0; im<fNmarkets; ++im )
  {
    string market = fMarketCodeIn[im];
    cout << " market " << market << endl;
    draw_nquote(im, fNdatesMin, fPadWidthSmall, fPadHeightSmall);
    draw_nquote(im, fNdatesMax, fPadWidth, fPadHeight);
    draw_nticker(im);
    draw_bidask(im);
    draw_comp(im);
  }

  update_html();
}

WebMon::WebMon(int imarket, int itype)
{
  initialize();
  fPlots = new TObjArray();
  InitVars();

  get_dates();
  draw(imarket, itype);
}

WebMon::~WebMon()
{
  if( fPlots != 0 )
    delete fPlots;
}

void WebMon::initialize()
{
  fNmarkets = 0;
  fNdates = 0;
  fNdatesMin = 3;
  fNdatesMax = 10;
  fPadWidth = 800;
  fPadWidthComp = 800;
  fPadWidthSmall = 240;
  fPadHeight = 160;
  fPadHeightComp = 300;
  fPadHeightSmall = 60;
  homepage = "liveTickSumm.html";
  return;
}

int WebMon::get_nmarkets()
{
  return fNmarkets;
}

void WebMon::InitVars()
{
  system("mkdir gif_live");
  system("mkdir html_live");

  gROOT->SetStyle("Plain");
  gStyle->SetOptStat(0);
  gStyle->SetPadLeftMargin(0.15);
  gStyle->SetPadRightMargin(0.15);

  ifstream ifs("wmon.conf");
  TString market = "";
  TString desc = "";
  TString descDesc = "";
  TString city = "";
  TString dir = "";
  TString dirComp = "";
  int useOK = 0;

  while( ifs >> market >> desc >> descDesc >> city >> dir >> dirComp >> useOK )
  {
    ++fNmarkets;
    fMarketCodeIn.push_back(market);
    fMarketCodeOut.push_back(market + desc);
    fMarketDesc.push_back(city + " " + desc);

    fPath1.push_back("\\\\smrc-ltc-mrct50\\data00\\tickMon\\mon\\" + dir);
    fPathComp1.push_back("\\\\smrc-ltc-mrct50\\data00\\tickMon\\bboComp\\" + dirComp);

    fCity.push_back(city);
    fDesc.push_back(desc);
    fDescDesc.push_back(descDesc);
    fUseOK.push_back(useOK);
    //cout<<market<<" "<<desc<<" "<<descDesc<<" "<<city<<" "<<dir<<" "<<dirComp<<" "<<useOK<<endl;
  }

  fBodyTypes.push_back("ntickers");
  fBodyTypes.push_back("nquote");
  fBodyTypes.push_back("bidask");
  fBodyTypes.push_back("comp");

  return;
}

void WebMon::draw(int imarket, int itype)
{
  if( itype == 0 )
    draw_nquote(imarket, fNdatesMin, fPadWidthSmall, fPadHeightSmall);
  else if( itype == 1 )
    draw_nquote(imarket, fNdatesMax, fPadWidth, fPadHeight);
  else if( itype == 2 )
    draw_nticker(imarket);
  else if( itype == 3 )
    draw_bidask(imarket);
  else if( itype == 4 )
    draw_comp(imarket);

  return;
}

void WebMon::draw_nquote(int imarket, int ndays, int padWidth, int padHeight)
{
  gStyle->SetTitleX(0.2);
  gStyle->SetTitleW(0.8);
  gStyle->SetTitleH(0.25);
  gStyle->SetPadTopMargin(0.2);
  gStyle->SetPadLeftMargin(0.08);
  gStyle->SetPadRightMargin(0.02);

  TString market = fMarketCodeIn[imarket];
  TString market2 = fMarketCodeOut[imarket];
  TString path = fPath1[imarket];

  TH1F** hist = new TH1F* [ndays];
  TH1F** histGap = new TH1F* [ndays];

  int histMax = 0;
  for( int i=0; i<ndays; ++i )
  {
    TString fdate = fDate1[i];
    hist[i] = 0;
    char fn[200];
    sprintf( fn, "%s%s_mon_%s.root", path.Data(), market.Data(), fdate.Data() );
    
    bool dataFound = false;
    TFile *f = 0;
    if( fileTest(fn) )
    {
      f = new TFile( fn, "read" );
      if( f->IsOpen() )
      {
        dataFound = true;
        TString name = "h_quote_10sec_" + fdate;
        TH1F* h = (TH1F*)f->Get( name.Data() );
        if( h != 0 )
        {
          hist[i] = h;
          hist[i]->SetDirectory(0);
          hist[i]->GetXaxis()->SetTimeDisplay(0);
          if( hist[i]->GetMaximum() > histMax )
            histMax = hist[i]->GetMaximum();
        }
        else
        {
          hist[i] = new TH1F("hbad_" + fdate, "Data not found [" + fdate + "]", 10,0,1);
          hist[i]->SetDirectory(0);
        }

        f->Close();
      }
    }

    if( !dataFound )
    {
      hist[i] = new TH1F("hbad_" + fdate, "Data not found [" + fdate + "]", 10,0,1);
    }
    if( f != 0 )
      delete f;
  }
  TCanvas* cnq = new TCanvas("cnq", "cnq", padWidth, padHeight*ndays);
  DrawNquotes dnq(cnq, hist, histGap, ndays, histMax);

  for( int i=0; i<ndays; ++i )
  {
    fPlots->Add(hist[i]);
    fPlots->Add(histGap[i]);
  }

  char outf[200];
  sprintf(outf, "gif_live\\nquote_%d_%s.gif", ndays, market2.Data());
  cnq->Print( outf );
  delete cnq;

  fPlots->Delete();
  delete [] hist;
  delete [] histGap;

  return;
}

void WebMon::draw_nticker(int imarket)
{
  gStyle->SetTitleX(0.3);
  gStyle->SetTitleW(0.4);
  gStyle->SetPadTopMargin(0.2);
  gStyle->SetTitleH(0.2);

  TString market = fMarketCodeIn[imarket];
  TString market2 = fMarketCodeOut[imarket];
  TString path = fPath1[imarket];
  TString pathComp = fPathComp1[imarket];
  int useOK = fUseOK[imarket];

  PlotSeries ps;
  DrawNtickers dn(ps, fNdatesMax, fDate1, path, pathComp, market, useOK);

  int nhist = ps.get_nhist();
  TH1F** hist = new TH1F* [nhist];
  for( int i=0; i<nhist; ++i )
  {
    hist[i] = (TH1F*)ps.get_hist(i)->Clone();
    hist[i]->GetXaxis()->SetLabelSize(0.1);
    hist[i]->GetYaxis()->SetLabelSize(0.1);
    fPlots->Add(hist[i]);
  }

  TCanvas *cnt = new TCanvas("cnt", "cnt", fPadWidth, fPadHeight*5);
  dn.draw_canvas(cnt, hist);
  char outf[200];
  sprintf( outf, "gif_live\\ntickers_%s.gif", market2.Data() );
  cnt->Print( outf );
  delete cnt;

  fPlots->Delete();
  delete [] hist;

  return;
}

void WebMon::draw_bidask(int imarket)
{
  gStyle->SetTitleX(0.2);
  gStyle->SetTitleW(0.4);
  gStyle->SetPadTopMargin(0.2);
  gStyle->SetTitleH(0.2);

  TString market = fMarketCodeIn[imarket];
  TString market2 = fMarketCodeOut[imarket];
  TString market3 = fMarketDesc[imarket];
  TString path = fPath1[imarket];

  int nTickerMax = 10;
  int headerLen = 7;
  TString fdate = "";

  TFile* f = 0;
  for( int iday = 0; (f == 0 || !f->IsOpen()) && iday < fNdatesMax; ++iday )
  {
    fdate = fDate1[iday];
    TString totalPath = path + market + "_mon_" + fDate1[iday] + ".root";
    if( fileTest(totalPath) )
      f = new TFile( totalPath );
  }

  if( f != 0 && f->IsOpen() )
  {
    // count the tickers
    int nTicker = 0;
    vector<string> vTickers;
    f->cd("bid_ask");
    TKey *key;
    TIter next( gDirectory->GetListOfKeys() );
    while( (key = (TKey*)next()) && nTicker < nTickerMax )
    {
      string grname = key->GetName();
      if( grname.substr(3,3) == "bid" )
      {
        ++nTicker;
        int tickerLen = grname.find("_", headerLen) - headerLen;
        TString ticker = grname.substr(headerLen, tickerLen).c_str();
        vTickers.push_back(ticker.Data());
      }
    }

    // draw
    if( nTicker > 0 )
    {
      // Create canvas
      TCanvas *cba = new TCanvas("cba", "cba", fPadWidth, fPadHeight*nTicker*2);

      cba->Divide(1,nTicker*2);
      for( int i=0; i<nTicker; ++i )
      {
        TString ticker = vTickers[i].c_str();

        TString name_bid = "gr_bid_" + ticker + "_" + fdate;
        TString name_ask = "gr_ask_" + ticker + "_" + fdate;
        TString name_spd = "gr_spd_" + ticker + "_" + fdate;

        TGraph* grB = (TGraph*)gDirectory->Get( name_bid );
        TGraph* grA = (TGraph*)gDirectory->Get( name_ask );
        TGraph* grS = (TGraph*)gDirectory->Get( name_spd );
        grB->GetXaxis()->SetTimeDisplay(0);
        grA->GetXaxis()->SetTimeDisplay(0);
        grS->GetXaxis()->SetTimeDisplay(0);

        grB->SetTitle("Bid Ask " + market3 + ":" + ticker + " " + fdate);
        grB->SetLineColor(2);
        grA->SetLineColor(4);
        grS->SetTitle("Spread " + market3 + ":" + ticker + " " + fdate);

        cba->cd(i*2+1);
        gPad->SetGrid();
        grB->GetXaxis()->SetLabelSize(0.1);
        grB->GetYaxis()->SetLabelSize(0.1);
        grB->Draw("al");
        grA->Draw("l");

        cba->cd(i*2+2);
        gPad->SetGrid();
        grS->GetXaxis()->SetLabelSize(0.1);
        grS->GetYaxis()->SetLabelSize(0.1);
        grS->Draw("al");

        fPlots->Add(grB);
        fPlots->Add(grA);
        fPlots->Add(grS);
      }

      char outf[200];
      sprintf(outf, "gif_live\\bidask_%s.gif", market2.Data());
      cba->Modified();
      cba->cd();
      cba->Print( outf );
      delete cba;

      fPlots->Delete();
    }
    f->Close();
  }
  if( f != 0 )
    delete f;

  return;
}

void WebMon::draw_comp(int imarket)
{
  gStyle->SetTitleX(0.08);
  gStyle->SetTitleW(0.7);
  gStyle->SetPadTopMargin(0.05);
  gStyle->SetPadBottomMargin(0.05);
  gStyle->SetPadLeftMargin(0.05);
  gStyle->SetPadRightMargin(0.05);
  gStyle->SetTitleH(0.15);

  TString city = fCity[imarket];
  TString market = fMarketCodeIn[imarket];
  TString market2 = fMarketCodeOut[imarket];
  TString market3 = fMarketDesc[imarket];
  TString path = fPathComp1[imarket];
  TString desc = fDesc[imarket];

  int nTickerMax = 10;
  int headerLen = 7;
  TString fdate = "";

  TFile* f = 0;
  for( int iday = 0; (f == 0 || !f->IsOpen()) && iday < fNdatesMax; ++iday )
  {
    fdate = fDate1[iday];
    TString totalPath = path + market + "_" + fDate1[iday] + ".root";
    if( fileTest(totalPath) )
      f = new TFile( totalPath );
  }

  if( f != 0 && f->IsOpen() )
  {
    // count the tickers
    int nTicker = 0;
    vector<string> vTickers;
    TKey *key;
    TIter next( gDirectory->GetListOfKeys() );
    while( (key = (TKey*)next()) && nTicker < nTickerMax )
    {
      string dirname = key->GetName();
      if( dirname.substr(0,1) != "h" )
      {
        ++nTicker;
        vTickers.push_back(dirname);
      }
    }

    // draw
    if( nTicker > 0 )
    {
      // Create canvas
      TCanvas *cba = new TCanvas("cba", "cba", fPadWidthComp, fPadHeightComp*nTicker*2);
      cba->Divide(2, nTicker*2);

      for( int i=0; i<nTicker; ++i )
      {
        TString ticker = vTickers[i].c_str();

        TString name_bid_1 = "gr1B";
        TString name_ask_1 = "gr1A";
        TString name_bid_2 = "gr2B";
        TString name_ask_2 = "gr2A";
        TString name_bidSize_1 = "gr1BS";
        TString name_askSize_1 = "gr1AS";
        TString name_bidSize_2 = "gr2BS";
        TString name_askSize_2 = "gr2AS";

        f->cd();
        f->cd(ticker);

        TGraph* gr1B = (TGraph*)gDirectory->Get( name_bid_1 );
        TGraph* gr1A = (TGraph*)gDirectory->Get( name_ask_1 );
        TGraph* gr2B = (TGraph*)gDirectory->Get( name_bid_2 );
        TGraph* gr2A = (TGraph*)gDirectory->Get( name_ask_2 );
        TGraph* gr1BS = (TGraph*)gDirectory->Get( name_bidSize_1 );
        TGraph* gr1AS = (TGraph*)gDirectory->Get( name_askSize_1 );
        TGraph* gr2BS = (TGraph*)gDirectory->Get( name_bidSize_2 );
        TGraph* gr2AS = (TGraph*)gDirectory->Get( name_askSize_2 );

        gr1B->GetXaxis()->SetTimeDisplay(0);
        gr1A->GetXaxis()->SetTimeDisplay(0);
        gr2B->GetXaxis()->SetTimeDisplay(0);
        gr2A->GetXaxis()->SetTimeDisplay(0);
        gr1BS->GetXaxis()->SetTimeDisplay(0);
        gr1AS->GetXaxis()->SetTimeDisplay(0);
        gr2BS->GetXaxis()->SetTimeDisplay(0);
        gr2AS->GetXaxis()->SetTimeDisplay(0);

        gr1B->SetTitle("Bid, " + city + " " + ticker + ", " + fdate);
        gr1B->SetLineColor(2);
        gr2B->SetLineColor(4);

        gr1A->SetTitle("Ask, " + city + " " + ticker + ", " + fdate);
        gr1A->SetLineColor(2);
        gr2A->SetLineColor(4);

        gr1BS->SetTitle("Bid Size, " + city + " " + ticker + ", " + fdate);
        gr1BS->SetLineColor(2);
        gr2BS->SetLineColor(4);

        gr1AS->SetTitle("Ask Size, " + city + " " + ticker + ", " + fdate);
        gr1AS->SetLineColor(2);
        gr2AS->SetLineColor(4);

        cba->cd(i*4 + 1);
        gPad->SetGrid();
        gr1B->GetXaxis()->SetLabelSize(0.1);
        gr2B->GetYaxis()->SetLabelSize(0.1);
        gr1B->Draw("al");
        gr2B->Draw("l");

        TLegend* leg1 = getLegend(desc, gr1B, gr2B);
        leg1->Draw();

        cba->cd(i*4 + 2);
        gPad->SetGrid();
        gr1A->GetXaxis()->SetLabelSize(0.1);
        gr2A->GetYaxis()->SetLabelSize(0.1);
        gr1A->Draw("al");
        gr2A->Draw("l");

        TLegend* leg2 = getLegend(desc, gr1A, gr2A);
        leg2->Draw();

        cba->cd(i*4 + 3);
        gPad->SetGrid();
        gr1BS->GetXaxis()->SetLabelSize(0.1);
        gr2BS->GetYaxis()->SetLabelSize(0.1);
        gr1BS->Draw("al");
        gr2BS->Draw("l");

        TLegend* leg3 = getLegend(desc, gr1BS, gr2BS);
        leg3->Draw();

        cba->cd(i*4 + 4);
        gPad->SetGrid();
        gr1AS->GetXaxis()->SetLabelSize(0.1);
        gr2AS->GetYaxis()->SetLabelSize(0.1);
        gr1AS->Draw("al");
        gr2AS->Draw("l");

        TLegend* leg4 = getLegend(desc, gr1AS, gr2AS);
        leg4->Draw();

        fPlots->Add(gr1B);
        fPlots->Add(gr1A);
        fPlots->Add(gr2B);
        fPlots->Add(gr2A);
        fPlots->Add(gr1BS);
        fPlots->Add(gr1AS);
        fPlots->Add(gr2BS);
        fPlots->Add(gr2AS);
        fPlots->Add(leg1);
        fPlots->Add(leg2);
        fPlots->Add(leg3);
        fPlots->Add(leg4);
      }

      char outf[100];
      sprintf(outf, "gif_live\\comp_%s.gif", market2.Data());
      cba->Modified();
      cba->cd();
      cba->Print( outf );

      fPlots->Delete();
      delete cba;
    }
    f->Close();
  }
  if( f != 0 )
    delete f;

  return;
}

void WebMon::update_html()
{
  fContinents.push_back("EU");
  fContinents.push_back("CA");
  fContinents.push_back("Asia");
  fContinents.push_back("MA");
  fContinents.push_back("SA");
  

  TTimeStamp time;
  string tnow = time.AsString("s");
  string now = tnow + " UTC";

  // Forward to the Home page
  ofstream ofs0( homepage.Data() );
  ofs0 << "<head><meta http-equiv=\"Refresh\" content=\"0; URL=html_live/liveTickSumm.html\"></head>";
  ofs0.close();

  // Home page
  char fn[200];
  sprintf(fn, "html_live\\%s", homepage.Data() );
  ofstream ofs(fn);
  write_html_head(ofs, now);
  write_html_home(ofs);
  write_html_bottom(ofs);
  ofs.close();

  // Each market
  for( int i=0; i<fNmarkets; ++i )
  {
    int nbt = fBodyTypes.size();
    for( int j=0; j<nbt; ++j )
      write_html_body(i, j, now);
  }
  // production directory 
  system( "xcopy /S /Y /I gif_live \\\\smrc-ltc-mrct50\\data00\\tickMon\\webmon\\gif_live" );
  system( "xcopy /S /Y /I html_live \\\\smrc-ltc-mrct50\\data00\\tickMon\\webmon\\html_live" );
  system( "xcopy /S /Y liveTickSumm.html \\\\smrc-ltc-mrct50\\data00\\tickMon\\webmon\\" );
  system( "rmdir /S /Q gif_live" );
  system( "rmdir /S /Q html_live" );
  system( "del liveTickSumm.html" );

  return;
}

void WebMon::write_html_body(int i, int j, string now)
{
  char fn2[200];
  TString btype = fBodyTypes[j];
  TString bodyfile = get_body_file(i, btype);
  sprintf(fn2, "html_live\\%s.html", bodyfile.Data());

  ofstream ofsm(fn2);
  write_html_head(ofsm, now, j);
  write_html_body(ofsm, i, btype);
  write_html_bottom(ofsm, j);
  ofsm.close();

  return;
}

void WebMon::write_html_head(ofstream& ofs, string now, int nbody)
{
  ofs << "<body>" << endl;
  ofs << "<table width=\"94%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" align=\"center\">\n";

  ifstream ifsh("holiday_alert.txt");
  if( ifsh.is_open() )
  {
    string line = "";
    getline(ifsh, line);
    if( line.size() > 10 )
      ofs << "<tr><td><b>Holiday Alert:</b> " << line << "</td></tr>\n";
  }

  ifstream ifsd("disk_alert.txt");
  if( ifsd.is_open() )
  {
    string line = "";
    while( getline(ifsh, line) )
      if( line.size() > 10 )
        ofs << "<tr><td>Disk Usage: " << line << "</td></tr>\n";
  }

  ofs << "<tr><td align=right>\n";
  ofs << "<p>Updated at " + now + "</p>\n";
  ofs << "</td></tr>\n";

  ofs << "<tr>\n";
  ofs << "<td>[";
  ofs << "<a href=" + homepage + ">Live Tick Summary</a> | ";
  ofs << "<a href=../fillratSumm.html>Fill Ratio Summary</a> | ";
  ofs << "<a href=../hstrTickSumm.html>Tick Data History</a> | ";
  ofs << "<a href=../usBookSumm.html>US Book Data</a> | ";
  ofs << "<a href=../tickDataAlerts.html>Tick Data Alerts</a>";
  ofs << "]</td>\n";
  ofs << "</tr>\n";
  ofs << "</table>\n";
  write_html_menu(ofs, nbody);
  return;
}

void WebMon::write_html_bottom(ofstream& ofs, int nbody)
{
  write_html_menu(ofs, nbody);
  ofs << "</body>" << endl;
  return;
}

void WebMon::write_html_home(ofstream& ofs)
{ 
  ofs << "<table width=\"94%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" align=\"center\">\n";
  for( int i=0; i<nRegion; ++i )
  {
    if ( i == 0 || i == 1 ) // EU and CA have L1, L2 and NBBO
    { 
      for( int j=0; j<nMarketsRegion[i]; ++++++j )
        write_html_home_market(ofs, fHtmlOrder[i][j], fHtmlOrder[i][j+1], fHtmlOrder[i][j+2]);
    }
    else // others only have L1 and L2 
    {
      for( int j=0; j<nMarketsRegion[i]; ++++j )
        write_html_home_market(ofs, fHtmlOrder[i][j], fHtmlOrder[i][j+1]);
    }
  }
  ofs << "</table>" << endl;
  return;
}

void WebMon::write_html_home_market(ofstream& ofs, int m1, int m2)
{
  char buf[1000];

  // market
  sprintf(buf, "<tr align=left><td colspan=6><b>%s</b></td></tr>", fCity[m1].Data() );
  ofs << buf;

  // header
  ofs << "<tr align=center>";
  //sprintf(buf, "<td>%s</td><td>%s</td>\n", fDescDesc[m1].Data(), fDescDesc[m2].Data());
  sprintf(buf, "<td>%s</td><td>%s</td><td></td>\n", fDescDesc[m1].Data(), fDescDesc[m2].Data());
  //ofs << "<td>fDescDesc[m1]</td><td>fDescDesc[m2]</td>";
  ofs << buf;
  ofs << "<td>idate</td><td>dataOK</td><td>tickValid</td><td>nUniv</td>";
  ofs << "</tr>\n";

  // content
  ofs << "<tr align=center>\n";
  if(m1>=0 && m2>=0)
  {
    // L1
    sprintf(buf, "<td><a href=%s.html><img src=../gif_live/nquote_%d_%s.gif></a></td>\n",
    get_body_file(m1, fBodyTypes[0]).Data(), fNdatesMin, fMarketCodeOut[m1].Data() );
    ofs << buf;
    // L2
    sprintf(buf, "<td><a href=%s.html><img src=../gif_live/nquote_%d_%s.gif></a></td>\n",
    get_body_file(m2, fBodyTypes[0]).Data(), fNdatesMin, fMarketCodeOut[m2].Data() );
    ofs << buf;
    // Empty NBBO
    sprintf(buf, "%s", "<td></td>\n");
    ofs << buf;
  }

    // dataOK and #univ
    write_html_market_stat(ofs, m1);

  ofs << "</tr>\n";

  // horizontal line
  ofs << "<tr><td colspan=7><hr></td></tr>\n";

  return;
}

void WebMon::write_html_home_market(ofstream& ofs, int m1, int m2, int m3)
{
  char buf[1000];
  // market
  sprintf(buf, "<tr align=left><td colspan=7><b>%s</b></td></tr>", fCity[m1].Data() );
  ofs << buf;
  // header
  ofs << "<tr align=center>";
  if ( exist_nbbo(m1,m2,m3) )
    sprintf(buf, "<td>%s</td><td>%s</td><td>%s</td>\n", fDescDesc[m1].Data(), fDescDesc[m2].Data(), fDescDesc[m3].Data());
  else
    sprintf(buf, "<td>%s</td><td>%s</td><td></td>\n", fDescDesc[m1].Data(), fDescDesc[m2].Data());
  //ofs << "<td>fDescDesc[m1]</td><td>fDescDesc[m2]</td>";
  ofs << buf;
  ofs << "<td>idate</td><td>dataOK</td><td>tickValid</td><td>nUniv</td>";
  ofs << "</tr>\n";

  // content
  ofs << "<tr align=center>\n";
 
  if( m1>=0 && m2 >= 0 && m3>=0)
  { 
    // L1 quotes
    sprintf(buf, "<td><a href=%s.html><img src=../gif_live/nquote_%d_%s.gif></a></td>\n",
    get_body_file(m1, fBodyTypes[0]).Data(), fNdatesMin, fMarketCodeOut[m1].Data() );
    ofs << buf;
    // L2 quotes   
    sprintf(buf, "<td><a href=%s.html><img src=../gif_live/nquote_%d_%s.gif></a></td>\n",
    get_body_file(m2, fBodyTypes[0]).Data(), fNdatesMin, fMarketCodeOut[m2].Data() );
    ofs << buf;
    // NBBO
    if ( exist_nbbo(m1,m2,m3) )
    {
    sprintf(buf, "<td><a href=%s.html><img src=../gif_live/nquote_%d_%s.gif></a></td>\n",
    get_body_file(m3, fBodyTypes[0]).Data(), fNdatesMin, fMarketCodeOut[m3].Data() );
    ofs << buf;
    }
    else // empty NBBO
    {
    sprintf(buf, "%s", "<td></td>\n");
    ofs << buf;
    }
  }
    // dataOK and #univ
    write_html_market_stat(ofs, m1);

    ofs << "</tr>\n";

    // horizontal line
    ofs << "<tr><td colspan=7><hr></td></tr>\n";

  return;
}

void WebMon::write_html_market_stat(ofstream& ofs, int m1)
{
  vector<string> v_idate;
  vector<string> v_dataOK;
  vector<string> v_tickValid;
  vector<string> v_nUniv;

  ifstream ifs("market_stat.txt");
  string market, idate, dataOK, tickValid, nUniv;
  while( ifs >> market >> idate >> dataOK >> tickValid >> nUniv )
  {
    if( market.c_str() == fMarketCodeIn[m1] )
    {
      v_idate.push_back(idate);
      v_dataOK.push_back(dataOK);
      v_tickValid.push_back(tickValid);
      v_nUniv.push_back(nUniv);
    }
  }

  ofs << "<td>";
    write_html_stat_column(ofs, v_idate);
  ofs << "</td>";

  ofs << "<td>";
    write_html_stat_column(ofs, v_dataOK);
  ofs << "</td>";

  ofs << "<td>";
    write_html_stat_column(ofs, v_tickValid);
  ofs << "</td>";

  ofs << "<td>";
    write_html_stat_column(ofs, v_nUniv);
  ofs << "</td>";

  return;
}

void WebMon::write_html_stat_column(ofstream& ofs, vector<string>& vs )
{
  int vs_size= vs.size();
  for( int i=0; i<vs_size; ++i )
    ofs << vs[i] << "<br>";

  return;
}

void WebMon::write_html_menu(ofstream& ofs, int nbody)
{
  ofs << "<table width=\"94%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" align=\"center\">\n";
  ofs << "<font size=2>\n";
  ofs << "<tr><td colspan=2><hr></td></tr>\n";

  for( int i=0; i != fContinents.size(); ++i )
  {
      ofs << "<tr>\n";
      ofs << "<td>[" << fContinents[i] << "]</td>\n";
      ofs << "<td>\n";
      if( i==0 || i ==1 ) // EU and CA markets L1, L2 and NBBO
      { 
          for( int j=0; j<nMarketsRegion[i]; j+=3 )
          link_a_market(ofs, fHtmlOrder[i][j], fHtmlOrder[i][j+1], fHtmlOrder[i][j+2], nbody);
      }
      else // other Non-EU markets L1 and L2 
      { 
          for( int j=0; j<nMarketsRegion[i]; j+=2 ) 
          link_a_market(ofs, fHtmlOrder[i][j], fHtmlOrder[i][j+1], nbody);
      }

    ofs << "</td>\n";

    ofs << "</tr>\n";
  }

  ofs << "<tr><td colspan=2><hr></td></tr>\n";

  ofs << "</font>\n";
  ofs << "</table>" << endl;

  return;
}

void WebMon::link_a_market(ofstream& ofs, int m1, int m2, int nbody)
{ 
  char buf[1000];
  TString file1 = get_body_file(m1, fBodyTypes[nbody]);
  TString file2 = get_body_file(m2, fBodyTypes[nbody]);
  if( m1 >= 0 && m2 >= 0 )
  {
    sprintf(buf, "%s [<a href=%s.html>%s</a>|<a href=%s.html>%s</a>]\n",
      fCity[m1].Data(), file1.Data(), fDesc[m1].Data(), file2.Data(), fDesc[m2].Data() );
  }
  else
  {
    sprintf(buf, "%s [<a href=%s.html>%s</a>]\n",
      fCity[m1].Data(), file1.Data(), fDesc[m1].Data() );
  }
  ofs << buf;
  return;
}

void WebMon::link_a_market(ofstream& ofs, int m1, int m2, int m3, int nbody)
{   
  char buf[1000];
  TString file1 = get_body_file(m1, fBodyTypes[nbody]);
  TString file2 = get_body_file(m2, fBodyTypes[nbody]);
  TString file3 = get_body_file(m3, fBodyTypes[nbody]);

  if( m1 >= 0 && m2 >= 0 && m3 >= 0)
  {
    if ( exist_nbbo(m1,m2,m3) )
    sprintf(buf, "%s [<a href=%s.html>%s</a>|<a href=%s.html>%s</a>|<a href=%s.html>%s</a>]\n",
      fCity[m1].Data(), file1.Data(), fDesc[m1].Data(), file2.Data(), fDesc[m2].Data(), file3.Data(), fDesc[m3].Data() );
  else
    sprintf(buf, "%s [<a href=%s.html>%s</a>|<a href=%s.html>%s</a>]\n",
      fCity[m1].Data(), file1.Data(), fDesc[m1].Data(), file2.Data(), fDesc[m2].Data() );
  }
  ofs << buf;
  return;
}

void WebMon::write_html_body(ofstream& ofs, int i, TString btype)
{
  ofs << "<table width=\"94%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" align=\"center\">\n";

  ofs << "<tr><td>[\n";
  int nbt = fBodyTypes.size();
  for( int b=0; b<nbt; ++b )
  {
    TString btypeLoop = fBodyTypes[b];
    if( btype == btypeLoop )
      ofs << btypeLoop;
    else
      ofs << "<a href=" << get_body_file(i, btypeLoop) << ".html>" << btypeLoop << "</a>";
    if( b < nbt - 1 )
      ofs << " | ";
  }
  ofs << "]</td></tr>\n";
  ofs << "<tr><td><hr></td></tr>\n";

    ofs << "<tr>\n";

      ofs << "<td align=center valign=bottom bgcolor=fff8dc>\n";
      ofs << "<H1>" + fMarketDesc[i] + "</h1>\n";
      ofs << "</td>\n";

    ofs << "</tr>\n";

    ofs << "<tr><td><img src=\"../gif_live/" << get_body_file(i, btype) << ".gif\"><hr></td></tr>\n";

  ofs << "</table>" << endl;
  ofs << "</body>" << endl;

  return;
}

TString WebMon::get_body_file(int i, TString btype)
{
  TString ret = "";
  if( "ntickers" == btype )
  {
    ret = btype + "_" + fMarketCodeOut[i];
  }
  else if( "nquote" == btype )
  {
    char nday[5];
    sprintf( nday, "%d", fNdatesMax );
    ret = btype + "_" + nday + "_" + fMarketCodeOut[i];
  }
  else if( "bidask" == btype )
    ret = btype + "_" + fMarketCodeOut[i];
  else if( "comp" == btype )
    ret = btype + "_" + fMarketCodeOut[i];

  return ret;
}

void WebMon::get_dates()
{
  char cmd[100];
  sprintf( cmd, "get_recent_dates.py %d", fNdatesMax );
  system( cmd );

  ifstream ifs( "temp_recent_dates.txt" );
  if( ifs.is_open() )
  {
    ifs >> fNdates;
    if( fNdates > 0 )
    {
      fDate1.clear();

      for( int i=0; i<fNdates; ++i )
      {
        TString date;
        ifs >> date;
        fDate1.push_back(date);
      }
    }
    ifs.close();
    system( "del temp_recent_dates.txt" );
  }

  return;
}

bool WebMon::exist_nbbo(int m1, int m2, int m3)
{
  bool nbbo = true;
  if (m3 == nEU*2/3+14 || m3 == nEU*2/3+15 || m3 == nEU*2/3+16 || m3 == nEU*2/3+17 ||
    m3 == offsetCA+5 || m3 == offsetCA+8 || m3 == offsetCA+11 || m3 == offsetCA+14 )
    nbbo = false;
  return nbbo;
}

TLegend* WebMon::getLegend(TString desc, TGraph* gr1B, TGraph* gr2B)
{
  TLegend* leg = 0;

  if( desc == "L1" || desc == "L2" )
  {
    leg = new TLegend(0.85, 0.7, 0.99, 0.99);
    leg->SetFillStyle(0);
    leg->SetBorderSize(0);
    leg->AddEntry(gr1B, "L1", "L");
    leg->AddEntry(gr2B, "L2", "L");
  }
  else if( desc == "NBBO" )
  {
    leg = new TLegend(0.8, 0.7, 0.99, 0.99);
    leg->SetFillStyle(0);
    leg->SetBorderSize(0);
    leg->AddEntry(gr1B, "Primary", "L");
    leg->AddEntry(gr2B, "NBBO", "L");
  }
  else
  {
    leg = new TLegend(0.1,0.1,0.9,0.9);
    leg->SetFillStyle(0);
    leg->SetBorderSize(0);
  }
  return leg;
}

