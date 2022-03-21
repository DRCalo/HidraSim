#include "HidraSiPMSD.hh"
#include "G4HCofThisEvent.hh"
#include "G4OpticalPhoton.hh"
#include "G4SDManager.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

HidraSiPMSD::HidraSiPMSD(const std::string& name, const std::string& hCName, const int nSensor, sipm::SiPMSensor* sipm)
    : G4VSensitiveDetector(name), m_HitsCollection(nullptr), m_nSiPM(nSensor), m_SiPMSensor(sipm) {
  collectionName.insert(hCName);
}

HidraSiPMSD::~HidraSiPMSD() {
  delete m_SiPMSensor;
}

void HidraSiPMSD::Initialize(G4HCofThisEvent* hce) {
  // Create hits collection
  m_HitsCollection = new HidraSiPMHitsCollection(SensitiveDetectorName, collectionName[0]);

  // Add this collection in hce
  auto hcID = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
  hce->AddHitsCollection(hcID, m_HitsCollection);

  // Create hits
  // Probably better to add only sipm when hitted and search for them
  // For now add all sipm
  for (int i = 0; i < m_nSiPM; i++) {
    auto hit = new HidraSiPMHit();
    m_HitsCollection->insert(hit);
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

bool HidraSiPMSD::ProcessHits(G4Step* step, G4TouchableHistory*) {
  if (step->GetTrack()->GetParticleDefinition() != G4OpticalPhoton::Definition()) {
    return false;
  }
  // GetCopyNumber(1) == sipmColCopyNumber  (aka col in row)
  // GetCopyNumber(2) == sipmRowCopyNumber  (aka row in module)
  // GetCopyNumber(3) == moduleCopyNumber   (aka module id in calo)
  const auto touchable = step->GetPreStepPoint()->GetTouchable();
  const int sipmCol = touchable->GetCopyNumber(1);
  const int sipmRow = touchable->GetCopyNumber(2);
  const int module = touchable->GetCopyNumber(3);
  // Need det contr to collect sipmId
  // Still to be defined, if need to search for hitted sipm only this has to change 
  const int sipmId = 0; //module * nFibersRows * nFibersCols + sipmRow * nFibersCols + sipmCol; 
  HidraSiPMHit* hit = static_cast<HidraSiPMHit*>(m_HitsCollection->GetHit(sipmId));

  hit->m_Times.push_back(step->GetTrack()->GetGlobalTime());
  hit->m_Wavelengths.push_back(1239.84193 / step->GetTrack()->GetKineticEnergy() * eV);

  step->GetTrack()->SetTrackStatus(fStopAndKill);
  return true;
}

void HidraSiPMSD::DoSiPMSim(HidraSiPMHit* hit) {
  // Run sipm simulation
  m_SiPMSensor->resetState();
  m_SiPMSensor->addPhotons(hit->m_Times, hit->m_Wavelengths);
  m_SiPMSensor->runEvent();
  // Get signal info
  const double integral = m_SiPMSensor->signal().integral(0, 250, 1.5);
  const double toa = m_SiPMSensor->signal().toa(0, 250, 1.5);
  const int nPhe = m_SiPMSensor->debug().nPhotoelectrons;
  // Save in hit class
  hit->m_Integral = integral;
  hit->m_Toa = toa;
  hit->m_NPhotoelectrons = nPhe;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void HidraSiPMSD::EndOfEvent(G4HCofThisEvent* hce) {
  auto hcID = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
  auto hc = hce->GetHC(hcID);
  for (int i = 0; i < m_nSiPM; ++i) {
    HidraSiPMHit* hit = static_cast<HidraSiPMHit*>(hc->GetHit(i));
    // Digitise only if at least 2 photon hit sipm
    if(hit->m_Times.size() > 2)
      DoSiPMSim(hit);
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
