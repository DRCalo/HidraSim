//**************************************************
// \file DREMTubesCalorimeterSD.cc
// \first implementation of HiDRa DRCal calorimeter sensitive detector
//**************************************************

#include "DREMTubesCalorimeterSD.hh"
#include "DREMTubesGeoPar.hh"
#include "DREMTubesSignalHelper.hh"

#include "G4HCofThisEvent.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4OpticalPhoton.hh"
#include "G4ProcessManager.hh"
#include "G4SDManager.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4SystemOfUnits.hh"
#include "G4TouchableHandle.hh"
#include "G4Tubs.hh"
#include "G4VPhysicalVolume.hh"
#include "Randomize.hh"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {

// TEMPORARY LEGACY CROSS-CHECK
// Set this single switch to false to restore the intended forward-only
// Cherenkov acceptance. While true, backward-going trapped photons contribute
// to the photon count and use |cos(theta_z)| for their propagation time.
constexpr G4bool kIncludeBackwardCherenkovForLegacyComparison = true;
//constexpr G4bool kIncludeBackwardCherenkovForLegacyComparison = false;

} // namespace

DREMTubesCalorimeterSD::DREMTubesCalorimeterSD(
    const G4String& name,
    const G4String& hitsCollectionName)
    : G4VSensitiveDetector(name)
{
    collectionName.insert(hitsCollectionName);
}

void DREMTubesCalorimeterSD::Initialize(G4HCofThisEvent* hitCollection)
{
    fHitsCollection = new DREMTubesCalorimeterHitsCollection(
        SensitiveDetectorName, collectionName[0]);
    fHitLookup.clear();

    if (fHitsCollectionID < 0) {
        const G4String fullCollectionName =
            SensitiveDetectorName + "/" + collectionName[0];
        fHitsCollectionID =
            G4SDManager::GetSDMpointer()->GetCollectionID(fullCollectionName);
    }

    hitCollection->AddHitsCollection(fHitsCollectionID, fHitsCollection);
}

G4bool DREMTubesCalorimeterSD::ProcessHits(
    G4Step* step,
    G4TouchableHistory*)
{
    const auto touchable = step->GetPreStepPoint()->GetTouchableHandle();
    const G4String& volumeName = touchable->GetVolume()->GetName();
    //const G4bool isCherenkov = volumeName == "Core_C_fiber";
    //if (!isCherenkov && volumeName != "Core_S_fiber") {
    //    return false;
    //}
    const G4bool isScintillation =
    volumeName == "Core_S_fiber" ||
    volumeName == "Clad_S_fiber";

    const G4bool isCherenkov =
        volumeName == "Core_C_fiber" ||
        volumeName == "Clad_C_fiber";

    if (!isScintillation && !isCherenkov) {
        return false;
    }

    const G4double energyDeposit = step->GetTotalEnergyDeposit();
    G4double productionTime = 0.;
    G4double driftTime = 0.;
    G4double distanceToReadout = 0.;
    const G4bool trappedPhoton = isCherenkov &&
        GetCherenkovPhotonTiming(
            step, productionTime, driftTime, distanceToReadout);
    if (energyDeposit <= 0. && !trappedPhoton) {
        return false;
    }

    const G4int fiberID = touchable->GetCopyNumber(1);
    const G4int towerID = touchable->GetCopyNumber(3);
    const std::uint64_t hitKey = MakeHitKey(towerID, fiberID, isCherenkov);

    auto found = fHitLookup.find(hitKey);
    DREMTubesCalorimeterHit* hit = nullptr;

    if (found == fHitLookup.end()) {
        hit = new DREMTubesCalorimeterHit;
        hit->SetTowerID(towerID);
        hit->SetFiberID(fiberID);
        hit->SetCherenkov(isCherenkov);
        fHitsCollection->insert(hit);
        fHitLookup.emplace(hitKey, hit);
    } else {
        hit = found->second;
    }

    if (energyDeposit > 0.) {
        const auto* pre = step->GetPreStepPoint();
        const auto* post = step->GetPostStepPoint();

        hit->AddEnergyDeposit(energyDeposit);

        if (!isCherenkov &&
            step->GetTrack()->GetDefinition() != G4OpticalPhoton::Definition() &&
            step->GetTrack()->GetDefinition()->GetPDGCharge() != 0.) {
            const auto* fiber =
                dynamic_cast<const G4Tubs*>(touchable->GetSolid());
            if (fiber) {
                // Use the step midpoint as the production point of the
                // parameterised scintillation light.
                const G4ThreeVector globalProductionPosition =
                    0.5 * (pre->GetPosition() + post->GetPosition());
                const G4ThreeVector localProductionPosition =
                    touchable->GetHistory()->GetTopTransform().TransformPoint(
                        globalProductionPosition);
                const G4double scintillationDistance = std::max(
                    0., fiber->GetZHalfLength() - localProductionPosition.z());
                const G4double scintillationDepositTime =
                    0.5 * (pre->GetGlobalTime() + post->GetGlobalTime());
                const G4double emissionDelay =
                    -ScintillationDecayTime *
                    std::log(1. - G4UniformRand());
                const G4double scintillationProductionTime =
                    scintillationDepositTime + emissionDelay;
                const G4double scintillationDriftTime =
                    scintillationDistance / ScintillationEffectiveVelocity;

                // Store quenched visible energy rather than creating the very
                // large number of individual scintillation optical photons.
                // Smearing and light attenuation will be postponed offline to the digitization stage
                G4double visibleEnergy = energyDeposit;
                if (step->GetStepLength() > 0.) {
                    visibleEnergy = DREMTubesSignalHelper::Instance()->ApplyBirks(
                        energyDeposit, step->GetStepLength());
                }

                hit->AddScintillationEnergy(
                    visibleEnergy,
                    scintillationProductionTime,
                    scintillationDriftTime,
                    scintillationDistance);
            }
        }
    }

    if (trappedPhoton) {
        hit->CountCherenkovPhoton(
            productionTime, driftTime, distanceToReadout);
        step->GetTrack()->SetTrackStatus(fStopAndKill);
    }

    return true;
}

G4bool DREMTubesCalorimeterSD::GetCherenkovPhotonTiming(
    G4Step* step,
    G4double& productionTime,
    G4double& driftTime,
    G4double& distanceToReadout)
{
    auto* track = step->GetTrack();
    if (track->GetDefinition() != G4OpticalPhoton::Definition()) {
        return false;
    }

    if (!fBoundaryProcess) {
        auto* processManager = G4OpticalPhoton::Definition()->GetProcessManager();
        if (!processManager) {
            return false;
        }

        auto* processes = processManager->GetPostStepProcessVector(typeDoIt);
        for (G4int index = 0; index < processes->entries(); ++index) {
            fBoundaryProcess =
                dynamic_cast<G4OpBoundaryProcess*>((*processes)[index]);
            if (fBoundaryProcess) {
                break;
            }
        }
    }

    if (!fBoundaryProcess) {
        return false;
    }

    if (fBoundaryProcess->GetStatus() != TotalInternalReflection) {
        if (step->GetPostStepPoint()->GetStepStatus() == fGeomBoundary) {
            track->SetTrackStatus(fStopAndKill);
        }
        return false;
    }

    const auto touchable = step->GetPreStepPoint()->GetTouchableHandle();
    const auto* fiber = dynamic_cast<const G4Tubs*>(touchable->GetSolid());
    if (!fiber) {
        return false;
    }

    const auto& globalToLocal =
        touchable->GetHistory()->GetTopTransform();
    const G4ThreeVector localVertex =
        globalToLocal.TransformPoint(track->GetVertexPosition());
    const G4ThreeVector localDirection =
        globalToLocal.TransformAxis(track->GetVertexMomentumDirection());
    const G4double directionCosine = localDirection.z();

    // The readout is located at the positive local-z end of each fibre.
    // The temporary legacy-comparison mode accepts photons travelling toward
    // negative local z as well. Its absolute cosine gives a positive axial
    // speed while preserving the angular path-length correction in the timing.
    if (!kIncludeBackwardCherenkovForLegacyComparison &&
        directionCosine <= 0.) {
        track->SetTrackStatus(fStopAndKill);
        return false;
    }

    distanceToReadout =
        std::max(0., fiber->GetZHalfLength() - localVertex.z());
    const G4double timingCosine =
        kIncludeBackwardCherenkovForLegacyComparison
            ? std::abs(directionCosine)
            : directionCosine;
    const G4double axialVelocity = track->GetVelocity() * timingCosine;
    if (axialVelocity <= 0.) {
        track->SetTrackStatus(fStopAndKill);
        return false;
    }

    // Local time is the elapsed time since this optical track was created.
    productionTime = track->GetGlobalTime() - track->GetLocalTime();
    driftTime = distanceToReadout / axialVelocity;
    return true;
}

std::uint64_t DREMTubesCalorimeterSD::MakeHitKey(
    G4int towerID,
    G4int fiberID,
    G4bool isCherenkov)
{
    const auto tower = static_cast<std::uint64_t>(static_cast<std::uint32_t>(towerID));
    const auto fiber = static_cast<std::uint64_t>(static_cast<std::uint32_t>(fiberID));
    const auto type = static_cast<std::uint64_t>(isCherenkov ? 1 : 0);
    return (type << 63) | (tower << 32) | fiber;
}

//**************************************************
