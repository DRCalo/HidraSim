#include "HidraHistoManager.hh"

#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"

HidraHistoManager::HidraHistoManager() : m_FactoryOn(false), m_AnalysisManager(G4AnalysisManager::Instance()) {}

void HistoManager::Book() {
  // Create histograms
  m_AnalysisManager->CreateH1("TotalEnergy","Total energy deposited in calorimeter",1000,0,120*GeV);
  m_AnalysisManager->CreateH1("ElectromagneticEnergy","EM energy deposited in calorimeter",1000,0,120*GeV);
  m_AnalysisManager->CreateH1("TotalScintillatingEnergy","Energy deposited in scintillating fibers",1000,0,120*GeV);
  m_AnalysisManager->CreateH1("TotalCherenkovEnergy","Energy deposited in cherenkov fibers",1000,0,120*GeV);
  m_AnalysisManager->CreateH1("LeakedEnergy","Energy leaked from calorimeter",1000,0,120*GeV);
  m_AnalysisManager->CreateH1("PrimaryEnergy","Energy of primary particle",100,0,120*GeV);
  m_AnalysisManager->CreateH1("PreShowerEnergy","Energy deposited in preshower",1000,0,1*GeV);
  m_AnalysisManager->CreateH1("TotalScintillatingPhotoelectrons","Total number of scintillating photons detected",1000,0,10000);
  m_AnalysisManager->CreateH1("TotalCherenkovPhotoelectrons","Total number of cherenkov photons detected",1000,0,10000);
  m_AnalysisManager->CreateH1("TotalScintillatingPhotons","Total number of photons produced",1000,0,10000);
  m_AnalysisManager->CreateH1("TotalScintillatingPhotons","Total number of photons produced",1000,0,10000);
  // Create ntuples.
  // Ntuples ids are generated automatically starting from 0.

  // Create 1st ntuple (id = 0)
  m_AnalysisManager->CreateNtuple("Energy", "Energy deposited in calorimeter");
  m_AnalysisManager->CreateNtupleDColumn("TotalEnergy");           // column Id = 0
  m_AnalysisManager->CreateNtupleDColumn("ElectromagneticEnergy"); // column Id = 1
  m_AnalysisManager->CreateNtupleDColumn("ScintillatingEnergy");   // column Id = 2
  m_AnalysisManager->CreateNtupleDColumn("CherenkovEnergy");       // column Id = 3
  m_AnalysisManager->CreateNtupleDColumn("LeakedEnergy");          // column Id = 4
  m_AnalysisManager->CreateNtupleDColumn("PrimaryEnergy");         // column Id = 5
  m_AnalysisManager->CreateNtupleDColumn("PreShowerEnergy");       // column Id = 6
  m_AnalysisManager->FinishNtuple();

  // Create 2nd ntuple (id = 1)
  m_AnalysisManager->CreateNtuple("Photons", "Light statistic");
  m_AnalysisManager->CreateNtupleIColumn("TotalScintillatingPhotons");        // column Id = 0
  m_AnalysisManager->CreateNtupleIColumn("TotalCherenkovPhotons");            // column Id = 1
  m_AnalysisManager->CreateNtupleIColumn("TotalScintillatingPhotoelectrons"); // column Id = 2
  m_AnalysisManager->CreateNtupleIColumn("TotalCherenkovPhotoelectrons");     // column Id = 3
  m_AnalysisManager->FinishNtuple();

  // Create 3nd ntuple (id = 2)
  // Store std::vector<double> / std::vector<int>
  m_AnalysisManager->CreateNtuple("Sensor", "SiPM informations");
  m_AnalysisManager->CreateNtupleDColumn("Integral", m_SipmIntegral); // Column Id = 0
  m_AnalysisManager->CreateNtupleDColumn("ArrivingTime", m_SipmToa);  // Column Id = 1
  m_AnalysisManager->CreateNtupleIColumn("Photons",m_SipmPhotons);    // Column id = 2
  m_AnalysisManager->CreateNtupleIColumn("Photoelectrons",m_SipmPhotoelectron); // Column id = 3
  m_AnalysisManager->FinishNtuple();

  m_FactoryOn = true;
}

void HidraHistoManager::Save() {
  if (!m_FactoryOn) {
    return;
  }

  m_AnalysisManager->Write();
  m_FactoryOn = false;
}

void HidraHistoManager::FillHisto(const int ih, const double xbin, const double weight) {
  m_AnalysisManager->FillH1(ih, xbin, weight);
}

// TODO: probably better create struct and pass it instead of many args
void HidraHistoManager::FillNtuple(const double totE, const double emE, const double sE, const double cE, const double leakE,
                              const double primE, const double preE, const int sPhe, const int cPhe, const int sPh, const int cPh,
                              const std::vector<double>& sipmInt, const std::vector<double>& sipmToa,
                              const std::vector<int>& sipmPhe, const std::vector<int>& sipmPh) {
  // Fill 1st ntuple ( id = 0)
  m_AnalysisManager->FillNtupleDColumn(0, 0, totE);
  m_AnalysisManager->FillNtupleDColumn(0, 1, emE);
  m_AnalysisManager->FillNtupleDColumn(0, 2, sE);
  m_AnalysisManager->FillNtupleDColumn(0, 3, cE);
  m_AnalysisManager->FillNtupleDColumn(0, 4, leakE);
  m_AnalysisManager->FillNtupleDColumn(0, 5, primE);
  m_AnalysisManager->FillNtupleDColumn(0, 6, preE);
  m_AnalysisManager->AddNtupleRow(0);

  // Fill 2nd ntuple ( id = 1)
  m_AnalysisManager->FillNtupleIColumn(1, 0, sPh);
  m_AnalysisManager->FillNtupleIColumn(1, 1, cPh);
  m_AnalysisManager->FillNtupleIColumn(1, 2, sPhe);
  m_AnalysisManager->FillNtupleIColumn(1, 3, cPhe);
  m_AnalysisManager->AddNtupleRow(1);

  // Fill 3nd ntuple ( id = 2)
  // Those are std::vector so no need to fill
  // Just update vector values in class members
  setSiPMIntegral(sipmInt);
  setSiPMToa(sipmToa);
  setSiPMPhotoelectrons(sipmPhe);
  setSiPMPhotons(sipmPh);
  m_AnalysisManager->AddNtupleRow(2);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void HidraHistoManager::PrintStatistic() {
  if (!fFactoryOn) {
    return;
  }

  G4cout << "Total energy deposited"
         << ": mean = " << G4BestUnit(m_AnalysisManager->GetH1(0)->mean(), "Energy")
         << " rms = " << G4BestUnit(m_AnalysisManager->GetH1(0)->rms(), "Energy") << G4endl;
  G4cout << "Total Em energy deposited"
         << ": mean = " << G4BestUnit(m_AnalysisManager->GetH1(1)->mean(), "Energy")
         << " rms = " << G4BestUnit(m_AnalysisManager->GetH1(1)->rms(), "Energy") << G4endl;
  G4cout << "Total Scintillating energy deposited"
         << ": mean = " << G4BestUnit(m_AnalysisManager->GetH1(2)->mean(), "Energy")
         << " rms = " << G4BestUnit(m_AnalysisManager->GetH1(2)->rms(), "Energy") << G4endl;
  G4cout << "Total Cherenkov energy deposited"
         << ": mean = " << G4BestUnit(m_AnalysisManager->GetH1(3)->mean(), "Energy")
         << " rms = " << G4BestUnit(m_AnalysisManager->GetH1(3)->rms(), "Energy") << G4endl;
  G4cout << "Total leaked energy"
         << ": mean = " << G4BestUnit(m_AnalysisManager->GetH1(4)->mean(), "Energy")
         << " rms = " << G4BestUnit(m_AnalysisManager->GetH1(4)->rms(), "Energy") << G4endl;
  G4cout << "Primary energy"
         << ": mean = " << G4BestUnit(m_AnalysisManager->GetH1(5)->mean(), "Energy")
         << " rms = " << G4BestUnit(m_AnalysisManager->GetH1(5)->rms(), "Energy") << G4endl;
  G4cout << "Preshower energy"
         << ": mean = " << G4BestUnit(m_AnalysisManager->GetH1(6)->mean(), "Energy")
         << " rms = " << G4BestUnit(m_AnalysisManager->GetH1(6)->rms(), "Energy") << G4endl;
  G4cout << "Scintillating photoelectrons"
         << ": mean = " << m_AnalysisManager->GetH1(7)->mean() << " rms = " << m_AnalysisManager->GetH1(6)->rms()
         << G4endl;
  G4cout << "Cherenkov photoelectrons"
         << ": mean = " << m_AnalysisManager->GetH1(8)->mean() << " rms = " << m_AnalysisManager->GetH1(7)->rms()
         << G4endl;
  G4cout << "=================================================================="
            "===="
         << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
