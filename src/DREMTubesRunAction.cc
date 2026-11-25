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

    //Using ROOT as analysisManager type, print it
    //
    G4cout << "DREMTubes-> Using " << analysisManager->GetType() << G4endl;

    //Define ntuple structure
    //
    analysisManager->CreateNtuple("DREMTubesout", "DREMTubesoutput");

    //Helpers to create the columns of the ntuple:
    //  addD / addI: scalar column (double / int), filled by name in EventAction
    //  addV: vector column, filled automatically from the EventAction vector
    //To add a scalar column: one line here + the matching Fill line in EventAction.
    //To remove it: delete both lines. The other columns are not affected.
    //The order of the lines here is the order of the branches in the output.
    //(if fEventAction is null a scalar column is only created)
    auto addD = [this, analysisManager](const G4String& name) {
        const G4int id = analysisManager->CreateNtupleDColumn(name);
        if (fEventAction) fEventAction->SetColumnId(name, id);
    };
    auto addI = [this, analysisManager](const G4String& name) {
        const G4int id = analysisManager->CreateNtupleIColumn(name);
        if (fEventAction) fEventAction->SetColumnId(name, id);
    };
    auto addV = [analysisManager](const G4String& name, std::vector<G4double>& vec) {
        analysisManager->CreateNtupleDColumn(name, vec);
    };

    //Primary particle
    addD("PrimaryParticleEnergy");
    addI("PrimaryPDGID");
    addD("PrimaryX");
    addD("PrimaryY");

    //Energy deposited (truth level)
    addD("EnergyDepCalo");
    addD("EnergyScin");
    addD("EnergyCher");
    addD("PSEnergy");

    //Leakage energy (l = lateral, d = longitudinal)
    addD("EscapedEnergyl");
    addD("EscapedEnergyd");

    //Photoelectrons detected (PMT and SiPM)
    addD("NofPMTScinDet");
    addD("NofPMTCherDet");
    addD("NofSiPMScinDet");
    addD("NofSiPMCherDet");

    //Vector columns (filled automatically)
    addV("VectorSignals", fEventAction->GetVectorSignals());
    addV("VectorSignalsCher", fEventAction->GetVectorSignalsCher());
    addV("VecTowerE", fEventAction->GetVecTowerE());
    addV("VecSPMT", fEventAction->GetVecSPMT());
    addV("VecCPMT", fEventAction->GetVecCPMT());
    addV("VecLeakCounter", fEventAction->GetVecLeakCounter());
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