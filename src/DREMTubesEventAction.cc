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
#include "DREMTubesCalorimeterHit.hh"
//Includers from Geant4
//
//#include "g4root.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4Event.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "Randomize.hh"

//Includers from C++
//
#include <iomanip>
#include <vector>
#include <numeric>


//Define constructor
//
DREMTubesEventAction::DREMTubesEventAction()
    : G4UserEventAction(),
    EnergyScin(0.),
    EnergyCher(0.),
    NofCherDet(0),
    NofScinDet(0),
    EnergyTot(0.),
    PrimaryPDGID(0),
    PrimaryX(0),
    PrimaryY(0),
    EventID(-1),
    PrimaryCalorimeterEntryTime(-1.),
    HasPrimaryCalorimeterEntry(false),
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

//Define BeginOfEventAction() and EndOfEventAction() methods
//
void DREMTubesEventAction::BeginOfEventAction(const G4Event* event) {
    
    //Initialize data memebers at begin of each event
    //
    EnergyScin = 0.;
    EnergyCher = 0.;
    NofCherDet = 0;
    NofScinDet = 0;
    EnergyTot = 0.;
    PrimaryPDGID = 0;
    PrimaryX = 0;
    EventID = event->GetEventID();
    PrimaryCalorimeterEntryTime = -1.;
    HasPrimaryCalorimeterEntry = false;
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
    CherenkovTowerIDs.clear();
    CherenkovFiberIDs.clear();
    CherenkovProductionTimeBins.clear();
    CherenkovTimeBins.clear();
    CherenkovDistanceBins.clear();
    CherenkovPhotonCounts.clear();
    ScintillationTowerIDs.clear();
    ScintillationFiberIDs.clear();
    ScintillationProductionTimeBins.clear();
    ScintillationTimeBins.clear();
    ScintillationDistanceBins.clear();
    ScintillationVisibleEnergies.clear();

    VectorSignals.assign(NoFibersTower*NoModulesSiPM, 0.);
    VectorSignalsCher.assign(NoFibersTower*NoModulesSiPM, 0.);
    VecSPMT.assign(NoModulesActive, 0.);
    VecCPMT.assign(NoModulesActive, 0.);
    VecTowerE.assign(NoModulesActive, 0.);
    VecLeakCounter.assign(4*NofLeakCounterLayers+1, 0.);


}

void DREMTubesEventAction::EndOfEventAction(const G4Event* event) {
 
    G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();

    //Add all p.e. in Scin and Cher fibers before calibration
    //
    NofScinDet = std::accumulate(VecSPMT.begin(),VecSPMT.end(),0);
    NofCherDet = std::accumulate(VecCPMT.begin(),VecCPMT.end(),0);

    G4int NofPheSciSiPM = std::accumulate(VectorSignals.begin(),VectorSignals.end(),0.);
    G4int NofPheCerSiPM = std::accumulate(VectorSignalsCher.begin(),VectorSignalsCher.end(),0.);

    StoreCalorimeterHits(event);

    // NofScinDet and NofCherDet take sum of photoelectrons deposited in PMT towers
    // also those in the SiPM modules, as if they were readout with PMTs
    // to add: sum separately photoelectrons in PMT and SiPM towers

    //Fill ntuple event by event
    //entries with vectors are automatically filled
    //
    analysisManager->FillNtupleDColumn(0, EnergyScin);
    analysisManager->FillNtupleDColumn(1, EnergyCher);
    analysisManager->FillNtupleDColumn(2, NofCherDet);
    analysisManager->FillNtupleDColumn(3, NofScinDet);
    analysisManager->FillNtupleDColumn(4, EnergyTot);
    analysisManager->FillNtupleDColumn(5, PrimaryParticleEnergy);
    analysisManager->FillNtupleIColumn(6, PrimaryPDGID);
    analysisManager->FillNtupleDColumn(7, EscapedEnergyl);
    analysisManager->FillNtupleDColumn(8, EscapedEnergyd);
    analysisManager->FillNtupleDColumn(9, PSEnergy);
    analysisManager->FillNtupleDColumn(10, PrimaryX);
    analysisManager->FillNtupleDColumn(11,PrimaryY);
    analysisManager->FillNtupleDColumn(12,NofPheSciSiPM);
    analysisManager->FillNtupleDColumn(13,NofPheCerSiPM);
    analysisManager->FillNtupleDColumn(14,CalorimeterTimeStart/ns);
    analysisManager->FillNtupleDColumn(15,CalorimeterTimeBinWidth/ns);
    analysisManager->FillNtupleIColumn(16,NofCalorimeterTimeBins);
    analysisManager->FillNtupleDColumn(17,CalorimeterDistanceStart/mm);
    analysisManager->FillNtupleDColumn(18,CalorimeterDistanceBinWidth/mm);
    analysisManager->FillNtupleIColumn(19,NofCalorimeterDistanceBins);
    analysisManager->FillNtupleDColumn(20,ScintillationDecayTime/ns);
    analysisManager->FillNtupleDColumn(
        21,ScintillationEffectiveVelocity/(mm/ns));
    analysisManager->FillNtupleIColumn(22,EventID);
    analysisManager->FillNtupleDColumn(
        23,PrimaryCalorimeterEntryTime/ns);
    analysisManager->AddNtupleRow();
    //Vector entries in ntuple are automatically filled

}

void DREMTubesEventAction::StoreCalorimeterHits(const G4Event* event)
{
    auto* hitCollections = event->GetHCofThisEvent();
    if (!hitCollections) {
        return;
    }

    if (CalorimeterHitsCollectionID < 0) {
        CalorimeterHitsCollectionID = G4SDManager::GetSDMpointer()->GetCollectionID(
            "CalorimeterHits");
    }

    if (CalorimeterHitsCollectionID < 0) {
        return;
    }

    const auto* hits = static_cast<const DREMTubesCalorimeterHitsCollection*>(
        hitCollections->GetHC(CalorimeterHitsCollectionID));
    if (!hits) {
        return;
    }

    for (const auto* hit : *hits->GetVector()) {
        if (!hit->IsCherenkov()) {
            for (const auto& [bins, visibleEnergy] :
                 hit->GetScintillationBinStructure()) {
                const auto& [productionTimeBin, timeBin, distanceBin] = bins;
                ScintillationTowerIDs.push_back(hit->GetTowerID());
                ScintillationFiberIDs.push_back(hit->GetFiberID());
                ScintillationProductionTimeBins.push_back(productionTimeBin);
                ScintillationTimeBins.push_back(timeBin);
                ScintillationDistanceBins.push_back(distanceBin);
                ScintillationVisibleEnergies.push_back(visibleEnergy/MeV);
            }
            continue;
        }

        // These output vectors are parallel. Each index describes one occupied
        // (tower, fibre, production-time bin, arrival-time bin, distance bin)
        // cell and the number of photons sharing those coordinates.
        for (const auto& [bins, count] : hit->GetPhotonBinStructure()) {
            const auto& [productionTimeBin, timeBin, distanceBin] = bins;
            CherenkovTowerIDs.push_back(hit->GetTowerID());
            CherenkovFiberIDs.push_back(hit->GetFiberID());
            CherenkovProductionTimeBins.push_back(productionTimeBin);
            CherenkovTimeBins.push_back(timeBin);
            CherenkovDistanceBins.push_back(distanceBin);
            CherenkovPhotonCounts.push_back(count);
        }
    }
}

//**************************************************
