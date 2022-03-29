#ifndef HistoManager_h
#define HistoManager_h 1

#include "G4AnalysisManager.hh"
#include <vector>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class HidraHistoManager {
public:
  HidraHistoManager();

  void Book();
  void Save();

  void FillHisto(const int, const double , const double weight = 1.0);

  void FillNtuple(const double, const double, const double, const double, const double, const double, const double, const int,
                  const int, const int, const int, const std::vector<double>&, const std::vector<double>&,
                  const std::vector<int>&, const std::vector<int>&);
  void setSiPMIntegral(const std::vector<double>&);
  void setSiPMToa(const std::vector<double>&);
  void setSiPMPhotoelectrons(const std::vector<int>&);
  void setSiPMPhotons(const std::vector<int>&);
  void PrintStatistic();

private:
  std::vector<double> m_SipmIntegral;
  std::vector<double> m_SipmToa;
  std::vector<int> m_SipmPhotoelectrons;
  std::vector<int> m_SipmPhotons;
  bool m_FactoryOn;
  G4AnalysisManager* m_AnalysisManager;
};

inline void HidraHistoManager::setSiPMIntegral(const std::vector<double>& x) { m_SipmIntegral = x; }
inline void HidraHistoManager::setSiPMToa(const std::vector<double>& x) { m_SipmToa = x; }
inline void HidraHistoManager::setSiPMPhotoelectrons(const std::vector<int>& x) { m_SipmPhotoelectrons = x; }
inline void HidraHistoManager::setSiPMPhotons(const std::vector<int>& x) { m_SipmPhotons = x; }
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
