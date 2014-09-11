/////////////////////////////////////////////////////////////////////
//
// Program to combine the histograms made by the makeHistograms
// program.
// 
// This program will take any number of input histograms files and
// combined them to produce the a final set of histograms.  However,
// if you want meaningful results, you must:
// 1) Combine only files from the same process/multiplicity;
// 2) Have at least one histogram file for each of the four parts
//    (born, loop, real, or vsub);
// 3) Already have the final binning from when the histograms were
//    created in the makeHistograms program.
//
// Joe Haley (jhaley@cern.ch)
/////////////////////////////////////////////////////////////////////

#include <fstream>
#include <iostream>
#include <iomanip>
#include "TCanvas.h"
#include "TFile.h"
#include "TClass.h"
#include "TH1D.h"
#include "TH1I.h"
#include "TMath.h"
#include "TList.h"
#include "TKey.h"
#include <TROOT.h>
#include <TRint.h>
#include <map>
#include <vector>
#include <math.h>
#include <string>

using namespace std;

namespace combineHistograms{
  
  typedef map< string,vector<TFile*> >::iterator m_s_vF_it;
  typedef map< string,TH1* >::iterator m_s_h_it;
  
  void UnnormToBinWidths(TH1* hist){
    for(int i=1; i<=hist->GetNbinsX(); ++i){
      hist->SetBinContent(i,hist->GetBinContent(i)*hist->GetBinWidth(i));
      hist->SetBinError(i,hist->GetBinError(i)*hist->GetBinWidth(i));
    }
  }
  
  //// MAIN ////
  int main(int argc, char* argv[]){
    gROOT->ProcessLine("#include <vector>");

    //// Constants
    const string counts_histname = "h_counts"; // the name of the histogram containing the event counts

    string output_filename = "CombinedHists.root";
    std::vector<std::string> fs;  // Input histogram files

    string arg;
    for(int i=1; i<argc; i++){
      arg = argv[i];
      if(arg == "-outfile"){
	if(argc>i){
	  ++i;
	  output_filename = argv[i];
	}else{
	  return 1;
	}
      }else if(arg.find(".root") != string::npos){
	fs.push_back(argv[i]);
      }else{
	cerr<<"Unknown argument: "<<arg<<endl;
	return 1;
      }
    }

    if(!fs.size()){
      cerr<<"No input files specified!  Aborting."<<endl;
      exit(1);
    }

    cout<<"=================================="<<endl;
    cout<<"output_filename = "<<output_filename<<endl;
    cout<<"=================================="<<endl;
  
    TFile* output_file = new TFile(output_filename.c_str(),"recreate");

    //// Get the true event count for each file and organize the files
    //// into the separate parts

    map< string,vector<TFile*> > files;
    files["born"] = vector<TFile*>();
    files["loop"] = vector<TFile*>();
    files["real"] = vector<TFile*>();
    files["vsub"] = vector<TFile*>();
    map< TFile*,int > truecounts; // the true event count for each file
    TH1I* h_counts=0; // Combined event count histogram

    for(int i=0; i<fs.size(); i++){
      TFile* f=0;
      if( !(f = new TFile(fs[i].c_str(),"open")) ){
	cerr<<"Could not open file: "<<fs[i]<<endl;
	cerr<<"Aborting."<<endl;
	return 2;
      }
      TH1I *h=0;
      if( !(h = (TH1I*)f->Get(counts_histname.c_str())) ){
	// Every input file must have this histogram
	cerr<<"Did not find h_counts in file: "<<fs[i]<<endl;
	cerr<<"Aborting."<<endl;
	return 3;
      }
      
      // Get the true event counts
      int t = h->GetBinContent(2);
      int b = h->GetBinContent(4);
      int r = h->GetBinContent(6);
      int v = h->GetBinContent(8);
      int l = h->GetBinContent(10);
      string part;
      // One of the parts must equal the total and all others must be zero
      if      (b==t && r==0 && v==0 && l==0){
	part="born";
      }else if(b==0 && r==t && v==0 && l==0){
	part="real";
      }else if(b==0 && r==0 && v==t && l==0){
	part="vsub";
      }else if(b==0 && r==0 && v==0 && l==t){
	part="loop";
      }else{
	cerr<<"The event counts in "<<fs[i]<<" indicate that these are not all from one part."<<endl;
	cerr<<"Aborting."<<endl;
	return 4;
      }
      files[part].push_back(f);
      truecounts[f]=t;

      //// Make the combined counts histogram (just a direct sum) ////
      if(h_counts){
	h_counts->Add(h);
      }else{
	cout<<"Creating combined event count histogram "<<counts_histname<<endl;
	output_file->cd();
	h_counts = (TH1I*)h->Clone();
      }
    }
    
    
    //// Get the list of histograms ////
    map< string,TH1* > combinedhists;
    cout<<"Get Keys..."<<endl;
    TList *keylist = truecounts.begin()->first->GetListOfKeys();
    for(int i=0; i<keylist->GetSize(); ++i){

      if( ((TKey*)keylist->At(i))->ReadObj()->IsA()->InheritsFrom( TH1::Class() ) ){ // it's a histogram
	string name = ((TKey*)keylist->At(i))->GetName();
	cout<<"  "<<((TKey*)keylist->At(i))->GetClassName()<<": "<<name<<endl;

	if(name == counts_histname) continue; // the event count histograms are treated differently
	if(name.find("_uncert") != string::npos) continue; // no need for these
	if(name.find("_last_true_wt") != string::npos) continue; // no need for these
	if(name.find("_sum_sqwts") != string::npos) continue; // no need for these

	combinedhists[name]=0; // make entry with null pointer
      }
    }

    //// Make the combined histograms ////
    //// First, combine the histograms for each part.
    //// Then, combine the parts.

    for(m_s_h_it p_s_h = combinedhists.begin(); p_s_h != combinedhists.end(); ++p_s_h){ // iterate through all the histograms
      string name = p_s_h->first;
      TH1* combhist = p_s_h->second;
      
      for( m_s_vF_it p_s_vf = files.begin(); p_s_vf != files.end(); ++p_s_vf ){ // iterate through each part

	string part = p_s_vf->first;
	int sumcount=0; // the total true event count for this part
	TH1* parthist=0; // combined histogram for this part
	
	for( int i=0; i<p_s_vf->second.size(); ++i){ // iterate through all the files for this part
	  
	  TFile* f = p_s_vf->second[i];
	  sumcount += truecounts[f];
	  TH1* h=0; // pointer for current hist
	  
	  if( !(h=(TH1*)f->Get(name.c_str())) ){
	    cerr<<"Did not find histogram "<<name<<" in file "<<f->GetName()<<endl;
	    cerr<<"Aborting."<<endl;
	    return 5;
	  }

	  h->Scale(truecounts[f]); // remove event count normalization before combining
	  
	  if( parthist ){
	    cout<<"   ...adding more "<<part<<endl;
	    parthist->Add(h);
	  }else{
	    cout<<"Creating combined histogram "<<name<<" for part "<<part<<endl;
	    if( !output_file->GetDirectory(part.c_str()) ) output_file->mkdir(part.c_str());
	    output_file->cd(part.c_str());
	    parthist = (TH1*)h->Clone( name.c_str() );
	  }
	} // --> next file for this part
	
	if(!sumcount){
	  cerr<<"sumcount==0 for part "<<part<<endl;
	  //cerr<<"Aborting..."<<endl; return 1;
	  cerr<<"Skipping..."<<endl; continue;
	}
	if(!parthist){
	  cerr<<"Combined histogram "<<name<<" for part "<<part<<" is NULL!"<<endl;
	  cout<<"Aborting..."<<endl; return 1;
	  //cout<<"Skipping..."<<endl; continue;	  
	}

	// normalize the combined part histogram by the summed true event count
	//UnnormToBinWidths(parthist); // Reverse normalization to bin width
	parthist->Scale(1.0/sumcount);

	cout<<"Integral                                                                       "<<parthist->Integral(0,parthist->GetNbinsX()+1)<<" : "<<part<<" "<<name<<endl;

	//// Combine parts into final histograms ////
	if( combhist ){
	  combhist->Add(parthist);
	}else{
	  cout<<"Creating final combined histogram "<<name<<endl;
	  output_file->cd();
	  combhist = (TH1*)parthist->Clone( name.c_str() );
	}
      } // --> next part

      
      bool make_plots=0;
      if( make_plots ){
	vector<TCanvas*> cans;
	cans.push_back( new TCanvas(("can_"+name).c_str(),("can_"+name).c_str()) );
	combhist->Draw(); 
	cans.back()->SaveAs( (name+".png").c_str() );
      }
      
    } // --> next histogram


    //// Save the output file ////
    output_file->Write();
    output_file->Close();

  } // main
  
} // namespace combineHistograms

int main(int argc, char* argv[]){
  return combineHistograms::main(argc, argv);
}

