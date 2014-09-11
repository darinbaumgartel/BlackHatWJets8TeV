#include "BH_hist.h"

// A wrapper class for the histograms to help with the special
// treatment required for the statistical uncertainties.  In addition
// to the main histogram, it uses a separate histogram to keep track
// of the sum of the sqares of the "true" weights for determining the
// correct final uncertainty on each bin.  It is because of thise
// special treatment that the histograms must have the final binning
// from the beginning.

BH_hist::BH_hist(string name, string title, int nbins, double bins[]):
  hist(0), sum_sqwts(0), last_true_wt(0){

  if(nbins>0){
    hist = new TH1D(name.c_str(), title.c_str(), nbins, bins );
    sum_sqwts = new TH1D((name+"_sum_sqwts").c_str(), (title+" Squared Uncertainty").c_str(), nbins, bins );
    last_true_wt = new TH1D((name+"_last_true_wt").c_str(), (title+" Last True Event Weight").c_str(), nbins, bins );
  }else{  // bins = {nbins, min, max}
    hist = new TH1D(name.c_str(), title.c_str(), bins[0],bins[1],bins[2] );
    sum_sqwts = new TH1D((name+"_sum_sqwts").c_str(), (title+" Squared Uncertainty").c_str(), bins[0],bins[1],bins[2] );
    last_true_wt = new TH1D((name+"_last_true_wt").c_str(), (title+" Last True Event Weight").c_str(), bins[0],bins[1],bins[2] );
  }
}

BH_hist::~BH_hist(){
  if(hist) delete hist;
  if(sum_sqwts) delete sum_sqwts;
  if(last_true_wt) delete last_true_wt;
}

Int_t BH_hist::Fill(double x, double wt){
  last_true_wt->Fill(x,wt);
  return hist->Fill(x,wt);
}

void BH_hist::UpdateUncertainty(){
  Double_t *sqwts = sum_sqwts->GetArray();
  Double_t *newwt = last_true_wt->GetArray();
  for(int i=0; i<=sum_sqwts->GetNbinsX()+1; ++i){
    sqwts[i] += newwt[i]*newwt[i]; // add squared weight of last event into sum of squared weights
    newwt[i] = 0; // zero last_true_wt histogram
  }
}

void BH_hist::ApplyUncertainty(){
  hist->Sumw2(); // create sumw2 array
  Double_t* sumw2 = hist->GetSumw2()->GetArray(); // get sumw2 array for hist
  Double_t *sqwts = sum_sqwts->GetArray(); // set sum of squared weights we have been keeping
  for(int i=0; i<=sum_sqwts->GetNbinsX()+1; ++i){
    sumw2[i] = sqwts[i];
  }
}

void BH_hist::NormToBinWidths(){
  for(int i=1; i<=hist->GetNbinsX(); ++i){
    hist->SetBinContent(i,hist->GetBinContent(i)/hist->GetBinWidth(i));
    hist->SetBinError(i,hist->GetBinError(i)/hist->GetBinWidth(i));
  }
}

void BH_hist::Scale(double s){
  hist->Scale(s);
  sum_sqwts->Scale(s*s);
  last_true_wt->Scale(s);
}
