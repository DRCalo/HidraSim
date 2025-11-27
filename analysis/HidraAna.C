//**************************************************
// \file HidraAna.C
// \brief:  analysis skeleton for HidraSim ntuples
// \author: Giacomo Polesello (INFN Pavia) 
//          Edoardo Proserpio (Uni Insubria)
//          Andrea Pareti (UniPV and INFN Pavia, andrea.pareti@cern.ch)
// \start date: May 3, 2022
//**************************************************
//
////usage: root -l -b -q 'HidraAna.C(energy,"filename")'
///  where energy is the energy of the beam, and filename
//   the name of the data ntuple
//
//   It produces an histogram file 
//   Hidra+"energy"+.root
//
#include <TTree.h>
#include <TFile.h>
#include <iostream>
#include <array>
#include <stdint.h>
#include <string>
#include <fstream>
#include <string>
#include <cstring>
// include file with geometry of module
#include "HidraGeo.h"


// Scintillator on even rows (start from 0), Cerenkov on uneven rows
// row ID input ranges from 0 to 31 
// need to know fiber type to distinguish global row
const unsigned int grouping = 8;
void GetSiPMcoordinate(int TowID, int rowID, int colID_original, double &SiPM_X, double &SiPM_Y, std::string fiber, unsigned int grouping)
{

    double TowerOffsetY = -( (NoModulesSiPM-1)*moduleY )/2 + TowID*moduleY;
    // after grouping, there are nfibercolumns/grouping channels
    unsigned int channel = static_cast<unsigned int>(colID_original/grouping);
    // colID should be such that the corresponding coordinate is in the middle of the channel,
    // between 0 and grouping-1 -> For 8 fibre grouping should be 3.5 (cast it to a double)
    double colID = (static_cast<double>(grouping)-1)/2 + channel*grouping;
    
    if(fiber == "S"){                                                         
        SiPM_X = +moduleX/2 - tuberadius - (tuberadius*2)*colID;
        SiPM_Y = -moduleY/2 + tuberadius + (sq3*tuberadius)*rowID+tuberadius*(2.*sq3m1-1.);
        //std::cout << "S X: " << SiPM_X << " Y: " << SiPM_Y << std::endl;
    }    

    if(fiber == "C"){                                                         
        SiPM_X = +moduleX/2 - tuberadius - tuberadius - (tuberadius*2)*colID ;
        SiPM_Y = -moduleY/2 + tuberadius + (sq3*tuberadius)*rowID+tuberadius*(2.*sq3m1-1.);
        //std::cout << "C X: " << SiPM_X << " Y: " << SiPM_Y << std::endl;
    }

    SiPM_Y = TowerOffsetY + SiPM_Y;  
}





void HidraAna(double energy, const string intup){
//Open ntuples
  string infile = "../build/"+intup;
  std::cout<<"Using file: "<<infile<<std::endl;
  char cinfile[infile.size() + 1];
  strcpy(cinfile, infile.c_str());
  auto simfile = new TFile(cinfile, "READ");
  auto *simtree = (TTree*)simfile->Get( "DREMTubesout" );
  std::cout << "\n Test 0 ongoing... \n" << std::endl;

  std::ostringstream os;
  os << energy;
  std::string enstr = os.str();
  string outfile="hidra"+enstr+".root";
  TFile f(outfile.c_str(), "RECREATE");
  std::cout << "\n Test 1 ongoing... \n" << std::endl;

//
//  build vectors with row and column position of each of
//  the MiniModules
//
  int modcol[NofModulesX*NofModulesY];
  int modrow[NofModulesX*NofModulesY];
  for(int i=0;i<NofModulesX*NofModulesY;i++){
    int row=i/NofModulesX;
    int col=i%NofModulesX;
    int imod=modflag[i];
    if(imod>=0){
      modcol[imod]=col;
      modrow[imod]=row;
    }
  }
  std::cout << "\n Test 2 ongoing... \n" << std::endl;
// book histograms  
  double bmin=energy-0.4*sqrt(energy)*10.;
  double bmax=energy+0.4*sqrt(energy)*10.;
  auto sciene = new TH1F("sciene", "sciene",100,bmin,bmax);
  auto cerene = new TH1F("cerene", "cerene",100,bmin,bmax);
  auto totene = new TH1F("totene", "totene",100,bmin,bmax);
  auto totenec = new TH1F("totenec", "totenec",100,bmin,bmax);
  auto totdep = new TH1F("totdep", "totdep",100,0.,bmax);
  auto leakene = new TH1F("leakene", "leakene",100,0.,0.1);
  auto chidist = new TH1F("chidist", "chidist",100,0.,1.);
  auto mapcalo  = new TH2F("mapcalo", "mapcalo",NofModulesX,0.,NofModulesX,NofModulesY,0.,NofModulesY);
  auto SipmMapS = new TH2F("SipmMapS", "SipmS; Col; Row", NofSiPMTowersX*NofFiberscolumn, 0, NofSiPMTowersX*NofFiberscolumn, NofSiPMTowersY*NofFibersrow/2, 0, NofSiPMTowersY*NofFibersrow);
  auto SipmMapC = new TH2F("SipmMapC", "SipmC; Col; Row", NofSiPMTowersX*NofFiberscolumn, 0, NofSiPMTowersX*NofFiberscolumn, NofSiPMTowersY*NofFibersrow/2, 0, NofSiPMTowersY*NofFibersrow);

  auto SciSiPMCoordinates = new TH2F("SciSiPMCoordinates", "Sci SiPM Coordinates; X [mm]; Y[mm]",  NofSiPMTowersX*NofFiberscolumn/grouping, -NofSiPMTowersX*moduleX/2, NofSiPMTowersX*moduleX/2, NofSiPMTowersY*NofFibersrow/2, -NofSiPMTowersY*moduleY/2, NofSiPMTowersY*moduleY/2);
  auto CerSiPMCoordinates = new TH2F("CerSiPMCoordinates", "Cer SiPM Coordinates; X [mm]; Y[mm]",  NofSiPMTowersX*NofFiberscolumn/grouping, -NofSiPMTowersX*moduleX/2, NofSiPMTowersX*moduleX/2, NofSiPMTowersY*NofFibersrow/2, -NofSiPMTowersY*moduleY/2, NofSiPMTowersY*moduleY/2);

  int nentries=simtree->GetEntries();
  std::cout<<"Entries "<<nentries<<std::endl;

//Allocate branch pointers
  int pdg; simtree->SetBranchAddress( "PrimaryPDGID", &pdg );
  double venergy; simtree->SetBranchAddress( "PrimaryParticleEnergy", &venergy );
  double lenergy; simtree->SetBranchAddress( "EscapedEnergyl", &lenergy );
  double denergy; simtree->SetBranchAddress( "EscapedEnergyd", &denergy );
  double edep; simtree->SetBranchAddress( "EnergyTot", &edep );
  double Stot; simtree->SetBranchAddress( "NofPMTScinDet", &Stot );
  double Ctot; simtree->SetBranchAddress( "NofPMTCherDet", &Ctot );
  double PSdep; simtree->SetBranchAddress( "PSEnergy", &PSdep );
  double beamX; simtree->SetBranchAddress( "PrimaryX", &beamX );
  double beamY; simtree->SetBranchAddress( "PrimaryY", &beamY );
  vector<double>* TowerE = NULL; 
  simtree->SetBranchAddress( "VecTowerE", &TowerE );
  vector<double>* SPMT = NULL; 
  simtree->SetBranchAddress( "VecSPMT", &SPMT );
  vector<double>* CPMT = NULL; 
  simtree->SetBranchAddress( "VecCPMT", &CPMT );
  vector<double>* SSiPM = NULL; 
  simtree->SetBranchAddress( "VectorSignals", &SSiPM );
  vector<double>* CSiPM = NULL; 
  simtree->SetBranchAddress( "VectorSignalsCher", &CSiPM );
// 
  double chi=0.38;   
  //double sciPheGeV=217.501;
  //double cerPheGeV=54.1621;
  double sciPheGeV=125.501;
  double cerPheGeV=29.6;  
  double elcont=1.005;
  double picont=1.028;
// Loop on events 
  for( unsigned int i=0; i<simtree->GetEntries(); i++){
    double ecalo=energy-lenergy/1000;
    simtree->GetEntry(i);
    double totsci=0.;
    double totcer=0.;
    double tottow=0.;
// Sum energy over all MiniModules
    for(unsigned int j=0; j<SPMT->size(); j++){
      totsci+=SPMT->at(j)/sciPheGeV;
      totcer+=CPMT->at(j)/cerPheGeV;
      tottow+=TowerE->at(j);
      mapcalo->Fill(modcol[j],modrow[j],TowerE->at(j)/1000/nentries);
    }

   for(unsigned int N=0; N<SSiPM->size(); N++){        // Loop over SiPMs - S Fibers
      double content = SSiPM->at(N)/sciPheGeV;
      totsci+=content;
      unsigned int towID = static_cast<unsigned int>( N/(NofFiberscolumn*NofFibersrow/2) );
      unsigned int SiPMID = N%(NofFiberscolumn*NofFibersrow/2);
      unsigned int colID = static_cast<unsigned int>(SiPMID/(NofFibersrow/2));
      unsigned int rowID = 2*static_cast<unsigned int>(SiPMID%(NofFibersrow/2)); 
      // Get coordinate
      double SiPM_X, SiPM_Y;
      GetSiPMcoordinate(towID, rowID, colID, SiPM_X, SiPM_Y, "S", grouping);
      //std::cout << towID << "\tSiPM ID: " << SiPMID << "\tcolID: " << colID << "\trowID: " << rowID << "\tX: " << SiPM_X << "\tY: " << SiPM_Y << std::endl;
      //std::cout << "Tower: " << towID << "\tModule row: " << modrow[towID] << "\tSiPMID: " << SiPMID << "\tColumn: " << colID << "\tRow: " << rowID << "\tTotal row: " << towID*NofFibersrow+rowID << std::endl;
      //SipmMapS->Fill( modcol[towID]*NofFiberscolumn + colID, modrow[towID]*NofFibersrow+rowID, content); 
      SipmMapS->Fill( colID, towID*NofFibersrow+rowID, content); 
      SciSiPMCoordinates->Fill(SiPM_X, SiPM_Y, content);
    }

   for(unsigned int N=0; N<CSiPM->size(); N++){        // Loop over SiPMs - S Fibers
      double content = CSiPM->at(N)/cerPheGeV;
      totcer+=content;
      unsigned int towID = static_cast<unsigned int>( N/(NofFiberscolumn*NofFibersrow/2) );
      unsigned int SiPMID = N%(NofFiberscolumn*NofFibersrow/2);
      unsigned int colID = static_cast<unsigned int>(SiPMID/(NofFibersrow/2));
      unsigned int rowID = 2*static_cast<unsigned int>(SiPMID%(NofFibersrow/2)) + 1; // Cerenkov fibres on odd rows 
      SipmMapC->Fill( colID, towID*NofFibersrow+rowID, content); 
      double SiPM_X, SiPM_Y;
      GetSiPMcoordinate(towID, rowID, colID, SiPM_X, SiPM_Y, "C", grouping);
      CerSiPMCoordinates->Fill(SiPM_X, SiPM_Y, content);     

    }

    sciene->Fill(totsci);        
    cerene->Fill(totcer);        
    totene->Fill(elcont*0.5*(totsci+totcer));   
    totenec->Fill(picont*(totsci-chi*totcer)/(1-chi));   
    totdep->Fill(tottow/1000.);   
    leakene->Fill(lenergy/1000/energy);   
    chidist->Fill((totsci-ecalo)/(totcer-ecalo));    
    //std::cout << "totSci: " << totsci << "\t totCer: " << totcer << std::endl;
    //break;
  }
  

  /*
  totenec->Fit("gaus","Q","");
  TF1 *fit1 = totenec->GetFunction("gaus");
  double peak1=fit1->GetParameter(1);
  double epeak1=fit1->GetParError(1);
  double rms1=fit1->GetParameter(2);
  double erms1=fit1->GetParError(2);
  //cout << " # " << energy << " " << peak1 << " " << epeak1 << " " << rms1 << " " << erms1 << endl;
  */
  f.Write();
  //

}

//**************************************************
