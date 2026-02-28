#include <string>
#include <map>
#include <set>
#include <vector>
#include <TStyle.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TStopwatch.h>
#include <TRootEmbeddedCanvas.h>
#include <RQ_OBJECT.h>

////////////////////////////////////////////////////////////////////////////////
// Class OrderBookSim
////////////////////////////////////////////////////////////////////////////////
class OrderBookSim {
  RQ_OBJECT("OrderBookSim")

public:
  OrderBookSim();
  OrderBookSim(const TGWindow *p, UInt_t w, UInt_t h);
  virtual ~OrderBookSim();


private:
  TGMainFrame *fMain;
  TRootEmbeddedCanvas *fEcanvas1;
  TRootEmbeddedCanvas *fEcanvas2;

  TGComboBox *fComboFile;
  TGTextEntry *fTextMsecs;

  TH1F* h00;
  TH1F* h01;
  TH1F* hbid;
  TH1F* hask;

  int histLow;
  int histHigh;
  int showPriceLow;
  int showPriceHigh;
  int interval;
  int nbins;
  int nFiles;
  int fNent;
  int fMsecs;
  int fNentries;
  ifstream ifsOrder;
  TString* fFiles;

  vector<int> vMsecs;
  vector<int> vPrice;
  vector<int> vSize;
  vector<int> vType;
  map<int, int> bidvol;
  map<int, int> askvol;

  void dump_book();
  void next_order();
  void open_file();
  void refresh_file();
  void reset_ifsOrder(int);
  void read_an_order();
  bool get_draw_param();
  void draw_book();
  void goto_msecs();
  void jumpto_msecs();
  bool need_new_histogram();
  bool need_new_zoom_range();
};

OrderBookSim::OrderBookSim(){}

OrderBookSim::OrderBookSim(const TGWindow *p, UInt_t w, UInt_t h)
:fNent(0),
interval(1),
showPriceLow(0),
showPriceHigh(0),
h00(0),
h01(0),
hbid(0),
hask(0)
{
  gROOT->SetStyle("Plain");
  gStyle->SetOptStat(0);
  gStyle->SetPadTopMargin(0.2);
  gStyle->SetPadBottomMargin(0.16);
  gStyle->SetPadLeftMargin(0.12);
  gStyle->SetTitleH(0.14);

// ----------------------------------------------------------------
// Part I
// ----------------------------------------------------------------

// Create a main frame
  fMain = new TGMainFrame(p,w,h);

// Create an upper canvas widget
  fEcanvas1 = new TRootEmbeddedCanvas("Ecanvas1",fMain,600,300);
  fMain->AddFrame(fEcanvas1, new TGLayoutHints(kLHintsExpandX| kLHintsExpandY,10,10,10,1));

// Create an bottom canvas widget
  fEcanvas2 = new TRootEmbeddedCanvas("Ecanvas2",fMain,600,300);
  fMain->AddFrame(fEcanvas2, new TGLayoutHints(kLHintsExpandX| kLHintsExpandY,10,10,10,1));

// hframe1
  TGHorizontalFrame *hframe1 = new TGHorizontalFrame(fMain,600,60);

// Combobox File
  fComboFile = new TGComboBox(hframe1,50);
  fComboFile->Connect("Selected(Int_t)", "OrderBookSim", this, "reset_ifsOrder(Int_t)");
  fComboFile->Resize(300,20);

// Exit Button
  TGTextButton *exit = new TGTextButton(hframe1,"&Exit","gApplication->Terminate(0)");

// Dump button
  TGTextButton *bDump = new TGTextButton(hframe1," Dump book ");
  bDump->Connect("Clicked()","OrderBookSim",this,"dump_book()");

// Next tick button
  TGTextButton *bTick = new TGTextButton(hframe1," Next Order ");
  bTick->Connect("Clicked()","OrderBookSim",this,"next_order()");

// Previous tick button
  TGTextButton *bPrev = new TGTextButton(hframe1," Previous Order ");
  bPrev->Connect("Clicked()","OrderBookSim",this,"prev_order()");

// Goto button
  TGTextButton *bGoto = new TGTextButton(hframe1," Go to ");
  bGoto->Connect("Clicked()","OrderBookSim",this,"goto_msecs()");

// Jumpto button
  TGTextButton *bJumpto = new TGTextButton(hframe1," Jump to ");
  bJumpto->Connect("Clicked()","OrderBookSim",this,"jumpto_msecs()");

// Msecs to go to
  fTextMsecs = new TGTextEntry(hframe1, "");

// End hframe1
  hframe1->AddFrame(fComboFile, new TGLayoutHints(kLHintsCenterX,5,5,5,5));
  hframe1->AddFrame(bDump, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  hframe1->AddFrame(bTick, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  hframe1->AddFrame(bPrev, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  hframe1->AddFrame(bGoto, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  hframe1->AddFrame(bJumpto, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  hframe1->AddFrame(fTextMsecs, new TGLayoutHints(kLHintsCenterX,5,70,3,4));
  hframe1->AddFrame(exit, new TGLayoutHints(kLHintsRight,5,5,3,4));
  fMain->AddFrame(hframe1, new TGLayoutHints(kLHintsCenterX,2,2,2,2));

  refresh_file();

// Initialize the class

  //InitVars();

//
// Main Frame...
//
  fMain->SetWindowName("Order Book Sim");
  fMain->MapSubwindows();
  fMain->Resize(fMain->GetDefaultSize());
  fMain->MapWindow();
}

void OrderBookSim::InitVars()
{
  if( h00 != 0 )
    delete h00;
  if( h01 != 0 )
    delete h01;
  if( hbid != 0 )
    delete hbid;
  if( hask != 0 )
    delete hask;

  h00 = new TH1F();
  h01 = new TH1F();
  hbid = new TH1F();
  hask = new TH1F();

  int ms = 0;
  int pr = 0;
  int si = 0;
  int ty = 0;
  if( ifsOrder.is_open() )
  {
    vMsecs.clear();
    vPrice.clear();
    vSize.clear();
    vType.clear();
    int cnt = 0;
    while( !ifsOrder.eof() )
    {
      if( ++cnt > 1e6 )
        break;
      ifsOrder >> ms >> pr >> si >> ty;
      vMsecs.push_back(ms);
      vPrice.push_back(pr);
      vSize.push_back(si);
      vType.push_back(ty);
    }
  }
  fNentries = vMsecs.size();

  reset();

  histLow = 0;
  histHigh = 0;
  showPriceLow = 0;
  showPriceHigh = 0;
  interval = 0;
  nbins = 0;
  nFiles = 0;
  fNentries = 0;

  return;
}

void OrderBookSim::reset()
{
  fNent = 0;
  fMsecs = 0;
  bidvol.clear();
  askvol.clear();

  return;
}

void OrderBookSim::goto_msecs()
{
  int msecs2go = atoi(fTextMsecs->GetText());

  // start timer
  TStopwatch timer;
  timer.Start();

  while( fMsecs < msecs2go )
  {
    read_an_order();
    draw_book();
  }

  timer.Stop();
  double ctime = timer.CpuTime();
  printf("Took %f seconds\n", ctime);

  return;
}

void OrderBookSim::jumpto_msecs()
{
  int msecs2go = atoi(fTextMsecs->GetText());

  // start timer
  TStopwatch timer;
  timer.Start();

  while( fMsecs < msecs2go )
    read_an_order();
  draw_book();

  timer.Stop();
  double ctime = timer.CpuTime();
  printf("Took %f seconds\n", ctime);

  return;
}

void OrderBookSim::next_order()
{
  read_an_order();
  draw_book();

  return;
}

void OrderBookSim::prev_order()
{
  int nent2go = fNent - 1;
  reset();

  while( fNent < nent2go )
    read_an_order();
  draw_book();

  return;
}

void OrderBookSim::read_an_order()
{
  // read an order
  //static int nent = 0;
  int price = 0;
  int size = 0;
  int type = 0;

  fMsecs = vMsecs[fNent];
  price = vPrice[fNent];
  size = vSize[fNent];
  type = vType[fNent];
  ++fNent;

  // update the book
  int bid_ask = (type >> 1) & 1; // 0: bid, 1: ask
  int add_del = type & 1;        // 0: add, 1: del
  if( 0 == bid_ask )
  {
    if( 0 == add_del )
      bidvol[price] += size;
    else if( 1 == add_del )
    {
      int thevol = bidvol[price];
      if( size > thevol )
      {
        //cout << "Cannot delete " << size << " from " << thevol << ". Setting the volume to 0." << endl;
        bidvol[price] = 0;
      }
      else
        bidvol[price] = thevol - size;
    }
  }
  else if( 1 == bid_ask )
  {
    if( 0 == add_del )
      askvol[price] += size;
    else if( 1 == add_del )
    {
      int thevol = askvol[price];
      if( size > thevol )
      {
        //cout << "Cannot delte " << size << " from " << thevol << ". Setting to 0." << endl;
        askvol[price] = 0;
      }
      else
        askvol[price] = thevol - size;
    }
  }

  // remove zero's
  for( map<int, int>::iterator it = bidvol.begin(); it != bidvol.end(); )
    if( it->first == 0 || it->second == 0 )
      bidvol.erase(it++);
    else
      ++it;
  for( map<int, int>::iterator it = askvol.begin(); it != askvol.end(); )
    if( it->first == 0 || it->second == 0 )
      askvol.erase(it++);
    else
      ++it;

  return;
}

bool OrderBookSim::get_draw_param()
{
  // find price range
  int xlow = bidvol.begin()->first;
  if( xlow > askvol.begin()->first && askvol.begin()->first != 0 )
    xlow = askvol.begin()->first;
  int xhigh = askvol.rbegin()->first;
  if( xhigh < bidvol.rbegin()->first )
    xhigh = bidvol.rbegin()->first;
  if( xlow >= xhigh )
  {
    int tempLow = xlow;
    xlow = xhigh;
    xhigh = tempLow;
  }

  // find minimum price interval
  if( bidvol.size() > 0 || askvol.size() > 0 )
  {
    set<int> sPrices;
    for( map<int, int>::iterator it = bidvol.begin(); it != bidvol.end(); ++it )
      sPrices.insert(it->first);
    for( map<int, int>::iterator it = askvol.begin(); it != askvol.end(); ++it )
      sPrices.insert(it->first);
    int maxPrice = *(sPrices.rbegin());
 
    bool stop = false;
    for( interval = 1; interval < maxPrice; interval *= 10 )
    {
      for(set<int>::iterator it2 = sPrices.begin(); it2 != sPrices.end(); ++it2 )
      {
        if( (*it2) % interval != 0 )
        {
          if( interval > 1 )
            interval /= 10;
          stop = true;
          break;
        }
      }
      if( stop )
        break;
    }
    cout << "interval = " << interval << endl;
  }

  histLow = xlow - interval*2;
  histHigh = xhigh + interval*3;
  nbins = (histHigh - histLow) / interval;

  //return nbins > 3;
  return bidvol.size() + askvol.size() > 0;
}

void OrderBookSim::draw_book()
{
  if( !get_draw_param() )
    return;

  if( need_new_histogram() )
  {
    if( h00 != 0 )
      delete h00;
    if( h01 != 0 )
      delete h01;
    if( hbid != 0 )
      delete hbid;
    if( hask != 0 )
      delete hask;

    // Create histograms
    char title0[100];
    sprintf(title0, "%d msecs (%02d:%02d:%02d)", fMsecs, fMsecs/1000/60/60, fMsecs/1000/60%60, fMsecs/1000%60);
    h00 = new TH1F("h00", title0, nbins, histLow, histHigh);
    hbid = new TH1F("hbid", "hbid", nbins, histLow, histHigh);
    hask = new TH1F("hask", "hask", nbins, histLow, histHigh);

    // fill the histograms
    for( map<int, int>::iterator it = bidvol.begin(); it != bidvol.end(); ++it )
    {
      hbid->Fill(it->first, it->second);
      if( it->second > 0 )
      {
        char buf[100];
        sprintf( buf, "%d", it->first );
        int i=0;
        while( h00->GetBinLowEdge(++i) <= it->first );
        if( i > 2 )
          h00->GetXaxis()->SetBinLabel(i-1, buf);
      }
    }
    for( map<int, int>::iterator it = askvol.begin(); it != askvol.end(); ++it )
    {
      hask->Fill(it->first, it->second);
      if( it->second > 0 )
      {
        char buf[100];
        sprintf( buf, "%d", it->first );
        int i=0;
        while( h00->GetBinLowEdge(++i) <= it->first );
        if( i > 2 )
          h00->GetXaxis()->SetBinLabel(i-1, buf);
      }
    }
  }
  else
  {
    // update the title
    char title0[100];
    sprintf(title0, "%d msecs (%02d:%02d:%02d)", fMsecs, fMsecs/1000/60/60, fMsecs/1000/60%60, fMsecs/1000%60);
    h00->SetTitle(title0);

    // fill the histograms
    hbid->Reset("ICE");
    hask->Reset("ICE");
    for( map<int, int>::iterator it = bidvol.begin(); it != bidvol.end(); ++it )
      hbid->Fill(it->first, it->second);
    for( map<int, int>::iterator it = askvol.begin(); it != askvol.end(); ++it )
      hask->Fill(it->first, it->second);

    // label the prices
    for( int i=0; i<nbins; ++i )
    {
      if( hbid->GetBinContent(i+1) != 0 || hask->GetBinContent(i+1) != 0 )
      {
        char buf[100];
        sprintf( buf, "%d", h00->GetBinLowEdge(i) );
        h00->GetXaxis()->SetBinLabel(i+1, buf);
      }
      else
        h00->GetXaxis()->SetBinLabel(i+1, "");
    }
  }

  // determine the price range to zoom in
  int showRangeLow = 1;
  int showRangeHigh = nbins;
  if( need_new_zoom_range() )
  {
    {
      int nonZeroBin = 0;
      for( map<int, int>::iterator it2 = askvol.begin(); it2 != askvol.end(); ++it2 )
      {
        if( it2->second )
        {
          showPriceHigh = it2->first;
          if( ++nonZeroBin > 4 )
            break;
        }
      }
    }
    if( showPriceHigh > 0 && !bidvol.empty() && !askvol.empty() )
      showPriceLow = min(bidvol.rbegin()->first, askvol.begin()->first) - (showPriceHigh - askvol.begin()->first);

    // if crossed, zoom out a little to show the cross
    if( showPriceLow != 0 && !askvol.empty() && showPriceLow > askvol.begin()->first )
      showPriceLow = askvol.begin()->first;
    if( showPriceHigh != 0 && !bidvol.empty() && showPriceHigh < bidvol.rbegin()->first )
      showPriceHigh = bidvol.rbegin()->first;

    // determine the bins ranges to zoom in
    if( showPriceLow != 0 )
      showRangeLow = (showPriceLow - histLow)/interval;
    if( showPriceHigh != 0 )
      showRangeHigh = (showPriceHigh - histLow)/interval + 1;
  }

  double histYmin = min( hbid->GetMinimum(), hask->GetMinimum() );
  double histYmax = max( hbid->GetMaximum(), hask->GetMaximum() )*1.2;
  h00->GetXaxis()->SetLabelSize(0.05);
  h00->SetMinimum(histYmin);
  h00->SetMaximum(histYmax);
  hbid->SetLineColor(2);
  hask->SetLineColor(4);

  TCanvas *fCanvas1 = fEcanvas1->GetCanvas();
  fCanvas1->cd();
  fCanvas1->SetGrid();
  if( h00->GetNbinsX() > 10000 )
  {
    double bbid = 0;
    double bask = 0;
    if( !bidvol.empty() )
      bbid = bidvol.rbegin()->first;
    if( !askvol.empty() )
      bask = askvol.begin()->first;
    double mid = (bbid + bask) / 2.0;
    int mrange = 0;
    int h00nbins = h00->GetNbinsX();
    for( i=1; i<h00nbins; ++i )
      if( h00->GetBinLowEdge(i) <= mid && mid < h00->GetBinLowEdge(i+1) )
      {
        mrange = i;
        break;
      }
    int range1 = max(1, mrange - 100);
    int range2 = min(h00nbins, mrange + 100);
    h00->GetXaxis()->SetRange(range1, range2);
  }
  h00->Draw();
  hbid->Draw("same");
  hask->Draw("same");
  h00->Draw("same");
  fCanvas1->cd();
  fCanvas1->Update();

  // create zoomed histogram
  h01 = (TH1F*)h00->Clone("h01");
  h01->SetTitle(h00->GetTitle());
  h01->GetXaxis()->SetRange(showRangeLow, showRangeHigh);
  double histYmaxZoom = 0;
  for( int i=showRangeLow; i<showRangeHigh; ++i )
  {
    double size = max(hbid->GetBinContent(i), hask->GetBinContent(i));
    if( size > histYmaxZoom )
      histYmaxZoom = size;
  }
  histYmaxZoom *= 1.2;
  h01->SetMaximum(histYmaxZoom);

  TCanvas *fCanvas2 = fEcanvas2->GetCanvas();
  fCanvas2->cd();
  fCanvas2->SetGrid();
  h01->Draw();
  hbid->Draw("same");
  hask->Draw("same");
  h01->Draw("same");
  fCanvas2->cd();
  fCanvas2->Update();

  return;
}

bool OrderBookSim::need_new_histogram()
{
  if( h00->GetNbinsX() < 2 ||
      histLow < h00->GetBinLowEdge(1) ||
      histHigh > h00->GetBinLowEdge(h00->GetNbinsX()+1) ||
      interval < h00->GetBinLowEdge(2) - h00->GetBinLowEdge(1) )
    return true;

  return true;
}

bool OrderBookSim::need_new_zoom_range()
{
  int nBidInPrice = 0;
  int nAskInPrice = 0;

  if( bidvol.size() > 2 )
  {
    for( map<int, int>::iterator it = bidvol.begin(); it != bidvol.end(); ++it )
      if( it->first > 0 && it->first <= showPriceLow && it->first <= showPriceHigh )
        ++nBidInPrice;
  }
  else
    nBidInPrice = 3;

  if( askvol.size() > 2 )
  {
    for( map<int, int>::iterator it = askvol.begin(); it != askvol.end(); ++it )
      if( it->first > 0 && it->first <= showPriceLow && it->first <= showPriceHigh )
        ++nAskInPrice;
  }
  else
    nAskInPrice = 3;

  return !(nBidInPrice > 2 && nAskInPrice > 2 && nBidInPrice < 10 && nAskInPrice < 10);
}

OrderBookSim::~OrderBookSim() {
  if( ifsOrder.is_open() )
    ifsOrder.close();

  fMain->Cleanup();
  delete fMain;
}

void OrderBookSim::refresh_file()
{
  fComboFile->RemoveAll();

  system( "get_file_list.py" );
  ifstream ifs( "temp_files.txt" );
  if( ifs.is_open() )
  {
    ifs >> nFiles;

    if( nFiles > 0 )
    {
      fFiles = new TString[nFiles];

      TString file;
      for( int i=0; i<nFiles; ++i )
      {
        ifs >> file;
        fFiles[i] = file;
        fComboFile->AddEntry( fFiles[i], i+1 );
      }
    }
    ifs.close();
    system( "del temp_files.txt" );
  }

  fComboFile->Select(nFiles);

  return;
}

void OrderBookSim::reset_ifsOrder(int i)
{
  if( ifsOrder.is_open() )
  {
    ifsOrder.close();
    ifsOrder.clear();
  }

  ifsOrder.open( ("./data/" + fFiles[i-1]).Data());

  InitVars();

  return;
}

void OrderBookSim::dump_book()
{
  cout << "\nfMsecs " << fMsecs << "\n";

  cout << "Bid" << endl;
  for( map<int, int>::iterator it = bidvol.begin(); it != bidvol.end(); ++it )
    cout << it->first << "\t" << it->second << endl;

  cout << "Ask" << endl;
  for( map<int, int>::iterator it = askvol.begin(); it != askvol.end(); ++it )
    cout << it->first << "\t" << it->second << endl;

  return;
}
