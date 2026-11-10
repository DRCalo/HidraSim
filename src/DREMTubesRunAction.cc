//**************************************************
// \file DREMTubesRunAction.cc 
// \brief: Implementation of DREMTubesRunAction class 
// \author: Lorenzo Pezzotti (CERN EP-SFT-sim) @lopezzot
// \start date: 7 July 2021
//**************************************************

//Includers from project files
//
#include "DREMTubesRunAction.hh"
#include "DREMTubesEventAction.hh"

//Includers from Geant4
//
//#include "g4root.hh"
#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

//Includers from C++
//
#include <string>

//Define constructor
//
DREMTubesRunAction::DREMTubesRunAction( DREMTubesEventAction* eventAction )
    : G4UserRunAction(),
      fEventAction( eventAction ){ 
  
    //print event number per each event (default, can be overwritten with macro)
    //
    G4RunManager::GetRunManager()->SetPrintProgress(1);     

    //Instantiate analysis manager
    //
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetVerboseLevel( 1 );
    analysisManager->SetNtupleMerging( 1 );
    analysisManager->SetBasketEntries(100);

    //Using ROOT as analysisManager type, print it
    //
    G4cout << "DREMTubes-> Using " << analysisManager->GetType() << G4endl;

    //Define ntuple structure
    //
    analysisManager->CreateNtuple("DREMTubesout", "DREMTubesoutput");
    analysisManager->CreateNtupleDColumn("EnergyScin");                     //0
    analysisManager->CreateNtupleDColumn("EnergyCher");                     //1
    analysisManager->CreateNtupleDColumn("NofPMTCherDet");                  //2
    analysisManager->CreateNtupleDColumn("NofPMTScinDet");                  //3
    analysisManager->CreateNtupleDColumn("EnergyTot");                      //4
    analysisManager->CreateNtupleDColumn("PrimaryParticleEnergy");          //5
    analysisManager->CreateNtupleIColumn("PrimaryPDGID");                   //6
    analysisManager->CreateNtupleDColumn("EscapedEnergyl");                 //7
    analysisManager->CreateNtupleDColumn("EscapedEnergyd");                 //8
    analysisManager->CreateNtupleDColumn("PSEnergy");                       //9
    analysisManager->CreateNtupleDColumn("PrimaryX");                       //10
    analysisManager->CreateNtupleDColumn("PrimaryY");                       //11
    analysisManager->CreateNtupleDColumn("NofSiPMScinDet");                 //12
    analysisManager->CreateNtupleDColumn("NofSiPMCherDet");                 //13
    analysisManager->CreateNtupleDColumn("CaloTimeStart_ns");           //14
    analysisManager->CreateNtupleDColumn("CaloTimeBinWidth_ns");        //15
    analysisManager->CreateNtupleIColumn("CaloTimeNBins");              //16
    analysisManager->CreateNtupleDColumn("CaloDistanceStart_mm");       //17
    analysisManager->CreateNtupleDColumn("CaloDistanceBinWidth_mm");    //18
    analysisManager->CreateNtupleIColumn("CaloDistanceNBins");          //19
    analysisManager->CreateNtupleDColumn("ScintillationDecayTime_ns");       //20
    analysisManager->CreateNtupleDColumn("ScintillationEffectiveVelocity_mm_per_ns"); //21
    analysisManager->CreateNtupleIColumn("EventID");                         //22
    analysisManager->CreateNtupleDColumn("PrimaryCalorimeterEntryTime_ns");  //23 <- left empty for now, to be filled in future with the time of the first hit in the calorimeter
    analysisManager->CreateNtupleDColumn("VectorSignals", fEventAction->GetVectorSignals());
    analysisManager->CreateNtupleDColumn("VectorSignalsCher", fEventAction->GetVectorSignalsCher());
    analysisManager->CreateNtupleDColumn("VecTowerE", fEventAction->GetVecTowerE());
    analysisManager->CreateNtupleDColumn("VecSPMT", fEventAction->GetVecSPMT());
    analysisManager->CreateNtupleDColumn ("VecCPMT", fEventAction->GetVecCPMT());
    analysisManager->CreateNtupleDColumn ("VecLeakCounter", fEventAction->GetVecLeakCounter());
    // Sparse sensitive-detector collections use the same collection.member
    // convention as the Key4hep IDEA fibre calorimeter. The time and distance
    // members remain integer bin indices; their common binning is stored in
    // the scalar metadata columns above.
    analysisManager->CreateNtupleIColumn("DREMTCher.TowerID", fEventAction->GetCherenkovTowerIDs());
    analysisManager->CreateNtupleIColumn("DREMTCher.CellID", fEventAction->GetCherenkovFiberIDs());
    analysisManager->CreateNtupleIColumn("DREMTCher.ProductionTimeBin", fEventAction->GetCherenkovProductionTimeBins());
    analysisManager->CreateNtupleIColumn("DREMTCher.ArrivalTimeBin", fEventAction->GetCherenkovTimeBins());
    analysisManager->CreateNtupleIColumn("DREMTCher.DistanceBin", fEventAction->GetCherenkovDistanceBins());
    analysisManager->CreateNtupleIColumn("DREMTCher.energy", fEventAction->GetCherenkovPhotonCounts());     // Cherenkov photon counts
    analysisManager->CreateNtupleIColumn("DREMTScin.TowerID", fEventAction->GetScintillationTowerIDs());
    analysisManager->CreateNtupleIColumn("DREMTScin.CellID", fEventAction->GetScintillationFiberIDs());
    analysisManager->CreateNtupleIColumn("DREMTScin.ProductionTimeBin", fEventAction->GetScintillationProductionTimeBins());
    analysisManager->CreateNtupleIColumn("DREMTScin.ArrivalTimeBin", fEventAction->GetScintillationTimeBins());
    analysisManager->CreateNtupleIColumn("DREMTScin.DistanceBin", fEventAction->GetScintillationDistanceBins());
    analysisManager->CreateNtupleDColumn("DREMTScin.energy", fEventAction->GetScintillationVisibleEnergies());  // Scintillation visible energy 
    analysisManager->FinishNtuple();
      
}

//Define de-constructor
//
DREMTubesRunAction::~DREMTubesRunAction(){
   
    //Delete only instance of G4AnalysisManager
    //
    //delete G4AnalysisManager::Instance();  

}

//Define BeginOfRunAction() and EndOfRunAction() methods
//
void DREMTubesRunAction::BeginOfRunAction( const G4Run* Run )  { 
    
    //Save random seeds (optional)
    //
    //G4RunManager::GetRunManager()->SetRandomNumberStore( true );
    
    //Open output file, one per Run
    //
    auto analysisManager = G4AnalysisManager::Instance();
    std::string runnumber = std::to_string( Run->GetRunID() );
    G4String outputfile = "DREMTubesout_Run"+runnumber+".root";
    analysisManager->OpenFile( outputfile );

}

void DREMTubesRunAction::EndOfRunAction( const G4Run* ) {
  
    auto analysisManager = G4AnalysisManager::Instance();

    analysisManager->Write();
    analysisManager->CloseFile();

}

//**************************************************
