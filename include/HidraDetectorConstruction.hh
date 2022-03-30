// HEADER // 

#ifndef HidraDetectorConstruction_h
#define HidraDetectorConstruction_h

// Includers from Geant4
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"

// Includes from this project
#include "HidraMaterials.hh"

// STL includes
#include <array>
#include <string>
#include <stdint.h>

// For faster compilation use ahead declaration of needed classes
class G4Material;
class G4VPhysicalVolume;
class G4Region;
class G4LogicalVolume;

// ----------------------------------------------------- //
// Geometrical constants that can be used in other files //
// ----------------------------------------------------- //

// Basic unit of the calorimeter is a 32x16 module (half of a real module)

// General geometry parameters
// The calorimeter is made froma a 10 x 24 square of modules
static constexpr int nModuleRows = 10;
static constexpr int nModuleCols = 24;
// Each module contains 32 rows made of 16 fibers each
static constexpr int nFiberRows = 32;
static constexpr int nFiberCols = 16;
// Not all modules are placed in the 10 x 24 shape
static constexpr std::array<int, nModuleRows> nModulesInRow = {10, 10, 20, 20, 24, 24, 20, 20, 10, 10};
static constexpr int nTotalModules = 168;
static constexpr int nSensors = nFiberCols * nFiberRows * nTotalModules;

// Tubes and fiber dimensions
// TODO: Check this sizes!
static constexpr double tubeOuterRadius = 1 * mm; 
static constexpr double tubeInnerRadius = 0.5 * mm;
static constexpr double fiberCoreRadius = 0.485 * mm;
static constexpr double tubeHalfSizeZ = 1000 * mm;
static constexpr double fiberCladInnerRadius = fiberCoreRadius;
static constexpr double fiberCladOuterRadius = 0.5 * mm;
static constexpr double fiberHalfSizeZ = tubeHalfSizeZ;

// Single module dimensions
static constexpr double moduleHalfSizeX = tubeOuterRadius * nFiberCols;
static constexpr double modulehalfSizeY = tubeOuterRadius * 1.733 * nFiberRows / 2;
static constexpr double modulehalfSizeZ = 1000. * mm;

// Size of full calorimeter
static constexpr double caloHalfSizeX = nModuleCols * moduleHalfSizeX;
static constexpr double caloHalfSizeY = nModuleRows * modulehalfSizeY;
static constexpr double caloHalfSizeZ = modulehalfSizeZ;
static constexpr double caloAngleX = 1.0 * deg;
static constexpr double caloAngleY = 0.0 * deg;

// Geometry parameters of the world, world is a G4Box
static constexpr double worldHalfSizeX = 1.5 * caloHalfSizeX;
static constexpr double worldhalfSizeY = 1.5 * caloHalfSizeY;
static constexpr double worldHalfSizeZ = 2 * caloHalfSizeZ;

// Geometry parameters of the SiPM
static constexpr double sipmHalfSizeX = 1.1 / 2 * mm;
static constexpr double sipmhalfSizeY = sipmHalfSizeX;
static constexpr double sipmhalfSizeZ = 500 / 2 * um;

// Geometry parameters of the SiPM, active silicon layer
static constexpr double sipmSiliconHalfSizeX = 1. / 2 * mm;
static constexpr double sipmSiliconHalfSizeY = sipmSiliconHalfSizeX;
static constexpr double sipmSiliconHalfSizeZ = 300 / 2* um;

class HidraDetectorConstruction : public G4VUserDetectorConstruction {
public:
  HidraDetectorConstruction();
  virtual ~HidraDetectorConstruction();

  virtual G4VPhysicalVolume* Construct();
  virtual void ConstructSDandField();

  void ConstructMaterials();
  G4VPhysicalVolume* getLeakageCounterPV() const { return m_LeakageCounter; }
  G4VPhysicalVolume* getWorldPV() const { return m_World; }

private:
  // Use helper class for building, storing and retrieving materials
  void DefineMaterials();
  G4Material* FindMaterial(std::string name) { return m_Materials->GetMaterial(name); }

  bool m_CheckOverlaps;
  
  // Define a region for fast optical photon transport
  G4Region* m_FastOpticalPhotonRegion = nullptr;
  G4VPhysicalVolume* m_LeakageCounter = nullptr;
  G4VPhysicalVolume* m_World = nullptr;
  // Helper class with materials
  HidraMaterials* m_Materials;
}; // class

#endif // Header guard
