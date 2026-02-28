class DrawNquotes {
public:
  DrawNquotes(TCanvas* cnq, TH1F** hist, TH1F** histGap, int ndays, int histMax);
};

DrawNquotes::DrawNquotes(TCanvas* cnq, TH1F** hist, TH1F** histGap, int ndays, int histMax)
{
  cnq->Divide(1,ndays);

  for( int i=0; i<ndays; ++i )
  {
    cnq->cd(i+1);
    gPad->SetGrid();
    if( hist[i]->Integral() > 0 )
    {
      hist[i]->SetMaximum(histMax);
      hist[i]->GetXaxis()->SetLabelSize(0.1);
      hist[i]->GetYaxis()->SetLabelSize(0.1);
    }

    // histGap
    histGap[i] = (TH1F*)hist[i]->Clone();
    int nbins = hist[i]->GetNbinsX();
    for( int b=1; b<=nbins; ++b )
    {
      if( hist[i]->GetBinContent(b) > 0 )
        histGap[i]->SetBinContent(b, 0);
      else
        histGap[i]->SetBinContent(b, -999);
    }

    // remove the first and last blocks
    for( int b=1; b<=nbins; ++b )
    {
      if( histGap[i]->GetBinContent(b) < 0 )
        histGap[i]->SetBinContent(b, 0);
      else
        break;
    }
    for( int b=nbins; b>=1; --b )
    {
      if( histGap[i]->GetBinContent(b) < 0 )
        histGap[i]->SetBinContent(b, 0);
      else
        break;
    }
    for( int b=1; b<=nbins; ++b )
    {
      if( histGap[i]->GetBinContent(b) < 0 )
        histGap[i]->SetBinContent(b, 1);
    }

    hist[i]->Draw();

    histGap[i]->Scale(histMax);
    histGap[i]->SetFillColor(2);
    histGap[i]->SetLineColor(2);
    histGap[i]->SetFillStyle(3003);
    histGap[i]->Draw("same");
  }

  return;
}
