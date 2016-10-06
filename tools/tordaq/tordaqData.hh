#ifndef __TORDAQDATA_HH__
#define __TORDAQDATA_HH__

// some was autogenerated with ROOT's MakeClass()

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>
#include <TH1.h>
#include <iostream>
#include <vector>
#include <string>

class tordaqData {
public :

   //std::vector <std::string> VARNAMES;

   static const int NVT=23;
   
   //static const int FREQUENCY=3846;
   static const int FREQUENCY=10000;
   static const int WFLENGTH=2000;

   TTree          *fChain;
   Int_t           fCurrent;
   Long64_t        record_tsec;
   Long64_t        record_tnsec;
   Float_t         record_data[WFLENGTH];
   TBranch        *b_record;

   TH1* getHisto()
   {
       if (fChain == 0) return NULL;
       fChain->LoadTree(0);
       fChain->GetEntry(0);
       const double tMin=getTime(0);
       fChain->LoadTree(fChain->GetEntriesFast()-1);
       fChain->GetEntry(fChain->GetEntriesFast()-1);
       const double tMax=getTime(WFLENGTH-1);
       const int nBins=(tMax-tMin)/FREQUENCY;
       TH1F *hh=new TH1F(Form("h%d",fChain->GetName()),"",nBins,tMin,tMax);
       for (Long64_t jentry=0; jentry<fChain->GetEntriesFast(); jentry++) {
           if (LoadTree(jentry)) break;
           fChain->GetEntry(jentry);
           for (int ii=0; ii<WFLENGTH; ii++)
           {
               const Double_t tt=getTime(ii);
               hh->SetBinContent(hh->GetXaxis()->FindBin(tt),record_data[ii]);
           }
       }
       return hh;
   }

   Double_t getTime(Long64_t sec,Long64_t nsec,Int_t wfLength,Int_t iSample)
   {
       // Assume the waveform is sampled at wfLength Hz
       // and updated at 1 Hz (i.e. no deadtime).
       // return sec + nsec/1e9 + (Double_t)iSample/wfLength;

       // Consecutive samples are recorded at 3.846 kHz, or once per 260 us.
       // 2000 samples are then reported to EPICS, but only once every 800 ms.
       // So that leaves a contiguous 279 ms of every 1 second with no data.
       return sec + nsec/1e9 + (Double_t)iSample/FREQUENCY;
   }
   
   Double_t getTime(Int_t iSample)
   {
       return getTime(record_tsec,record_tnsec,WFLENGTH,iSample);
   }
   
   tordaqData(TTree *tree) : fChain(0) 
   {
       //for (int ii=1; ii<=NVT; ii++)
       //{
       //    VARNAMES.push_back(Form("VT%d",ii));
       //    if (ii==17) VARNAMES.push_back(Form("VT17_1"));
       //}

       if (tree == 0) {
           TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("torus_20160818_114059.root");
           if (!f || !f->IsOpen()) {
               f = new TFile("torus.root");
           }
           f->GetObject("VT6",tree);
       }
       Init(tree);
   }

   ~tordaqData()
   {
       if (!fChain) return;
       delete fChain->GetCurrentFile();
   }

   Int_t GetEntry(Long64_t entry)
   {
       if (!fChain) return 0;
       return fChain->GetEntry(entry);
   }
   
   Long64_t LoadTree(Long64_t entry)
   {
       if (!fChain) return -5;
       Long64_t centry = fChain->LoadTree(entry);
       if (centry < 0) return centry;
       if (fChain->GetTreeNumber() != fCurrent) {
           fCurrent = fChain->GetTreeNumber();
           Notify();
       }
       return centry;
   }

   void Init(TTree *tree)
   {
       if (!tree) return;
       fChain = tree;
       fCurrent = -1;
       fChain->SetMakeClass(1);
       fChain->SetBranchAddress("record", &record_tsec, &b_record);
       Notify();
   }
   
   void Show(Long64_t entry)
   {
       if (!fChain) return;
       fChain->Show(entry);
   }
   
   inline Bool_t Notify() { return kTRUE; }
   inline Int_t Cut(Long64_t entry) { return 1; }
   
   void Loop()
   {
       if (fChain == 0) return;
       Long64_t nentries = fChain->GetEntriesFast();
       Long64_t nbytes = 0, nb = 0;
       for (Long64_t jentry=0; jentry<nentries;jentry++) {
           Long64_t ientry = LoadTree(jentry);
           if (ientry < 0) break;
           nb = fChain->GetEntry(jentry);   nbytes += nb;
           std::cout<<record_tsec<<" "<<record_tnsec<<" "<<std::endl<<std::endl;
           //for (int ii=0; ii<WFLENGTH; ii++)
           //    std::cout<<record_data[ii]<<" ";
           std::cout<<std::endl;
           getchar();
       }
   }
};
#endif
