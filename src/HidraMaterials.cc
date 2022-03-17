#include "HidraMaterials.hh"
#include "G4NistManager.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"

#include <math.h>

HidraMaterials* HidraMaterials::m_Instance = nullptr;

HidraMaterials::HidraMaterials() {
  // Get Nist manager and create materials
  m_NistMan = G4NistManager::Instance();
  CreateMaterials();
}

HidraMaterials::~HidraMaterials() {
  if (m_Instance != nullptr)
    delete m_Instance;
  m_Instance = nullptr;
}

HidraMaterials* HidraMaterials::Instance() {
  if (m_Instance == nullptr)
    m_Instance = new HidraMaterials();
  return m_Instance;
}


G4Material* HidraMaterials::GetMaterial(const std::string& materialName) {
  // Look for material in NIST manager
  G4Material* materialPtr = m_NistMan->FindOrBuildMaterial(materialName);

  if (!materialPtr)
    materialPtr = G4Material::GetMaterial(materialName);
  if (!materialPtr) {
    std::ostringstream ss;
    ss << "Material " << materialName << " not found!";
    G4Exception("HidraMaterials::GetMaterial", "", FatalException, ss.str().c_str());
  }

  return materialPtr;
}

void HidraMaterials::CreateMaterials() {

  // --------------------------------------- //
  // Define Elements, Mixtures and Materials //
  // --------------------------------------- //

  // Elements //
  m_NistMan = G4NistManager::Instance();

  G4Element* elementH = m_NistMan->FindOrBuildElement("H"); // Hidrogen
  G4Element* elementC = m_NistMan->FindOrBuildElement("C"); // Carbon
  G4Element* elementO = m_NistMan->FindOrBuildElement("O");// Oxygen
  G4Element* elementSi = m_NistMan->FindOrBuildElement("Si");// Silicon
  G4Element* elementF = m_NistMan->FindOrBuildElement("F");// Fluorine
  G4Element* elementCu = m_NistMan->FindOrBuildElement("Cu");// Copper
  G4Element* elementZn = m_NistMan->FindOrBuildElement("Zn"); // Zinc

  // Materials //
  m_NistMan = G4NistManager::Instance();
  m_NistMan->FindOrBuildMaterial("G4_Cu");
  m_NistMan->FindOrBuildMaterial("G4_Si");
  m_NistMan->FindOrBuildMaterial("G4_AIR");
  m_NistMan->FindOrBuildMaterial("G4_TEFLON");
  m_NistMan->FindOrBuildMaterial("G4_SILICON_DIOXIDE");
  m_NistMan->FindOrBuildMaterial("G4_Pyrex_Glass");
  m_NistMan->FindOrBuildMaterial("G4_BRASS");
  m_NistMan->FindOrBuildMaterial("G4_POLYSTYRENE");
  m_NistMan->FindOrBuildMaterial("G4_PLEXIGLASS");
  
  // Get material pointers
  m_Air = G4Material::GetMaterial("G4_AIR");
  m_Teflon = G4Material::GetMaterial("G4_TEFLON");
  m_Silicon = G4Material::GetMaterial("G4_Si");
  m_Glass = G4Material::GetMaterial("G4_Pyrex_Glass");
  m_Brass = G4Material::GetMaterial("G4_BRASS");
  m_Polystyrene = G4Material::GetMaterial("G4_POLYSTYRENE");
  m_PmmaScin = G4Material::GetMaterial("G4_PLEXIGLASS");
  m_PmmaCher = G4Material::GetMaterial("G4_PLEXIGLASS");

  // Fluorinated Polymer material from elements (C2F2)
  m_FluorinatedPolymer = new G4Material("fluorinatedPolymer", 1.43 * g / cm3, 2);
  m_FluorinatedPolymer->AddElement(elementC, 2);
  m_FluorinatedPolymer->AddElement(elementF, 2);
  
  // Setup optical properties
  G4MaterialPropertiesTable* MPTAir = new G4MaterialPropertiesTable();
  G4MaterialPropertiesTable* MPTSilicon = new G4MaterialPropertiesTable();
  G4MaterialPropertiesTable* MPTPMMAScin = new G4MaterialPropertiesTable();
  G4MaterialPropertiesTable* MPTPMMACher = new G4MaterialPropertiesTable();
  G4MaterialPropertiesTable* MPTFluoPoly = new G4MaterialPropertiesTable();
  G4MaterialPropertiesTable* MPTGlass = new G4MaterialPropertiesTable();
  G4MaterialPropertiesTable* MPTPolystyrene = new G4MaterialPropertiesTable();
  
  // Wavelength
  // 600., 590., 580., 570., 560., 550., 540., 530.,
  // 520., 510., 500., 490., 480., 470., 460., 450.,
  // 440., 430., 420., 400., 350., 300.
  std::vector<double> photonEnergy = {
      2.06640 * eV, 2.10143 * eV, 2.13766 * eV, 2.17516 * eV, 2.21400 * eV, 2.25426 * eV, 2.29600 * eV, 2.33932 * eV,
      2.38431 * eV, 2.43106 * eV, 2.47968 * eV, 2.53029 * eV, 2.58300 * eV, 2.63796 * eV, 2.69531 * eV, 2.75520 * eV,
      2.81782 * eV, 2.88335 * eV, 2.95200 * eV, 3.09960 * eV, 3.54241 * eV, 4.13281 * eV};

  const int nEntries = photonEnergy.size();
  
  // Needed ?
  std::vector<double> refractiveindexAir(nEntries, 1.0);
  MPTAir->AddProperty("RINDEX", photonEnergy, refractiveindexAir);
  m_Air->SetMaterialPropertiesTable(MPTAir);

  // ---------------------------------------------- //
  // Optical properties of Scintillating fiber core //
  // ---------------------------------------------- //
  // https://refractiveindex.info
  std::vector<double> refractiveindexPolystyrene = {
      1.5882,  1.58898, 1.58996, 1.59084, 1.59217, 1.59338, 1.59443, 1.59587, 1.59721, 1.59862, 1.60021,
      1.60201, 1.60372, 1.60594, 1.60797, 1.61037, 1.61279, 1.61546, 1.61834, 1.62552, 1.63,    1.63};
  std::vector<double> absLenPolystyrene = {5.429 * m, 13.00 * m, 14.50 * m, 16.00 * m, 18.00 * m, 16.50 * m,
                                           17.00 * m, 14.00 * m, 16.00 * m, 15.00 * m, 14.50 * m, 13.00 * m,
                                           12.00 * m, 10.00 * m, 8.000 * m, 7.238 * m, 4.000 * m, 1.200 * m,
                                           0.500 * m, 0.200 * m, 0.200 * m, 0.100 * m};
  // Saint Gobain BCF-10 product catalog
  std::vector<double> emissionPolystyrene = {0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.02, 0.04, 0.06, 0.09, 0.10,
                                             0.12, 0.20, 0.30, 0.40, 0.60, 0.80, 1.00, 0.70, 0.0,  0.0,  0.0};
  MPTPolystyrene->AddProperty("RINDEX", photonEnergy, refractiveindexPolystyrene);
  MPTPolystyrene->AddProperty("ABSLENGTH", photonEnergy, absLenPolystyrene);
  MPTPolystyrene->AddProperty("SCINTILLATIONCOMPONENT1", photonEnergy, emissionPolystyrene);
  MPTPolystyrene->AddConstProperty("SCINTILLATIONYIELD", 1000. / MeV);
  MPTPolystyrene->AddConstProperty("RESOLUTIONSCALE", 1.0);
  MPTPolystyrene->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.7 * ns);

  m_Polystyrene->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);
  m_Polystyrene->SetMaterialPropertiesTable(MPTPolystyrene);

  // -------------------------------------------- //
  // Optical properties of Scintillating cladding //
  // -------------------------------------------- //
  // https://refractiveindex.info
  std::vector<double> refractiveIndexPmma = {1.49216, 1.4927,  1.49324, 1.4937,  1.49405, 1.49463, 1.49521, 1.49594,
                                             1.49639, 1.49722, 1.49795, 1.49873, 1.49955, 1.50056, 1.50114, 1.50227,
                                             1.50342, 1.50426, 1.50539, 1.50818, 1.51,    1.52};
  std::vector<double> absLenPmma = {39.48 * m, 48.25 * m, 54.29 * m, 57.91 * m, 54.29 * m, 33.40 * m,
                                    31.02 * m, 43.43 * m, 43.43 * m, 41.36 * m, 39.48 * m, 37.76 * m,
                                    36.19 * m, 36.19 * m, 33.40 * m, 31.02 * m, 28.95 * m, 25.55 * m,
                                    24.13 * m, 21.71 * m, 2.171 * m, 0.434 * m};
  MPTPMMAScin->AddProperty("RINDEX", photonEnergy, refractiveIndexPmma);
  MPTPMMAScin->AddProperty("ABSLENGTH", photonEnergy, absLenPmma);
  m_PmmaScin->SetMaterialPropertiesTable(MPTPMMAScint);

  // ------------------------------------------ //
  // Optical properties of Cherenkov fiber core //
  // ------------------------------------------ //
  // https://refractiveindex.info
  MPTPMMACher->AddProperty("RINDEX", photonEnergy, refractiveIndexPmma);
  MPTPMMACher->AddProperty("ABSLENGTH", photonEnergy, absLenPmma);
  m_PmmaCher->SetMaterialPropertiesTable(MPTPMMACher);

  // ---------------------------------------------- //
  // Optical properties of Cherenkov fiber cladding //
  // ---------------------------------------------- //
  std::vector<double> refractiveIndexFluoPoly(nEntries, 1.42);
  MPTFluoPoly->AddProperty("RINDEX", photonEnergy, refractiveIndexFluoPoly);
  m_FluorinatedPolymer->SetMaterialPropertiesTable(MPTFluoPoly);

  // --------------------------- //
  // Optical properties of Glass //
  // --------------------------- //
  std::vector<double> refractiveIndexGlass(nEntries, 1.51);
  MPTGlass->AddProperty("RINDEX", photonEnergy, refractiveIndexGlass);
  m_Glass->SetMaterialPropertiesTable(MPTGlass);

  // ------------------------------ //
  // Optical properties of Silicon  //
  // ------------------------------ //
  std::vector<double> refractiveIndexSilicon = {3.94,  3.963, 3.988, 4.015, 4.045, 4.077, 4.112, 4.151,
                                                4.193, 4.241, 4.294, 4.35,  4.419, 4.497, 4.587, 4.691,
                                                4.812, 4.949, 5.119, 5.613, 5.494, 4.976};

  std::vector<double> absLenSilicon(nEntries, 0.001 * mm);

  MPTSilicon->AddProperty("RINDEX", photonEnergy, refractiveIndexSilicon);
  MPTSilicon->AddProperty("ABSLENGTH", photonEnergy, absLenSilicon);
  m_Silicon->SetMaterialPropertiesTable(MPTSilicon);
}
