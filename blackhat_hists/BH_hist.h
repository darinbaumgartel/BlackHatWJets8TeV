#ifndef BH_HIST_H
#define BH_HIST_H

#include "TH1D.h"
#include "TH1.h"
#include <iostream>

using namespace std;

class BH_hist{

 public:
  TH1D *hist, *sum_sqwts, *last_true_wt;
  
 public:
  BH_hist(string name, string title, int nbins, double bins[]);
  ~BH_hist();

  Int_t Fill(double x, double wt);
  void NormToBinWidths();
  void Scale(double s);
  void UpdateUncertainty();
  void ApplyUncertainty();

};
#endif
