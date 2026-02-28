////////////////////////////////////////////////////////////////////////////////
// Class PlotSeries
////////////////////////////////////////////////////////////////////////////////

class PlotSeries {
public:
  PlotSeries();
  PlotSeries(vector<string> varNames);
  ~PlotSeries();

  void add(string date, vector<double>& val);
  void update();
  TH1F* get_hist(int n);
  void set_varnames(vector<string>& varnames);
  int get_nhist();

private:
  int nvars_;
  int ndays_;

  vector<string> varNames_;
  vector<string> vDates;
  vector<double>* vVal;
  TH1F** hist_;
};

PlotSeries::PlotSeries()
:ndays_(0)
{}

PlotSeries::PlotSeries(vector<string> varNames)
:ndays_(0),
nvars_(varNames.size()),
varNames_(varNames)
{
  hist_ = new TH1F*[nvars_];
  vVal = new vector<double>[nvars_];
}

PlotSeries::~PlotSeries()
{
  delete [] vVal;
  for( int i=0; i<nvars_; ++i )
    delete hist_[i];
  delete [] hist_;
}

int PlotSeries::get_nhist()
{
  return nvars_;
}

void PlotSeries::set_varnames(vector<string>& varnames)
{
  varNames_ = varnames;
  nvars_ = varNames_.size();
  hist_ = new TH1F*[nvars_];
  vVal = new vector<double>[nvars_];
  return;
}

void PlotSeries::add(string date, vector<double>& val)
{
  vDates.push_back( date );
  for( int i=0; i<nvars_; ++i )
    vVal[i].push_back( val[i] );
  return;
}

void PlotSeries::update()
{
  ndays_ = vDates.size();

  for( int i=0; i<nvars_; ++i )
  {
    char name[100];
    sprintf( name, "h_ser_%s", varNames_[i].c_str() );
    hist_[i] = new TH1F( name, name, ndays_, 0, ndays_ );
  }

  for( int b=0; b<ndays_; ++b )
  {
    for( int i=0; i<nvars_; ++i )
    {
      hist_[i]->SetBinContent(b+1, vVal[i][b]);
      hist_[i]->GetXaxis()->SetBinLabel(b+1, vDates[b].c_str());
    }
  }

  for( int i=0; i<nvars_; ++i )
  {
    hist_[i]->GetXaxis()->SetLabelSize(0.1);
    hist_[i]->GetYaxis()->SetLabelSize(0.1);
    hist_[i]->SetMarkerStyle((i%7)+22);
    hist_[i]->SetLineColor((i%4+1)*2);
  }

  return;
}

TH1F* PlotSeries::get_hist(int n)
{
  return hist_[n];
}
