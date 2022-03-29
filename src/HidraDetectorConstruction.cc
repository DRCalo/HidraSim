#include "HidraDetectorConstruction.hh"
#include "G4Material.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4Sphere.hh"
#include "G4Tubs.hh"

#include "G4SDManager.hh"

#include "G4Colour.hh"
#include "G4VisAttributes.hh"

#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4PVReplica.hh"


HidraDetectorConstruction::HidraDetectorConstruction() : G4VUserDetectorConstruction(), fCheckOverlaps(false) {
  DefineMaterials();
}

HidraDetectorConstruction::~HidraDetectorConstruction() {}

void HidraDetectorConstruction::DefineMaterials() { fMaterials = HidraMaterials::GetInstance(); }

G4VPhysicalVolume* HidraDetectorConstruction::Construct() {
  // Assign material to the calorimeter volumes
  G4Material* airMaterial = FindMaterial("G4_AIR");
  G4Material* vacuumMaterial = FindMaterial("G4_Galactic");
  G4Material* siliconMaterial = FindMaterial("G4_Si");
  G4Material* teflonMaterial = FindMaterial("G4_TEFLON");
  G4Material* tubesMaterial = FindMaterial("G4_BRASS");
  G4Material* scintillatingCoreMaterial = FindMaterial("G$_POLYSTIRENE");
  G4Material* cherenkovCoreMaterial = FindMaterial("pmmaCher");
  G4Material* scintillatingCladMaterial = FindMaterial("pmmaScint");
  G4Material* cherenkovCladMaterial = FindMaterial("fluorinatedPolymer");
  G4Material* glassMaterial = FindMaterial("glass");

  // ------------------- //
  // Building geometries //
  // ------------------- //

  // World
  G4VSolid* worldS = new G4Box("world", worldHalfSizeX, worldhalfSizeY, worldHafSizeZ);
  G4LogicalVolume* worldLV = new G4LogicalVolume(worldS, airMaterial, "world");
  G4VPhysicalVolume* worldPV = new G4PVPlacement(0, G4ThreeVector(), worldLV, "world", 0, false, 0, fCheckOverlaps);

  // Absorber to calculate leakage
  G4VSolid* leakageAbsorberS = new G4Sphere("leakageAbsorber", 3 * m, 2.8 * m, 0 * deg, 360 * deg, 0 * deg, 180 * deg);
  G4LogicalVolume* leakageAbsorberLV = new G4LogicalVolume(leakageAbsorberS, airMaterial, "leakageAbsorber");
  new G4PVPlacement(0, G4ThreeVector(), leakageAbsorberLV, "leakageAbsorber", worldLV, false, 0, fCheckOverlaps);

  // Calorimeter placement (with rotation wrt beam axis)
  G4RotationMatrix calorimeterRotiationMatrix = G4RotationMatrix();
  calorimeterRotiationMatrix.rotateY(angleX);
  calorimeterRotiationMatrix.rotateX(angleY);
  G4ThreeVector calorimeterPosition(0,0,0);
  G4Transform3D calorimeterTransform = G4Transform3D(calorimeterRotiationMatrix, calorimeterPosition);

  G4VSolid* calorimeterS = new G4Box("calorimeter", caloHalfSizeX, caloHalfSizeY, caloHalfSizeZ + sipmHalfSizeZ);
  G4LogicalVolume* calorimeterLV = new G4LogicalVolume(calorimeterS, airMaterial, "calorimeter");
  new G4PVPlacement(calorimeterTransform, calorimeterLV, "calorimeter", worldLV, false, 0, fCheckOverlaps);

  // Single calorimeter module 64x80 fibers
  G4VSolid* moduleS = new G4Box("module", moduleHalfSizeX, moduleHalfSizeY, moduleHalfSizeZ + sipmaHalfSizeZ);
  G4LogicalVolume* moduleLV = new G4LogicalVolume(moduleS, airMaterial, "module");

  // Modules equipped placement
  int moduleCopyNumber = 0;
  for (int row = 0; row < nModuleRows; row++) {
    for (int col = 0; col < nModulesInRow[row]; col++) {
      const double modulePositionX = -(nModulesInRow[row] - 1) * moduleHalfSizeX + 2*moduleHalfSizeX * col;
      const double modulePositionY = -(nModuleRows - 1) * moduleHalfSizeY + 2*moduleHalfSizeY * row;

      G4ThreeVector modulePosition(modulePositionX, modulePositionY, 2*sipmhalfSizeZ);

      new G4PVPlacement(0, modulePosition, moduleLV, "module" + std::to_string(moduleCopyNumber), calorimeterLV, false, moduleCopyNumber);
      std::cout << "Placing module: " << moduleCopyNumber << " in X: " << modulePositionX << " Y: " << modulePositionY << "\n";
      moduleCopyNumber++;
    }
  }

  // Single sipm volume
  G4VSolid* sipmS = new G4Box("sipm", sipmHalfSizeX, sipmHalfSizeY, sipmHalfSizeZ);
  G4LogicalVolume* sipmLV = new G4LogicalVolume(sipmS, glassMaterial, "sipm");

  // Silicon inside sipm
  G4VSolid* siliconS = new G4Box("sipmSilicon", sipmSiliconHalfSizeX, sipmSiliconHalfSizeY, sipmSiliconHalfSizeZ);
  G4LogicalVolume* siliconLV = new G4LogicalVolume(siliconS, siliconMaterial, "sipmSilicon");

  const G4ThreeVector siliconPosition(0, 0, -sipmHalfSizeZ + sipmSiliconHalfSizeZ);
  new G4PVPlacement(0, siliconPosition, siliconLV, "sipmSilicon", sipmLV, false, 0, fCheckOverlaps);

  m_FastOpticalPhotonRegion = new G4Region("opticalFiberRegion");

  // Tube
  G4Tubs* tubeS = new G4Tubs("tube", tubeInnerRadius, tubeOuterRadius, tubeHalfSizeZ, 0 * deg, 360 * deg);
  G4LogicalVolume* tubeScinLV = new G4LogicalVolume(tubeS, tubesMaterial, "tubeScin");
  G4LogicalVolume* tubeCherLV = new G4LogicalVolume(tubeS, tubesMaterial, "tubeCher");

  // Clad and core solid
  // Cladding must have inner radius of 0!
  G4Tubs* fiberCladS = new G4Tubs("fiberClad", 0, fiberCladOuterRadius, fiberHalfSizeZ, 0 * deg, 360 * deg);
  G4Tubs* fiberCoreS = new G4Tubs("fiberCore", 0, fiberCoreRadius, fiberHalfSizeZ, 0 * deg, 360 * deg);

  // S and C clad logical
  G4LogicalVolume* fiberCladScinLV = new G4LogicalVolume(fiberCladS, scintillatingCladMaterial, "fiberScinClad");
  G4LogicalVolume* fiberCoreScinLV = new G4LogicalVolume(fiberCoreS, scintillatingCoreMaterial, "fiberScinCore");
  
  G4LogicalVolume* fiberCladCherLV = new G4LogicalVolume(fiberCladS, cherenkovCladMaterial, "fiberCherClad");
  G4LogicalVolume* fiberCoreCherLV = new G4LogicalVolume(fiberCoreS, cherenkovCoreMaterial, "fiberCherCore");
  
  // Placing fiber and clad in tube
  new G4PVPlacement(0,G4ThreeVector(),fiberCladScinLV,"fiberScinClad",tubeScinLV,false,0,fCheckOverlaps);
  new G4PVPlacement(0,G4ThreeVector(),fiberCoreScinLV,"fiberScinCore",fiberCladScinLV,false,0,fCheckOverlaps);  
  new G4PVPlacement(0,G4ThreeVector(),fiberCladCherLV,"fiberCherClad",tubeCherLV,false,0,fCheckOverlaps);
  new G4PVPlacement(0,G4ThreeVector(),fiberCoreCherLV,"fiberCherCore",fiberCladCherLV,false,0,fCheckOverlaps);
  
  // Cladding is added in region
  fiberCladScinLV->SetRegion(m_FastOpticalPhotonRegion);
  fiberCladCherLV->SetRegion(m_FastOpticalPhotonRegion);
  m_FastOpticalPhotonRegion->AddRootLogicalVolume(fiberCladScinLV);
  m_FastOpticalPhotonRegion->AddRootLogicalVolume(fiberCladCherLV);
  
  // Use replicas to reduce memory usage and faster geo navigation
  // Row of fibers is a replica along X axis
  G4VSolid* fibersRowS = new G4Box("fiberRow", moduleHalfSizeX, tubeOuterRadius, modulehalfSizeZ);
  G4LogicalVolume* tubesScinRowLV = new G4LogicalVolume(fibersRowS, airMaterial, "fiberScinRow");
  G4LogicalVolume* tubesCherRowLV = new G4LogicalVolume(fibersRowS, airMaterial, "fiberCherRow");
  
  // N fibers spread by 2*radius filling volume along X axis
  new G4PVReplica("tubeScinRow", tubeScinLV, tubesScinRowLV, kXAxis, nFibersCols, 2*tubeOuterRadius,0);
  new G4PVReplica("tubeCherRow", tubeCherLV, tubesCherRowLV, kXAxis, nFibersCols, 2*tubeOuterRadius,0);

  G4VSolid* sipmRowS = new G4Box("sipmRow", moduleHalfSizeX, tubeOuterRadius, sipmhalfSizeZ);
  G4LogicalVolume* sipmRowLV = new G4LogicalVolume(sipmRowS, teflonMaterial, "sipmRow");
  
  // Repeated placement of sipm along X axis
  new G4PVReplica("sipmRow", sipmLV, sipmRowLV, kXAxis, nFibersCols, 2*tubeOuterRadius,0);
  
  // Place sipm and fibers
  // Each fiber is identified by Module ID, Replica No and Copy No.
  int copyNo = 0;
  for(int i=0; i<nFibersRows; ++i){
    const double xpos = (i%2 == 0) * tubeOuterRadius;
    const double ypos = -modulehalfSizeY + i * std::sqrt(3) * tubeOuterRadius + tubeOuterRadius;
    if(i % 2 == 0){
      new G4PVPlacement(0,G4ThreeVector(xpos,ypos,0),tubesScinRowLV, "tubeScinRow", moduleLV,false,copyNo,fCheckOverlaps);
    }else{
      new G4PVPlacement(0,G4ThreeVector(xpos,ypos,0),tubesCherRowLV, "tubeCherRow", moduleLV,false,copyNo,fCheckOverlaps);
    }
    new G4PVPlacement(0,G4ThreeVector(xpos,ypos,-fiberHalfSizeZ-sipmhalfSizeZ),sipmRowLV,"sipmRow",moduleLV,false,copyNo,fCheckOverlaps);
    copyNo++;
  }


  // Vis attributes
  G4VisAttributes* tubeVisAtt = new G4VisAttributes(G4Colour(0.6, 0.3, 0.3));
  tubeVisAtt->SetVisibility(true);
  tubeVisAtt->SetForceSolid(true);
  tubeScinLV->SetVisAttributes(tubeVisAtt);
  tubeCherLV->SetVisAttributes(tubeVisAtt);
  
  G4VisAttributes* scinCoreVisAtt = new G4VisAttributes(G4Colour(0.0, 0.0, 0.8));
  scinCoreVisAtt->SetVisibility(true);
  scinCoreVisAtt->SetForceSolid(true);
  fiberCoreScinLV->SetVisAttributes(scinCoreVisAtt);

  G4VisAttributes* scinCladVisAtt = new G4VisAttributes(G4Colour(0.2, 0.2, 0.8));
  scinCladVisAtt->SetVisibility(true);
  scinCladVisAtt->SetForceSolid(true);
  fiberCladScinLV->SetVisAttributes(scinCladVisAtt);

  G4VisAttributes* cherCoreVisAtt = new G4VisAttributes(G4Colour(0.8, 0.0, 0.0));
  cherCoreVisAtt->SetVisibility(true);
  cherCoreVisAtt->SetForceSolid(true);
  fiberCoreCherLV->SetVisAttributes(cherCoreVisAtt);

  G4VisAttributes* cherCladVisAtt = new G4VisAttributes(G4Colour(0.8, 0.2, 0.2));
  cherCladVisAtt->SetVisibility(true);
  cherCladVisAtt->SetForceSolid(true);
  fiberCladCherLV->SetVisAttributes(cherCladVisAtt);
 
  G4VisAttributes* siliconVisAtt = new G4VisAttributes(G4Colour(0.0, 0.8, 0.0));
  tubeVisAtt->SetVisibility(true);
  tubeVisAtt->SetForceSolid(true);
  siliconLV->SetVisAttributes(siliconVisAtt); 
  
  m_LeakageCounter = leakageAbsorberPV;
  return worldPV;
}

void HidraDetectorConstruction::ConstructSDandField() {
  sipm::SiPMProperties sipmProperties;
  sipm::SiPMProperties sipmPropertiesScin;
  sipm::SiPMProperties sipmPropertiesCher;

  // Common sipm properties
  sipmProperties.setDcr(250e3);
  sipmProperties.setXt(0.01);
  sipmProperties.setAp(0.01);
  sipmProperties.setSnr(25);
  sipmProperties.setSignalLength(500);
  sipmProperties.setSampling(1);
  sipmProperties.setPitch(15);
  sipmProperties.setRiseTime(1);
  sipmProperties.setFallTimeFast(25);
  sipmProperties.setRecoveryTime(25);

  const std::vector<double> wlen = {280, 300, 325, 350, 375, 400, 425, 450, 475, 500, 525,
                                    550, 575, 600, 625, 650, 675, 700, 800, 900, 1000};
  const std::vector<double> sipmpde = {0,    0.12, 0.16, 0.21, 0.22, 0.27, 0.30, 0.32, 0.32, 0.31, 0.30,
                                       0.27, 0.25, 0.23, 0.21, 0.20, 0.17, 0.14, 0.07, 0.03, 0.01};
  const std::vector<double> wratten3 = {1.,   1.,    1.,    1.,    1.,    1.,    1.,    0.5,   0.067, 0.033, 0.025,
                                        0.02, 0.017, 0.017, 0.017, 0.017, 0.017, 0.017, 0.017, 0.017, 0.017};

  std::vector<double> pdeS(sipmpde.size());
  std::vector<double> pdeC(sipmpde.size());

  for (size_t i = 0; i < sipmpde.size(); ++i) {
    pdeS[i] = sipmpde[i] * wratten3[i];
    pdeC[i] = 0.63 * sipmpde[i];
  }

  sipmPropertiesScin = sipmProperties;
  sipmPropertiesCher = sipmProperties;
  sipmProperties.setPdeSpectrum(wlen, pdeS);
  sipmPropertiesCher.setPdeSpectrum(wlen, pdeC);

  // Pointer is deallocated in HidraSD~
  sipm::SiPMSensor* sipmS = new sipm::SiPMSensor(sipmPropertiesScin);
  sipm::SiPMSensor* sipmC = new sipm::SiPMSensor(sipmPropertiesCher);

  HidraSD* sipmSD = new HidraSD("sipmSD", "sipmHitsCollection", nSensor, sipmS, sipmC);
  G4SDManager::GetSDMpointer()->AddNewDetector(sipmSD);
  SetSensitiveDetector("sipmSilicon", sipmSD);

  new FastSimModelOpFiber("FastSimOpFiberRegion", fRegion);
}
