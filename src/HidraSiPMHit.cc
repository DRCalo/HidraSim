#include "HidraSiPMHit.hh"
#include "G4UnitsTable.hh"

#include <iostream>

G4ThreadLocal G4Allocator<HidraSiPMHit>* HidraSiPMHitAllocator = nullptr;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

HidraSiPMHit::HidraSiPMHit() : G4VHit() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

HidraSiPMHit::~HidraSiPMHit() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void HidraSiPMHit::Print() {
  if (m_Times.size() > 0) {
    G4cout << "========================================" << G4endl;
    G4cout << "Integral: " << std::setw(7) << m_Integral << " Toa: " << std::setw(7) << m_Toa << G4endl;
  }
  for (size_t i = 0; i < m_Times.size(); ++i) {
    G4cout << "Time: " << std::setw(7) << G4BestUnit(m_Times[i], "Time") << " Wavelength: " << std::setw(7)
           << G4BestUnit(m_Wavelengths[i], "Length") << G4endl;
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
