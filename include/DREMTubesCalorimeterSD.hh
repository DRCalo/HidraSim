//**************************************************
// \file DREMTubesCalorimeterSD.hh
// \brief Definition of the dual-readout calorimeter sensitive detector
//**************************************************

#ifndef DREMTubesCalorimeterSD_h
#define DREMTubesCalorimeterSD_h 1

#include "DREMTubesCalorimeterHit.hh"

#include "G4VSensitiveDetector.hh"
#include "globals.hh"

#include <cstdint>
#include <unordered_map>

class G4HCofThisEvent;
class G4OpBoundaryProcess;
class G4Step;
class G4TouchableHistory;

class DREMTubesCalorimeterSD : public G4VSensitiveDetector {
  public:
    DREMTubesCalorimeterSD(const G4String& name, const G4String& hitsCollectionName);
    ~DREMTubesCalorimeterSD() override = default;

    void Initialize(G4HCofThisEvent* hitCollection) override;
    G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;

  private:
    static std::uint64_t MakeHitKey(G4int towerID, G4int fiberID, G4bool isCherenkov);
    G4bool GetCherenkovPhotonTiming(
        G4Step* step,
        G4double& productionTime,
        G4double& driftTime,
        G4double& distanceToReadout);

    DREMTubesCalorimeterHitsCollection* fHitsCollection{nullptr};
    G4int fHitsCollectionID{-1};
    std::unordered_map<std::uint64_t, DREMTubesCalorimeterHit*> fHitLookup;
    G4OpBoundaryProcess* fBoundaryProcess{nullptr};
};

#endif

//**************************************************
