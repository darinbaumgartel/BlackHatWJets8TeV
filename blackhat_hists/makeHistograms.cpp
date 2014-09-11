/////////////////////////////////////////////////////////////////////
//
// Program to make histograms for the W+jets analysis from
// Blackhat+Sherpa ntuples provided by the Blackhat authors.
//
// You should only run this program over one "part" of the Blackhat
// samples at a time (born, loop, real, or vsub).  After you have make
// histograms for each part, you should combine the histograms from
// the separate parts using the combineHistograms program to get
// meaningful final histograms.  You can separate each part into as
// many pieces as you like, you just need to make sure that:
// 1) You never run makeHistograms on more than one part at a time;
// 2) You use the final binning from the beginning if you want
//    meaningful statistical uncertianties.
//
// Joe Haley (jhaley@cern.ch)
//
// Modifications for PDF error windows
// Darin Baumgartel (darinb@cern.ch)
/////////////////////////////////////////////////////////////////////

#include "NtuplesReader.h"
#include "BH_hist.h"
#include "Particle.h"
#include "fastjet/ClusterSequence.hh"
#include <iostream>
#include <sstream>
#include <iomanip>
#include "TCanvas.h"
#include "TH1D.h"
#include "TH1I.h"
#include "TFile.h"
#include "TMath.h"
#include <TROOT.h>
#include <TRint.h>
#include <vector>
#include <math.h>
#include <string>
#include <map>

static const int AUTO_MIN_JET_N = -9;
static const string output_filename_default="Histograms.root";
static const string pdfset_default = "CT10.LHgrid";
static const int pdfmember_default = 0;

namespace makeHistograms
{
	using namespace fastjet;
	using namespace std;

	typedef vector<Particle>::iterator  vP_it;
	typedef vector<PseudoJet>::iterator  vPJ_it;
	typedef map<string,BH_hist*>::iterator mhist_it;
	enum mettype {NUS, VISIBLEMCPARTICLES, SELECTEDOBJECTS};

	//// Declare helper functions ////
	void creatBHHists(map<string,BH_hist*> &bhh);
	bool comparePt(const Particle &a, const Particle &b){ return a.Pt()>b.Pt(); }
	double deltaPhi(const PseudoJet &j, const TLorentzVector &v);
	double deltaR(const PseudoJet &j, const TLorentzVector &v);
	int get_min_jet_n_from_filenames(vector<string> fs);
	int print_help(int rval);

	int print_help(int rval=1)
	{
		cout<<"------------------------------------------------------------------------"<<endl
			<<"Required arguments:"<<endl
			<<"  "<<endl
			<<"  List of Blackhat+Sherpa ROOT ntuples."<<endl
			<<"  "<<endl
			<<"Optional arguments:"<<endl
			<<"  "<<endl
			<<"  -out <string> : Histograms will be saved to a file named <string>"<<endl
			<<"      Default: "<<output_filename_default<<endl
			<<"      Aliases: -o, -output, -outfile"<<endl
			<<"  "<<endl
			<<"  -njet <int> : Require at least <int> jets"<<endl
			<<"      Default: automatically determined from name of list file."<<endl
			<<"      Aliases: -nj, -njets"<<endl
			<<"  "<<endl
			<<"  -pdf <string> : Change the PDF set to that give by <string>"<<endl
			<<"      Default: "<<pdfset_default<<endl
			<<"      Aliases: -PDF, -pdfset"<<endl
            <<"  -member <int> : Change the PDF set member to that give by <int>"<<endl
            <<"      Default: "<<pdfmember_default<<endl
            <<"      Aliases: -Mem, -mem"<<endl
			<<"  "<<endl
			<<"  -ren <float> : Change the renormalization scale by the factor <float>"<<endl
			<<"  "<<endl
			<<"  -fac <float> : Change the factorization scale by the factor <float>"<<endl
			<<"------------------------------------------------------------------------"<<endl
			<<endl;
		return rval;
	}

	//// MAIN ////
	int main(int argc, char* argv[])
	{
								 // allow branches of vectors
		gROOT->ProcessLine("#include <vector>");

		//// Configuration Parameters ////
								 //  cteq6ll.LHpdf, cteq6m.LHpdf, CT10.LHgrid, MSTW2008nlo68cl.LHgrid
		string pdfset = pdfset_default;
        int pdfmember = pdfmember_default;
		float changefacscale=1.0;
		float changerenscale=1.0;
		string output_filename = output_filename_default;
		vector<string> fs;		 // Input Blackhat files
								 // will determine cut from file name if not changed
		int min_jet_n = AUTO_MIN_JET_N;
		mettype met_from = VISIBLEMCPARTICLES;
		bool use_only_leading_clep = 1;

		//// Selection Cuts ////
		// Jet cuts
		double R = 0.5;			 // R for jet clustering
								 // deltaR to use for dressing leptons (doesn't actually matter because leptons are from ME)
		double dress_clep_R = 0.1;
		JetDefinition jet_def(antikt_algorithm, R);
		double min_jet_pt = 30;	 // minimum pt of any jet
		double max_jet_eta = 2.4;// maximum eta of any jet
								 // minimum deltaR between and jet and the charged lepton
		double min_dR_jet_clep = 0.5;
		// Other cuts
		double min_clep_pt = 25; // minimum pt of charged lepton
								 // maximum eta of charged lepton
		double max_clep_eta = 2.1;
		double min_MT = 50;		 // minimum MT
		double max_MT = 1e6;	 // maximim MT  Should I have an upper bound at 110 GeV?

		string arg;
		for(int i=1; i<argc; ++i)
		{
			arg = argv[i];

			if(arg == "-o" ||
				arg == "-out" ||
				arg == "-output" ||
				arg == "-outfile" )
			{
				if(argc>i)
				{
					++i;
					output_filename = argv[i];
				}
				else
				{
					cout<<"You did not specify an output file name after "<<arg<<endl;
					return print_help(1);
				}

			}
            else if(arg == "-pdf" ||
				arg == "-PDF" ||
				arg == "-pdfset" )
			{
				if(argc>i)
				{
					++i;
					pdfset = argv[i];
				}
				else
				{
					cout<<"You did not specify a PDF set after "<<arg<<endl;
					return print_help(1);
				}

			}
            else if(arg == "-member" ||
                arg == "-mem" ||
                arg == "-Mem" )
            {
                if(argc>i)
                {
                    ++i;
                    istringstream ss(argv[i]);
                    if((ss >> pdfmember).fail())
                    {
                        return print_help(2);
                    }
                }
                else
                {
                    cout<<"You did not specify a PDF member after "<<arg<<endl;
                    return print_help(1);
                }

            }            
			else if(arg == "-ren" )
			{
				if(argc>i)
				{
					++i;
					istringstream ss(argv[i]);
					if((ss >> changerenscale).fail())
					{
						return print_help(2);
					}
				}
				else
				{
					cout<<"You did not specify a value after "<<arg<<endl;
					return print_help(1);
				}

			}
			else if(arg == "-fac" )
			{
				if(argc>i)
				{
					++i;
					istringstream ss(argv[i]);
					if((ss >> changefacscale).fail())
					{
						return print_help(2);
					}
				}
				else
				{
					cout<<"You did not specify a value after "<<arg<<endl;
					return print_help(1);
				}

			}else if(arg == "-nj" ||
				arg == "-njet" ||
				arg == "-njets" )
			{
				if(argc>i)
				{
					++i;
					min_jet_n = (size_t)atoi(argv[i]);
				}
				else
				{
					cout<<"You did not specify a value after "<<arg<<endl;
					return print_help(1);
				}

			}
			else if(arg.find(".root") != string::npos)
			{
				fs.push_back(argv[i]);

			}
			else
			{
				cerr<<"Unknown argument: "<<arg<<endl;
				return print_help(1);
			}
		}

		if(min_jet_n == AUTO_MIN_JET_N)
		{
			//// Get min_jet_n from the file names and make sure it does not change
			min_jet_n = get_min_jet_n_from_filenames(fs);
			if(min_jet_n<0)
			{
				cout<<"Could not determine jet multiplicity from file names, specify manually with -nj"<<endl;
				return print_help(1);
			}
		}

		cout<<"=================================="<<endl;
		cout<<"output_filename = "<<output_filename<<endl;
		cout<<"min_jet_n = "<<min_jet_n<<endl;
		cout<<"pdfset = "<<pdfset<<endl;
        cout<<"pdfmember (>0 for error set) = "<<pdfmember<<endl;
		cout<<"changerenscale = "<<changerenscale<<endl;
		cout<<"changefacscale = "<<changefacscale<<endl;
		cout<<"=================================="<<endl;

		TFile* OutputFile = new TFile(output_filename.c_str(),"recreate");

		//// Blackhat ntuple reader ////
		NtupleReader r;
		r.setPP();
		r.useProperStats(true);
		r.setPDF(pdfset.c_str());
		r.setPDFmember(pdfmember);		 // PDF member number from LHAPDF
		r.addFiles(fs);

		// for (int ip = 0; ip<5000; ip++)
		// {
		// 	std::cout<<"Setting pdf memeber: "<<ip<<std::endl;
		// 	r.setPDFmember(ip);
		// }

		//// Create histograms ////
		map<string,BH_hist*> bhh;// map of my blackhat hists
		creatBHHists(bhh);

		// Histogram to keep track of event counts.  This histogram is
		// used when combining histograms, so do not change its format
		// unless you also update combineHistograms.cpp accordingly.
		TH1I* h_counts = new TH1I("h_counts","Event counts", 10,0.5,10.5);
								 // total number of events and counter-events processed
		h_counts->GetXaxis()->SetBinLabel(1,"All Total Count");
								 // total number of unique event IDs processed
		h_counts->GetXaxis()->SetBinLabel(2,"All True Count");
		h_counts->GetXaxis()->SetBinLabel(3,"Born Total Count");
		h_counts->GetXaxis()->SetBinLabel(4,"Born True Count");
		h_counts->GetXaxis()->SetBinLabel(5,"Real Total Count");
		h_counts->GetXaxis()->SetBinLabel(6,"Real True Count");
		h_counts->GetXaxis()->SetBinLabel(7,"VSub Total Count");
		h_counts->GetXaxis()->SetBinLabel(8,"VSub True Count");
		h_counts->GetXaxis()->SetBinLabel(9,"Loop Total Count");
		h_counts->GetXaxis()->SetBinLabel(10,"Loop True Count");

		// Histogram to keep track of total cross section
		TH1D* h_xsec = new TH1D("h_xsec","Total Cross Section (fb)", 5, 0.5, 5.5);
		h_xsec->GetXaxis()->SetBinLabel(1,"BEFORE CUTS");
		h_xsec->GetXaxis()->SetBinLabel(2,"AFTER ALL CUTS");
		h_xsec->GetXaxis()->SetBinLabel(3,"After Requiring Lepton");
		h_xsec->GetXaxis()->SetBinLabel(4,"After Requiring Jets");
		h_xsec->GetXaxis()->SetBinLabel(5,"After MT Cuts");

		// Check hists to keep around
		TH1D* h_pt_j1 = new TH1D("h_pt_j1","pT(jet1)",50,0,1000); h_pt_j1->Sumw2();
		TH1D* h_pt_j2 = new TH1D("h_pt_j2","pT(jet2)",50,0,1000); h_pt_j2->Sumw2();

		// counters
		int totalcount = 0;		 // total number of events and counter-events processed
		int truecount = 0;		 // total number of unique event IDs processed
		int lastID = 99;

		while(r.nextEvent())	 // loop over all the input entries
		{

			//// Update total counts ////
			++totalcount;
			h_counts->Fill(1);
			switch(r.getPart())
			{
				case 'B':
					h_counts->Fill(3); break;
				case 'R':
					h_counts->Fill(5); break;
				case 'V':
					h_counts->Fill(7); break;
				case 'I':
					h_counts->Fill(9); break;
				default:
					cerr<<"Unknown event \"part\":"<<r.getPart()<<"  => Aboring!"<<endl;
					return 2;
			}

			if(r.getID() != lastID)
			{
				lastID = r.getID();

				for(mhist_it h=bhh.begin(); h!=bhh.end(); ++h)
				{
								 // reached a new true event, so now we can add in uncertainty from the last true event
					h->second->UpdateUncertainty();
				}

				//// Update true counts ////
				++truecount;
				h_counts->Fill(2);
				switch(r.getPart())
				{
					case 'B':
						h_counts->Fill(4); break;
					case 'R':
						h_counts->Fill(6); break;
					case 'V':
						h_counts->Fill(8); break;
					case 'I':
						h_counts->Fill(10); break;
					default:
						cerr<<"Unknown event \"part\":"<<r.getPart()<<"  => Aboring!"<<endl;
						return 2;
				}
			}

			if(totalcount%10000 == 0) cout<<"Event number: "<<totalcount<<" ("<<truecount<<")"<<endl;

			//double wt = r.getWeight(); // Use Weight for now.  Change to Weight2 when I get special treatment of uncertainties.
			//double wt = r.getWeight2(); // Weight2 with special treatment of uncertainties.
			double wt = r.computeWeight(r.getFacScale()*changefacscale,r.getRenScale()*changerenscale);

			h_xsec->Fill(1,wt);	 // Cross section before cuts

			//// Get particles ////
			vector<Particle> cleps, nus;
			vector<PseudoJet> partons;

			for(int j=0; j < r.getParticleNbr(); ++j)
			{
				int tmpid = r.getPDGcode(j);

								 // e
				if ( abs(tmpid)==11 ||
								 // mu
					abs(tmpid)==13 )
				{

					cleps.push_back(Particle(TLorentzVector(r.getX(j), r.getY(j), r.getZ(j), r.getEnergy(j)), tmpid));

								 // nu_e
				}else if ( abs(tmpid)==12 ||
					abs(tmpid)==14 ||// nu_mu
					abs(tmpid)==16 )// nu_tau
				{

					nus.push_back(Particle(TLorentzVector(r.getX(j), r.getY(j), r.getZ(j), r.getEnergy(j)), tmpid));

				}				 // u,d,c,s,b, or gluons
				else if( abs(tmpid)<=5 || tmpid==21 )
				{

					partons.push_back(PseudoJet(r.getX(j), r.getY(j), r.getZ(j), r.getEnergy(j)));

				}
				else
				{
					cerr<<"ERROR: Unexpected pdgID: "<<tmpid<<endl;
					cerr<<"       Should this particle be included in the jet clustering?!"<<endl;
					cerr<<"       Aborting..."<<endl;
					return tmpid;
				}
			}

			//// Dress the charged leptons ////
			for(vP_it l=cleps.begin(); l!=cleps.end(); ++l)
			{
				for(int j=0; j < r.getParticleNbr(); ++j)
				{
								 // photon
					if( r.getPDGcode(j) == 22 )
					{
						TLorentzVector photon = TLorentzVector(r.getX(j), r.getY(j), r.getZ(j), r.getEnergy(j));
						if( l->DeltaR(photon) <= dress_clep_R )
						{
							(*l) += photon;
						}
					}
				}
			}

			//// Sort and select charged leptons ////
			sort(cleps.begin(), cleps.end(), comparePt);
			if(use_only_leading_clep && cleps.size()>1)
			{
								 // remove all but the first charged leptons
				cleps.erase(cleps.begin()+1,cleps.end());
			}

			for(vP_it l=cleps.begin(); l!=cleps.end(); ++l)
			{
				if( l->Pt() < min_clep_pt )
				{
					// they are sorted by pt, so just remove the rest and done
					cleps.erase(l,cleps.end());
					break;
				}
				if( fabs(l->Eta()) > max_clep_eta )
				{
					cleps.erase(l);
					--l;
				}
			}

								 // require charged lepton that passed cuts
			if(cleps.size() < 1) continue;
			const Particle *clep = &cleps[0];
			h_xsec->Fill(3,wt);

			//// Cluster and sort all jets ////
			ClusterSequence cs(partons, jet_def);
			vector<PseudoJet> jets_all = cs.inclusive_jets(0);
			jets_all = sorted_by_pt(jets_all);

			//// Select jets ////
			vector<PseudoJet> jets;
			for(vPJ_it j=jets_all.begin(); j!=jets_all.end(); ++j)
			{
								 // jet kinematic cuts
				if (fabs(j->eta()) < max_jet_eta && j->perp() > min_jet_pt)
				{
					bool passDeltaR(1);
								 // deltaR cut
					for(vP_it l=cleps.begin(); l!=cleps.end(); ++l)
					{
						if(deltaR(*j,*l) < min_dR_jet_clep)
						{
								 // found a lepton within minimum dR
							passDeltaR=0;
							break;
						}
					}
								 // you are worthy
					if(passDeltaR) jets.push_back(*j);
				}
			}
								 // require the right number of jets
			if(jets.size() < min_jet_n) continue;
			h_xsec->Fill(4,wt);

			//// Calculate MET ////
			float mex(0), mey(0), met(0);
			switch(met_from)
			{
				case NUS:		 //// Sort nus and create MET from nus
					sort(nus.begin(), nus.end(), comparePt);
					for(vP_it nu=nus.begin(); nu!=nus.end(); ++nu)
					{
						mex += nu->Px();
						mey += nu->Py();
					}
					break;
								 //// Make MET from all visible MC particles
				case VISIBLEMCPARTICLES:
					for(int j=0; j < r.getParticleNbr(); ++j)
					{
						int tmpid = r.getPDGcode(j);
								 // nu_e
						if ( abs(tmpid)!=12 &&
								 // nu_mu
							abs(tmpid)!=14 &&
								 // nu_tau
							abs(tmpid)!=16 )
						{
							mex -= r.getX(j);
							mey -= r.getY(j);
						}
					}
					break;
								 //// Make MET from leptons and jets
				case SELECTEDOBJECTS:
					for(vP_it lep=cleps.begin(); lep!=cleps.end(); ++lep)
					{
						mex -= lep->Px();
						mey -= lep->Py();
					}
					for(vPJ_it jet=jets.begin(); jet!=jets.end(); ++jet)
					{
						mex -= jet->px();
						mey -= jet->py();
					}
					break;
				default:
					cerr<<"ERROR: met_from set to unknown value: "<<met_from<<endl;
					cerr<<"       Aborting..."<<endl;
					return -9;
			}
			met = sqrt(mex*mex + mey*mey);

			//// W Selection ////
			Particle WT( TLorentzVector( clep->Px()+mex, clep->Py()+mey, 0, clep->Pt()+met), 24 *( clep->id()/abs(clep->id()) ) );
			if(WT.M() < min_MT) continue;
			if(WT.M() > max_MT) continue;
			h_xsec->Fill(5,wt);

			//// Calculate HT ////
			float ht(0);
			for(vPJ_it jet=jets.begin(); jet!=jets.end(); ++jet)
			{
				ht += jet->perp();
			}

			//// Fill histograms ////

			h_xsec->Fill(2,wt);	 // Cross section after cuts

			if(jets.size()>0) bhh["ptjet1"] -> Fill( jets[0].perp(), wt);
			if(jets.size()>1) bhh["ptjet2"] -> Fill( jets[1].perp(), wt);
			if(jets.size()>2) bhh["ptjet3"] -> Fill( jets[2].perp(), wt);
			if(jets.size()>3) bhh["ptjet4"] -> Fill( jets[3].perp(), wt);

			if(jets.size()>0) bhh["dphijet1muon"] -> Fill( fabs(deltaPhi(jets[0],*clep)), wt);
			if(jets.size()>1) bhh["dphijet2muon"] -> Fill( fabs(deltaPhi(jets[1],*clep)), wt);
			if(jets.size()>2) bhh["dphijet3muon"] -> Fill( fabs(deltaPhi(jets[2],*clep)), wt);
			if(jets.size()>3) bhh["dphijet4muon"] -> Fill( fabs(deltaPhi(jets[3],*clep)), wt);

			if(jets.size()>0) bhh["etajet1"] -> Fill( jets[0].eta(), wt);
			if(jets.size()>1) bhh["etajet2"] -> Fill( jets[1].eta(), wt);
			if(jets.size()>2) bhh["etajet3"] -> Fill( jets[2].eta(), wt);
			if(jets.size()>3) bhh["etajet4"] -> Fill( jets[3].eta(), wt);

			bhh["ht"] -> Fill( ht, wt);
			bhh["ptmet"] -> Fill( met, wt);
			bhh["mt_mumet"] -> Fill( WT.M(), wt);
			bhh["njet_WMuNu"] -> Fill( jets.size(), wt);
			bhh["ptmuon"] -> Fill( clep->Pt(), wt);
			bhh["etamuon"] -> Fill( clep->Eta(), wt);

			// Check histograms
			if(jets.size()>0) h_pt_j1->Fill(jets[0].perp(),wt);
			if(jets.size()>1) h_pt_j2->Fill(jets[1].perp(),wt);

		}
		cout<<"Last event: "<<totalcount<<" ("<<truecount<<")"<<endl;

		for(mhist_it h=bhh.begin(); h!=bhh.end(); ++h)
		{
								 // Add in uncertainty for last true event
			h->second->UpdateUncertainty();
		}

		//// Normalization ////
		if(totalcount != h_counts->GetBinContent(1))
		{
			cerr<<"totalcount != h_counts->GetBinContent(1): "<<totalcount<<" != "<<h_counts->GetBinContent(1)<<endl;
			cout<<"Abort!"<<endl;
			return 3;
		}
		if(truecount != h_counts->GetBinContent(2))
		{
			cerr<<"truecount != h_counts->GetBinContent(2): "<<truecount<<" != "<<h_counts->GetBinContent(2)<<endl;
			cout<<"Abort!"<<endl;
			return 3;
		}

		if(!truecount)
		{
			cerr<<"truecount ==0: "<<truecount<<endl;
			cout<<"Abort!"<<endl;
			return 3;
		}

		for(mhist_it h=bhh.begin(); h!=bhh.end(); ++h)
		{
								 // Normalize to bin width
			h->second->NormToBinWidths();
								 // Normalize to 1fb
			h->second->Scale(1000.0/truecount);
		}

								 // Normalize to 1fb
		h_xsec->Scale(1000.0/truecount);
		h_pt_j1->Scale(1.0/truecount/h_pt_j1->GetBinWidth(0));
		h_pt_j2->Scale(1.0/truecount/h_pt_j2->GetBinWidth(0));

		OutputFile->cd();
		OutputFile->Write();
		OutputFile->Close();
	}							 // end of main()

	//// Function to create all of the BH_Hists
	void creatBHHists(map<string,BH_hist*> &bhh)
	{
		int nbins=0;
		double bins[200];

		nbins=-1;
		bins[++nbins] = 30;
		bins[++nbins] = 60;
		bins[++nbins] = 90;
		bins[++nbins] = 120;
		// bins[++nbins] = 150;
		bins[++nbins] = 180;
		bins[++nbins] = 240;
		bins[++nbins] = 300;
		bins[++nbins] = 360;
		bins[++nbins] = 420;
		bins[++nbins] = 540;
		bins[++nbins] = 660;
		bins[++nbins] = 840;
		// bins[++nbins] = 1140;
		bins[++nbins] = 1680;
		bhh["ht"] = new BH_hist( "ht", "HT", nbins, bins );

		nbins=-1;
		bins[++nbins] = 30;
		bins[++nbins] = 50;
		bins[++nbins] = 70;
		bins[++nbins] = 90;
		bins[++nbins] = 110;
		bins[++nbins] = 150;
		bins[++nbins] = 190;
		bins[++nbins] = 250;
		bins[++nbins] = 310;
		bins[++nbins] = 430;
		bins[++nbins] = 850;
		bhh["ptjet1"] = new BH_hist( "ptjet1", "pT(jet1)", nbins, bins );

		nbins=-1;
		bins[++nbins] = 30;
		bins[++nbins] = 50;
		bins[++nbins] = 70;
		bins[++nbins] = 90;
		bins[++nbins] = 110;
		bins[++nbins] = 150;
		bins[++nbins] = 190;
		bins[++nbins] = 250;
		bins[++nbins] = 550;
		bhh["ptjet2"] = new BH_hist( "ptjet2", "pT(jet2)", nbins, bins );

		nbins=-1;
		bins[++nbins] = 30;
		bins[++nbins] = 50;
		bins[++nbins] = 70;
		bins[++nbins] = 90;
		bins[++nbins] = 110;
		bins[++nbins] = 150;
		bins[++nbins] = 210;
		bins[++nbins] = 450;
		bhh["ptjet3"] = new BH_hist( "ptjet3", "pT(jet3)", nbins, bins );

		nbins=-1;
		bins[++nbins] = 30;
		bins[++nbins] = 50;
		bins[++nbins] = 70;
		bins[++nbins] = 90;
		bins[++nbins] = 350;
		bhh["ptjet4"] = new BH_hist( "ptjet4", "pT(jet4)", nbins, bins );

		nbins=-1;
		bins[++nbins] = 20;
		bins[++nbins] = 0;
		bins[++nbins] = 3.1415927;
		nbins=-1;
		bhh["dphijet1muon"] = new BH_hist( "dphijet1muon", "dPhi(jet1,mu)", nbins, bins );

		nbins=-1;
		bins[++nbins] = 15;
		bins[++nbins] = 0;
		bins[++nbins] = 3.1415927;
		nbins=-1;
		bhh["dphijet2muon"] = new BH_hist( "dphijet2muon", "dPhi(jet2,mu)", nbins, bins );

		nbins=-1;
		bins[++nbins] = 10;
		bins[++nbins] = 0;
		bins[++nbins] = 3.1415927;
		nbins=-1;
		bhh["dphijet3muon"] = new BH_hist( "dphijet3muon", "dPhi(jet3,mu)", nbins, bins );

		nbins=-1;
		bins[++nbins] = 6;
		bins[++nbins] = 0;
		bins[++nbins] = 3.1415927;
		nbins=-1;
		bhh["dphijet4muon"] = new BH_hist( "dphijet4muon", "dPhi(jet4,mu)", nbins, bins );

		nbins=-1;
		bins[++nbins] = 0;
		bins[++nbins] = 10;
		bins[++nbins] = 20;
		bins[++nbins] = 30;
		bins[++nbins] = 40;
		bins[++nbins] = 50;
		bins[++nbins] = 60;
		bins[++nbins] = 70;
		bins[++nbins] = 80;
		bins[++nbins] = 90;
		bins[++nbins] = 100;
		bins[++nbins] = 115;
		bins[++nbins] = 130;
		bins[++nbins] = 150;
		bins[++nbins] = 170;
		bins[++nbins] = 200;
		bins[++nbins] = 250;
		bins[++nbins] = 400;
		bhh["ptmet"] = new BH_hist( "ptmet", "MET", nbins, bins );

		nbins=-1;
		bins[++nbins] = 50;
		bins[++nbins] = 55;
		bins[++nbins] = 60;
		bins[++nbins] = 65;
		bins[++nbins] = 70;
		bins[++nbins] = 75;
		bins[++nbins] = 80;
		bins[++nbins] = 85;
		bins[++nbins] = 90;
		bins[++nbins] = 95;
		bins[++nbins] = 100;
		bins[++nbins] = 110;
		bins[++nbins] = 120;
		bins[++nbins] = 130;
		bins[++nbins] = 145;
		bins[++nbins] = 160;
		bins[++nbins] = 180;
		bins[++nbins] = 200;
		bins[++nbins] = 250;
		bhh["mt_mumet"] = new BH_hist( "mt_mumet", "MT(mu,MET)", nbins, bins );

		nbins=-1;
		bins[++nbins] = 0;
		bins[++nbins] = 1;
		bins[++nbins] = 2;
		bins[++nbins] = 3;
		bins[++nbins] = 4;
		bins[++nbins] = 5;
                bins[++nbins] = 6;
                bins[++nbins] = 7;

		bhh["njet_WMuNu"] = new BH_hist( "njet_WMuNu", "JetCount", nbins, bins );

		nbins=-1;
		bins[++nbins] = 24;
		bins[++nbins] = -2.4;
		bins[++nbins] = 2.4;
		nbins=-1;
		bhh["etajet1"] = new BH_hist( "etajet1", "Eta(jet1)", nbins, bins );

		nbins=-1;
		bins[++nbins] = 24;
		bins[++nbins] = -2.4;
		bins[++nbins] = 2.4;
		nbins=-1;
		bhh["etajet2"] = new BH_hist( "etajet2", "Eta(jet2)", nbins, bins );

		nbins=-1;
		bins[++nbins] = 8;
		bins[++nbins] = -2.4;
		bins[++nbins] = 2.4;
		nbins=-1;
		bhh["etajet3"] = new BH_hist( "etajet3", "Eta(jet3)", nbins, bins );

		nbins=-1;
		bins[++nbins] = 6;
		bins[++nbins] = -2.4;
		bins[++nbins] = 2.4;
		nbins=-1;
		bhh["etajet4"] = new BH_hist( "etajet4", "Eta(jet4)", nbins, bins );

		nbins=-1;
		bins[++nbins] = 25;
		bins[++nbins] = 30;
		bins[++nbins] = 35;
		bins[++nbins] = 40;
		bins[++nbins] = 45;
		bins[++nbins] = 50;
		bins[++nbins] = 55;
		bins[++nbins] = 60;
		bins[++nbins] = 70;
		bins[++nbins] = 80;
		bins[++nbins] = 90;
		bins[++nbins] = 100;
		bins[++nbins] = 115;
		bins[++nbins] = 130;
		bins[++nbins] = 145;
		bins[++nbins] = 160;
		bins[++nbins] = 180;
		bins[++nbins] = 200;
		bins[++nbins] = 230;
		bins[++nbins] = 260;
		bins[++nbins] = 300;
		bhh["ptmuon"] = new BH_hist( "ptmuon", "pT(mu)", nbins, bins );

		nbins=-1;
		bins[++nbins] = 42;
		bins[++nbins] = -2.1;
		bins[++nbins] = 2.1;
		nbins=-1;
		bhh["etamuon"] = new BH_hist( "etamuon", "Eta(mu)", nbins, bins );

	}

	double deltaPhi(const PseudoJet &j, const TLorentzVector &v)
	{
		return TVector2::Phi_mpi_pi(j.phi() - v.Phi());
	}

	double deltaR(const PseudoJet &j, const TLorentzVector &v)
	{
		double deta(j.eta() - v.Eta());
		double dphi(deltaPhi(j,v));
		return sqrt(dphi*dphi + deta*deta);
	}

	int get_min_jet_n_from_filenames(vector<string> fs)
	{
		// this is not the most robust way to do this, but it should work for now
		int min_jet_n = AUTO_MIN_JET_N;
		for(int i=0; i<fs.size(); ++i)
		{
			if( fs[i].find("0j_")!=string::npos)
			{
				if(min_jet_n != 0 && min_jet_n != AUTO_MIN_JET_N)
				{
					cerr<<""<<endl;
					return -1;
				}
				else
				{
					min_jet_n = 0;
				}
			}
			if( fs[i].find("1j_")!=string::npos)
			{
				if(min_jet_n != 1 && min_jet_n != AUTO_MIN_JET_N)
				{
					cerr<<""<<endl;
					return -1;
				}
				else
				{
					min_jet_n = 1;
				}
			}
			if( fs[i].find("2j_")!=string::npos)
			{
				if(min_jet_n != 2 && min_jet_n != AUTO_MIN_JET_N)
				{
					cerr<<""<<endl;
					return -1;
				}
				else
				{
					min_jet_n = 2;
				}
			}
			if( fs[i].find("3j_")!=string::npos)
			{
				if(min_jet_n != 3 && min_jet_n != AUTO_MIN_JET_N)
				{
					cerr<<""<<endl;
					return -1;
				}
				else
				{
					min_jet_n = 3;
				}
			}
			if( fs[i].find("4j_")!=string::npos)
			{
				if(min_jet_n != 4 && min_jet_n != AUTO_MIN_JET_N)
				{
					cerr<<""<<endl;
					return -1;
				}
				else
				{
					min_jet_n = 4;
				}
			}
			if( fs[i].find("5j_")!=string::npos)
			{
				if(min_jet_n != 5 && min_jet_n != AUTO_MIN_JET_N)
				{
					cerr<<""<<endl;
					return -1;
				}
				else
				{
					min_jet_n = 5;
				}
			}

		}
		return min_jet_n;
	}
}								 // namespace makeHistograms


int main(int argc, char* argv[])
{
	return makeHistograms::main(argc, argv);
}
