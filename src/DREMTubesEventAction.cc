//**************************************************
// \file DREMTubesEventAction.cc
// \brief: Implementation of DREMTubesEventAction
//         class
// \author: Lorenzo Pezzotti (CERN EP-SFT-sim)
//          @lopezzot
// \start date: 7 July 2021
//**************************************************

//Includers from project files
//
#include "DREMTubesEventAction.hh"
#include "DREMTubesRunAction.hh"
#include "DREMTubesDetectorConstruction.hh"
//Includers from Geant4
//
//#include "g4root.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4Event.hh"
#include "G4UnitsTable.hh"
#include "Randomize.hh"
#include "G4Exception.hh"

//Includers from C++
//
#include <iomanip>
#include <vector>
#include <numeric>
#include <string>


//Define constructor
//
DREMTubesEventAction::DREMTubesEventAction()
    : G4UserEventAction(),
    EnergyScin(0.),
    EnergyCher(0.),
    NofCherDet(0),
    NofScinDet(0),
    EnergyDepCalo(0.),
    PrimaryPDGID(0),
    PrimaryX(0),
    PrimaryY(0),
    PrimaryParticleEnergy(0.),
    //EscapedEnergy(0.),
    EscapedEnergyl(0.),
    EscapedEnergyd(0.),
    PSEnergy(0.),
    VectorSignals(0.),
    VectorSignalsCher(0.),
    VecSPMT(0.),
    VecCPMT(0.),
    VecTowerE(0.) {
}

//Define de-constructor
//
DREMTubesEventAction::~DREMTubesEventAction() {}

//Define SetColumnId() and ColumnId() methods
//
void DREMTubesEventAction::SetColumnId(const std::string& name, G4int id) {
    fColumnId[name] = id;
}

G4int DREMTubesEventAction::ColumnId(const std::string& name) const {
    auto it = fColumnId.find(name);
    if (it != fColumnId.end()) return it->second;
    std::string msg = "Ntuple column '" + name + "' is filled in EventAction "
                      "but was never created in RunAction (check the column name).";
    G4Exception("DREMTubesEventAction::ColumnId", "NtupleColumnNotFound",
                FatalException, msg.c_str());
    return -1; //never reached: FatalException stops the program
}

//Define BeginOfEventAction() and EndOfEventAction() methods
//
void DREMTubesEventAction::BeginOfEventAction(const G4Event*) {

    //Initialize data memebers at begin of each event
    //
    EnergyScin = 0.;
    EnergyCher = 0.;
    NofCherDet = 0;
    NofScinDet = 0;
    EnergyDepCalo = 0.;
    PrimaryPDGID = 0;
    PrimaryX = 0;
    PrimaryY = 0;
    PrimaryParticleEnergy = 0.;
    //EscapedEnergy = 0.;
    EscapedEnergyl = 0.;
    EscapedEnergyd = 0.;
    PSEnergy = 0.;

    VectorSignals.clear();
    VectorSignalsCher.clear();
    VecSPMT.clear();
    VecCPMT.clear();
    VecTowerE.clear();
    VecLeakCounter.clear();

    VectorSignals.assign(NoFibersTower*NoModulesSiPM, 0.);
    VectorSignalsCher.assign(NoFibersTower*NoModulesSiPM, 0.);
    VecSPMT.assign(NoModulesActive, 0.);
    VecCPMT.assign(NoModulesActive, 0.);
    VecTowerE.assign(NoModulesActive, 0.);
    VecLeakCounter.assign(4*NofLeakCounterLayers+1, 0.);


}

void DREMTubesEventAction::EndOfEventAction(const G4Event* ) {

    G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();

    //Add all p.e. in Scin and Cher fibers before calibration
    //
    NofScinDet = std::accumulate(VecSPMT.begin(),VecSPMT.end(),0);
    NofCherDet = std::accumulate(VecCPMT.begin(),VecCPMT.end(),0);

    G4int NofPheSciSiPM = std::accumulate(VectorSignals.begin(),VectorSignals.end(),0.);
    G4int NofPheCerSiPM = std::accumulate(VectorSignalsCher.begin(),VectorSignalsCher.end(),0.);

    // NofScinDet and NofCherDet take sum of photoelectrons deposited in PMT towers
    // also those in the SiPM modules, as if they were readout with PMTs
    // to add: sum separately photoelectrons in PMT and SiPM towers

    //Fill ntuple event by event
    //entries with vectors are automatically filled
    //scalar columns are filled by name (same names as in RunAction),
    //so the order of the lines below does not matter
    //
    //Primary particle
    analysisManager->FillNtupleDColumn(ColumnId("PrimaryParticleEnergy"), PrimaryParticleEnergy);
    analysisManager->FillNtupleIColumn(ColumnId("PrimaryPDGID"), PrimaryPDGID);
    analysisManager->FillNtupleDColumn(ColumnId("PrimaryX"), PrimaryX);
    analysisManager->FillNtupleDColumn(ColumnId("PrimaryY"), PrimaryY);

    //Energy deposited (truth level)
    analysisManager->FillNtupleDColumn(ColumnId("EnergyDepCalo"), EnergyDepCalo);
    analysisManager->FillNtupleDColumn(ColumnId("EnergyScin"), EnergyScin);
    analysisManager->FillNtupleDColumn(ColumnId("EnergyCher"), EnergyCher);
    analysisManager->FillNtupleDColumn(ColumnId("PSEnergy"), PSEnergy);

    //Leakage energy (l = lateral, d = longitudinal)
    analysisManager->FillNtupleDColumn(ColumnId("EscapedEnergyl"), EscapedEnergyl);
    analysisManager->FillNtupleDColumn(ColumnId("EscapedEnergyd"), EscapedEnergyd);

    //Photoelectrons detected (PMT and SiPM)
    analysisManager->FillNtupleDColumn(ColumnId("NofPMTScinDet"), NofScinDet);
    analysisManager->FillNtupleDColumn(ColumnId("NofPMTCherDet"), NofCherDet);
    analysisManager->FillNtupleDColumn(ColumnId("NofSiPMScinDet"), NofPheSciSiPM);
    analysisManager->FillNtupleDColumn(ColumnId("NofSiPMCherDet"), NofPheCerSiPM);
    analysisManager->AddNtupleRow();
    //Vector entries in ntuple are automatically filled

}

//**************************************************