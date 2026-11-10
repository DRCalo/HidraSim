//**************************************************
// \file DREMTubesCalorimeterHit.hh
// \brief Definition of a raw dual-readout calorimeter hit
//**************************************************

#ifndef DREMTubesCalorimeterHit_h
#define DREMTubesCalorimeterHit_h 1

#include "G4Allocator.hh"
#include "G4THitsCollection.hh"
#include "G4VHit.hh"
#include "globals.hh"

#include <cstddef>
#include <map>
#include <tuple>

class DREMTubesCalorimeterHit : public G4VHit {
  public:
    // The key preserves the complete production-time, arrival-time and
    // production-distance correlation. Only occupied cells are stored.
    using PhotonBinKey = std::tuple<G4int, G4int, G4int>;
    using PhotonBinStructure = std::map<PhotonBinKey, G4int>;
    using ScintillationBinStructure = std::map<PhotonBinKey, G4double>;

    DREMTubesCalorimeterHit() = default;
    DREMTubesCalorimeterHit(const DREMTubesCalorimeterHit&) = default;
    ~DREMTubesCalorimeterHit() override = default;

    DREMTubesCalorimeterHit& operator=(const DREMTubesCalorimeterHit&) = default;
    G4bool operator==(const DREMTubesCalorimeterHit& right) const { return this == &right; }

    void* operator new(std::size_t);
    void operator delete(void* hit);

    void Print() override;

    void AddEnergyDeposit(G4double energyDeposit) { fEnergyDeposit += energyDeposit; }
    void CountCherenkovPhoton(
        G4double productionTime,
        G4double driftTime,
        G4double distanceToReadout);
    void AddScintillationEnergy(
        G4double visibleEnergy,
        G4double productionTime,
        G4double driftTime,
        G4double distanceToReadout);

    void SetTowerID(G4int towerID) { fTowerID = towerID; }
    void SetFiberID(G4int fiberID) { fFiberID = fiberID; }
    void SetCherenkov(G4bool isCherenkov) { fIsCherenkov = isCherenkov; }

    G4int GetTowerID() const { return fTowerID; }
    G4int GetFiberID() const { return fFiberID; }
    G4bool IsCherenkov() const { return fIsCherenkov; }
    G4double GetEnergyDeposit() const { return fEnergyDeposit; }
    unsigned long GetPhotonCount() const { return fPhotonCount; }
    const PhotonBinStructure& GetPhotonBinStructure() const {
        return fPhotonBinStructure;
    }
    const ScintillationBinStructure& GetScintillationBinStructure() const {
        return fScintillationBinStructure;
    }

  private:
    G4int fTowerID{-1};
    G4int fFiberID{-1};
    G4bool fIsCherenkov{false};
    G4double fEnergyDeposit{0.};
    unsigned long fPhotonCount{0};
    PhotonBinStructure fPhotonBinStructure;
    ScintillationBinStructure fScintillationBinStructure;
};

using DREMTubesCalorimeterHitsCollection = G4THitsCollection<DREMTubesCalorimeterHit>;

extern G4ThreadLocal G4Allocator<DREMTubesCalorimeterHit>* DREMTubesCalorimeterHitAllocator;

inline void* DREMTubesCalorimeterHit::operator new(std::size_t)
{
    if (!DREMTubesCalorimeterHitAllocator) {
        DREMTubesCalorimeterHitAllocator = new G4Allocator<DREMTubesCalorimeterHit>;
    }
    return DREMTubesCalorimeterHitAllocator->MallocSingle();
}

inline void DREMTubesCalorimeterHit::operator delete(void* hit)
{
    DREMTubesCalorimeterHitAllocator->FreeSingle(
        static_cast<DREMTubesCalorimeterHit*>(hit));
}

#endif

//**************************************************
