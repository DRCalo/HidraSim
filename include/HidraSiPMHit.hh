#ifndef BucatiniHit_h
#define BucatiniHit_h 1

#include "G4Allocator.hh"
#include "G4THitsCollection.hh"
#include "G4Threading.hh"
#include "G4VHit.hh"
#include <vector>

class HidraSiPMHit : public G4VHit {
public:
  HidraSiPMHit();
  virtual ~HidraSiPMHit();

  inline void* operator new(size_t);
  inline void operator delete(void*);

  virtual void Print();

private:
  friend class HidraSiPMSD;
  friend class HidraEventAction;

  std::vector<double> m_Times;       // Photon hit time
  std::vector<double> m_Wavelengths; // Photon wlen
  
  int m_NPhotoelectrons = 0;         // Photons detected
  double m_Integral = 0;
  double m_Toa = 0;
};

using HidraSiPMHitsCollection = G4THitsCollection<HidraSiPMHit>;

extern G4ThreadLocal G4Allocator<HidraSiPMHit>* HidraSiPMHitAllocator;

inline void* HidraSiPMHit::operator new(size_t) {
  if (!HidraSiPMHitAllocator) {
    HidraSiPMHitAllocator = new G4Allocator<HidraSiPMHit>;
  }
  return (void*)HidraSiPMHitAllocator->MallocSingle();
}

inline void HidraSiPMHit::operator delete(void* hit) {
  if (!HidraSiPMHitAllocator) {
    HidraSiPMHitAllocator = new G4Allocator<HidraSiPMHit>;
  }
  HidraSiPMHitAllocator->FreeSingle((HidraSiPMHit*)hit);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
