#include "PlotSeries.h"
#include "FileTest.C"
#include "TPad.h"

class DrawNtickers {
public:
  DrawNtickers(PlotSeries& ps, int npast, vector<TString>& fRecentDate,
               TString path, TString pathComp, TString market, int useOK);
  void draw_comp( TPad* pad, TH1F** h );
  void draw_canvas(TCanvas* c, TH1F** hist);
  TGraph* hist_to_graph(TH1F* h, double*, double*, double*);
  TGraph* hist_to_graph(TH1F* h, double* mini, double* maxi, double* mean);

private:
  vector<double> get_eff(TGraph* g);
};

DrawNtickers::DrawNtickers(PlotSeries& ps, int npast, vector<TString>& fRecentDate,
                           TString path, TString pathComp, TString market, int useOK)
{
  vector<string> varnames;
  varnames.push_back("quote");
  varnames.push_back("trade");
  varnames.push_back("order");
  varnames.push_back("crossed");
  varnames.push_back("eff_max_95");
  varnames.push_back("eff_max_90");
  varnames.push_back("eff_max_80");
  varnames.push_back("eff_sum_95");
  varnames.push_back("eff_sum_90");
  varnames.push_back("eff_sum_80");
  varnames.push_back("prcCompGood");
  varnames.push_back("prcCompAll");
  varnames.push_back("sizeCompGood");
  varnames.push_back("sizeCompAll");
  varnames.push_back("latency mean");
  varnames.push_back("latency stdev");
  varnames.push_back("book depth");
  varnames.push_back("book depth max");
  varnames.push_back("total quotes");
  varnames.push_back("total trades");
  varnames.push_back("total orders");

  ps.set_varnames(varnames);

  for( int i=0; i<npast; ++i )
  {
    TString fdate = fRecentDate[npast - i - 1];

    double nticker_quote = 0;
    double nticker_trade = 0;
    double nticker_order = 0;
    double nticker_cross = 0;
    double eff_max_95 = 0;
    double eff_max_90 = 0;
    double eff_max_80 = 0;
    double eff_sum_95 = 0;
    double eff_sum_90 = 0;
    double eff_sum_80 = 0;
    double prcCompGood = 0;
    double prcCompAll = 0;
    double sizeCompGood = 0;
    double sizeCompAll = 0;
    double latencyMean = 0;
    double latencyStdev = 0;
    double bookDepth = 0;
    double bookDepthMax = 0;
    double totNquotes = 0;
    double totNtrades = 0;
    double totNorders = 0;

    // number of symbols plots and ticker eff
    TString fn = path + market + "_mon_" + fdate + ".root";
    if( fileTest(fn) )
    {
      TFile *f = new TFile( fn );
      if( f->IsOpen() )
      {
        //
        // number of symbols
        //
        TString name = "h_n_symbol_" + fdate;
        TH1F* h = (TH1F*)gDirectory->Get( name.Data() );
        if( h != 0 )
        {
          nticker_quote = h->GetBinContent(2);
          nticker_trade = h->GetBinContent(3);
          nticker_order = h->GetBinContent(4);
          nticker_cross = h->GetBinContent(5);

          delete h;
        }

        //
        // ticker eff max
        //
        name = "gr_eff_max_" + fdate;
        TGraph* gm = (TGraph*)gDirectory->Get( name.Data() );
        if( gm != 0 )
        {
          vector<double> vv = get_eff(gm);
          eff_max_95 = vv[0];
          eff_max_90 = vv[1];
          eff_max_80 = vv[2];

          delete gm;
        }

        //
        // ticker eff sum
        //
        name = "gr_eff_sum_" + fdate;
        TGraph* gs = (TGraph*)gDirectory->Get( name.Data() );
        if( gs != 0 )
        {
          vector<double> vv = get_eff(gs);
          eff_sum_95 = vv[0];
          eff_sum_90 = vv[1];
          eff_sum_80 = vv[2];

          delete gs;
        }

        //
        // Book Depth, total quote/trade/order
        //
        name = "h_stat_" + fdate;
        TH1F* hd = (TH1F*)gDirectory->Get( name.Data() );
        if( hd != 0 )
        {
          int nbins = hd->GetNbinsX();
          if( nbins >= 1 )
            bookDepth = hd->GetBinContent(1);
          if( nbins >= 5 )
            bookDepthMax = hd->GetBinContent(5);
          if( nbins >= 2 )
            totNquotes = hd->GetBinContent(2);
          if( nbins >= 3 )
            totNtrades = hd->GetBinContent(3);
          if( nbins >= 4 )
            totNorders = hd->GetBinContent(4);

          delete hd;
        }

        f->Close();
        delete f;
      }
    }


    //
    // L1 vs L2 comparison and latency
    //
    TString fn = pathComp + market + "_" + fdate + ".root";
    if( fileTest(fn) )
    {
      TFile *f = new TFile( fn );
      if( f->IsOpen() )
      {
        bool filledComp = false;
        TH1F* ha = (TH1F*)gDirectory->Get( "h_agree" );
        if( ha != 0 )
        {
          double dataok = ha->GetBinContent(1);
          if( !useOK || dataok > 0.8 )
          {
            prcCompGood = ha->GetBinContent(4);
            prcCompAll = ha->GetBinContent(2);
            sizeCompGood = ha->GetBinContent(5);
            sizeCompAll = ha->GetBinContent(3);
          }
        }

        TH1F* hl = (TH1F*)gDirectory->Get("hLat");
        if( hl != 0 )
        {
          double mean = hl->GetMean();
          double stdev = hl->GetRMS();
          latencyMean = mean;
          latencyStdev = stdev;
        }

        f->Close();
        delete f;
      }
    }

    vector<double> values;
    values.push_back(nticker_quote);
    values.push_back(nticker_trade);
    values.push_back(nticker_order);
    values.push_back(nticker_cross);
    values.push_back(eff_max_95);
    values.push_back(eff_max_90);
    values.push_back(eff_max_80);
    values.push_back(eff_sum_95);
    values.push_back(eff_sum_90);
    values.push_back(eff_sum_80);
    values.push_back(prcCompGood);
    values.push_back(prcCompAll);
    values.push_back(sizeCompGood);
    values.push_back(sizeCompAll);
    values.push_back(latencyMean);
    values.push_back(latencyStdev);
    values.push_back(bookDepth);
    values.push_back(bookDepthMax);
    values.push_back(totNquotes);
    values.push_back(totNtrades);
    values.push_back(totNorders);

    ps.add( fdate.Data(), values );
  }

  ps.update();
}

vector<double> DrawNtickers::get_eff(TGraph* g)
{
  vector<double> ret(3);

  int n = g->GetN();
  double* x = g->GetX();
  double* y = g->GetY();

  for( int i=0; i<n-1; ++i )
  {
    if( y[i] <= 0.95 )
      ret[0] = x[i];
    if( y[i] <= 0.9 )
      ret[1] = x[i];
    if( y[i] <= 0.8 )
      ret[2] = x[i];
  }

  return ret;
}

void DrawNtickers::draw_canvas(TCanvas* c, TH1F** hist)
{
  c->Clear();
  //c->Divide(1,10);
  c->Divide(2,5);
  int padNum = 0;

//
// ntickers
//
  double histMax = 0;
  double histMin = 0;
  for( int i=0; i<3; ++i )
  {
    double newmax = hist[i]->GetMaximum();
    double newmin = hist[i]->GetMinimum();
    if( newmax > histMax )
      histMax = newmax;
    if( newmin < histMin )
      histMin = newmin;
  }
  for( int i=0; i<3; ++i )
  {
    hist[i]->SetMaximum(histMax * 1.1);
    hist[i]->SetMinimum(histMin * 0.9);
  }

  c->cd(++padNum);
  gPad->SetGrid();
  hist[0]->SetTitle("Number of tickers");
  hist[0]->Draw("LP");
  hist[1]->Draw("LP,same");
  hist[2]->Draw("LP,same");

  TLegend* leg1 = new TLegend(0.9, 0.55, 1.0, 1.0);
  leg1->SetFillColor(10);
  leg1->SetBorderSize(0);
  leg1->AddEntry(hist[0], "Quote", "pl");
  leg1->AddEntry(hist[1], "Trade", "pl");
  leg1->AddEntry(hist[2], "Order", "pl");
  leg1->Draw();

//
// ncrossed
//
  c->cd(++padNum);
  gPad->SetGrid();
  hist[3]->SetTitle("Number of crossed tickers");
  hist[3]->Draw("LP");

//
// tolerance eff max
//
  c->cd(++padNum);
  gPad->SetGrid();
  hist[4]->SetTitle("Max cross tolerance vs. Ticker efficiency");
  hist[4]->GetYaxis()->SetTitle("milliseconds");
  hist[4]->GetYaxis()->SetTitleSize(0.1);
  hist[4]->GetYaxis()->SetTitleOffset(0.4);
  hist[4]->Draw("LP");
  hist[5]->Draw("LP,same");
  hist[6]->Draw("LP,same");

  TLegend* leg2 = new TLegend(0.9, 0.45, 1.0, 1.0);
  leg2->SetFillColor(10);
  leg2->SetBorderSize(0);
  leg2->AddEntry(hist[4], "95%", "pl");
  leg2->AddEntry(hist[5], "90%", "pl");
  leg2->AddEntry(hist[6], "80%", "pl");
  leg2->Draw();

//
// tolerance eff sum
//
  c->cd(++padNum);
  gPad->SetGrid();
  hist[7]->SetTitle("Sum cross tolerance vs. Ticker efficiency");
  hist[7]->GetYaxis()->SetTitle("milliseconds");
  hist[7]->GetYaxis()->SetTitleSize(0.1);
  hist[7]->GetYaxis()->SetTitleOffset(0.4);
  hist[7]->Draw("LP");
  hist[8]->Draw("LP,same");
  hist[9]->Draw("LP,same");

  TLegend* leg3 = new TLegend(0.9, 0.45, 1.0, 1.0);
  leg3->SetFillColor(10);
  leg3->SetBorderSize(0);
  leg3->AddEntry(hist[7], "95%", "pl");
  leg3->AddEntry(hist[8], "90%", "pl");
  leg3->AddEntry(hist[9], "80%", "pl");
  leg3->Draw();

//
// L1 L2
//
  c->cd(++padNum);
  draw_comp(gPad, &hist[10], 0);
  c->cd(++padNum);
  draw_comp(gPad, &hist[12], 1);

//
// latency
//
  c->cd(++padNum);
  gPad->SetGrid();
  hist[14]->SetTitle("#Delta Latency (L1-L2)");
  hist[14]->Draw("LP");
  c->cd(++padNum);
  gPad->SetGrid();
  hist[15]->SetTitle("Stdev #Delta Latency (L1-L2)");
  hist[15]->Draw("LP");

// book depth
  c->cd(++padNum);
  gPad->SetGrid();
  double depth_max = max(hist[16]->GetMaximum(), hist[17]->GetMaximum());
  hist[16]->SetMinimum(0);
  hist[16]->SetMaximum(depth_max * 1.1);
  hist[16]->SetTitle("Average/Max Book Depth");
  hist[16]->Draw("LP");
  hist[17]->Draw("LP, same");

// tot quote/trade/order
  c->cd(++padNum);
  gPad->SetGrid();
  double tot_max = 0;
  for( i=18; i<=20; ++i )
    if( tot_max < hist[i]->GetMaximum() )
      tot_max = hist[i]->GetMaximum();
  hist[18]->SetTitle("total quotes/trades/orders");
  hist[18]->SetMinimum(0);
  hist[18]->SetMaximum(tot_max);
  hist[18]->Draw("LP");
  hist[19]->Draw("LP, same");
  hist[20]->Draw("LP, same");

  TLegend* legT = new TLegend(0.85, 0.45, 0.98, 1.0);
  legT->SetFillColor(10);
  legT->SetBorderSize(0);
  legT->AddEntry(hist[18], "quotes", "pl");
  legT->AddEntry(hist[19], "trades", "pl");
  legT->AddEntry(hist[20], "orders", "pl");
  legT->Draw();

  c->Update();

  return;
}

void DrawNtickers::draw_comp( TPad* pad, TH1F** h, int flag )
{
  pad->SetGrid();

  TGraph* gr[2] = {0, 0};
  double mins[2] = {0, 0};
  double maxs[2] = {0, 0};
  double means[2] = {0, 0};
  gr[0] = hist_to_graph(h[0], mins, maxs, means);
  gr[1] = hist_to_graph(h[1], mins+1, maxs+1, means+1);

  double grmin = min(mins[0], mins[1]) - 0.02;
  double grmax = max(maxs[0], maxs[1]) + 0.02;
  gr[0]->SetMinimum(grmin);
  gr[1]->SetMinimum(grmin);
  gr[0]->SetMaximum(grmax);
  gr[1]->SetMaximum(grmax);

  if( flag == 0 )
    gr[0]->SetTitle("BEST PRICE, L1 vs L2 comparison");
  else if( flag == 1 )
    gr[0]->SetTitle("BEST SIZE, L1 vs L2 comparison");
  gr[0]->Draw("apl");
  gr[1]->Draw("pl");

  char desc[100];
  TLegend* legC = new TLegend(0.7, 0.7, 0.98, 1.0);
  legC->SetFillColor(10);
  legC->SetBorderSize(0);
  sprintf( desc, "good tickers (%d%%)", (int)means[0] );
  legC->AddEntry(gr[0], desc, "pl");
  sprintf( desc, "all tickers (%d%%)", (int)means[1] );
  legC->AddEntry(gr[1], desc, "pl");
  legC->Draw();
  return;
}

TGraph* DrawNtickers::hist_to_graph(TH1F* h, double* mini, double* maxi, double* mean)
{
  vector<double> x;
  vector<double> y;
  double ysum = 0;

  int nbins = h->GetNbinsX();
  for( int b=1; b<=nbins; ++b )
  {
    double bcont = h->GetBinContent(b);
    if( bcont > 0.1 )
    {
      x.push_back(b - 0.5);
      y.push_back(bcont);
      ysum += bcont;
    }
  }
  if( x.size() > 0 )
    *mean = ysum / x.size() * 100;
  else
  {
    x.push_back(0);
    y.push_back(0);
  }

  TGraph* gr = new TGraph( x.size(), &x[0], &y[0] );
  gr->SetLineColor(h->GetLineColor());

  h->SetLineColor(10);
  gr->SetHistogram(h);

  sort(y.begin(), y.end());
  double grmin = 0;
  double grmax = 0;
  if( !y.empty() )
  {
    grmin = y[0];
    grmax = y[y.size()-1];
  }
  *mini = grmin;
  *maxi = grmax;

  gr->SetMarkerStyle(h->GetMarkerStyle());

  return gr;
}
