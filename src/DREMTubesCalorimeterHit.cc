//**************************************************
// \file DREMTubesCalorimeterHit.cc
// \brief Implementation of a raw dual-readout calorimeter hit
//**************************************************

#include "DREMTubesCalorimeterHit.hh"
#include "DREMTubesGeoPar.hh"

#include "G4UnitsTable.hh"
#include "G4ios.hh"

namespace {

G4int FindTimeBin(G4double time)
{
    if (time < CalorimeterTimeStart) {
        return 0;
    }
    if (time >= CalorimeterTimeEnd) {
        return NofCalorimeterTimeBins + 1;
    }
    return 1 + static_cast<G4int>(
        (time - CalorimeterTimeStart) / CalorimeterTimeBinWidth);
}

G4int FindDistanceBin(G4double distance)
{
    if (distance < CalorimeterDistanceStart) {
        return 0;
    }
    if (distance >= CalorimeterDistanceEnd) {
        return NofCalorimeterDistanceBins + 1;
    }
    return 1 + static_cast<G4int>(
        (distance - CalorimeterDistanceStart) /
        CalorimeterDistanceBinWidth);
}

} // namespace

G4ThreadLocal G4Allocator<DREMTubesCalorimeterHit>* DREMTubesCalorimeterHitAllocator = nullptr;

void DREMTubesCalorimeterHit::CountCherenkovPhoton(
    G4double productionTime,
    G4double driftTime,
    G4double distanceToReadout)
{
    ++fPhotonCount;

    const G4int productionTimeBin = FindTimeBin(productionTime);

    // Arrival time combines the event-level production time with the
    // parameterised propagation time to the positive-z fibre end.
    const G4double hitTime = productionTime + driftTime;

    const G4int timeBin = FindTimeBin(hitTime);
    const G4int distanceBin = FindDistanceBin(distanceToReadout);

    // Photons sharing all three coordinates are aggregated into one cell.
    ++fPhotonBinStructure[{productionTimeBin, timeBin, distanceBin}];
}

void DREMTubesCalorimeterHit::AddScintillationEnergy(
    G4double visibleEnergy,
    G4double productionTime,
    G4double driftTime,
    G4double distanceToReadout)
{
    if (visibleEnergy <= 0.) {
        return;
    }

    const G4int productionTimeBin = FindTimeBin(productionTime);
    const G4int distanceBin = FindDistanceBin(distanceToReadout);

    // Production time contains the sampled scintillation emission delay.
    const G4double arrivalTime = productionTime + driftTime;
    const G4int arrivalTimeBin = FindTimeBin(arrivalTime);

    fScintillationBinStructure[
        {productionTimeBin, arrivalTimeBin, distanceBin}] += visibleEnergy;
}

void DREMTubesCalorimeterHit::Print()
{
    G4cout << "DREMTubes calorimeter hit: tower " << fTowerID
           << ", fiber " << fFiberID
           << ", type " << (fIsCherenkov ? "Cherenkov" : "scintillation")
           << ", deposited energy " << G4BestUnit(fEnergyDeposit, "Energy")
           << ", accepted photons " << fPhotonCount
           << G4endl;
}

//**************************************************
