#ifndef BucatiniSD_h
#define BucatiniSD_h 1

#include "G4VSensitiveDetector.hh"
#include "HidraSiPMHit.hh"
#include "SiPM.h"

class G4Step;
class G4HCofThisEvent;
class HidraSiPMHit;

class HidraSiPMSD : public G4VSensitiveDetector {
public:
  HidraSiPMSD(const std::string&, const std::string&, const int, sipm::SiPMSensor*);
  virtual ~HidraSiPMSD();

  virtual void Initialize(G4HCofThisEvent* hitCollection);
  virtual bool ProcessHits(G4Step* step, G4TouchableHistory* history);
  virtual void EndOfEvent(G4HCofThisEvent* hitCollection);

private:
  void DoSiPMSim(HidraSiPMHit*);

  HidraSiPMHitsCollection* m_HitsCollection;
  const int m_nSiPM;
  sipm::SiPMSensor* m_SiPMSensor;
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
