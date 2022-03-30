// HEADER //

#ifndef BucatiniMaterials_h
#define BucatiniMaterials_h

#include <string>

class G4Material;
class G4NistManager;

class HidraMaterials {
public:
  virtual ~HidraMaterials();
  
  // Singleton class with all materials
  static HidraMaterials* instance();

  G4Material* GetMaterial(const std::string&);

private:
  // Singleton = private contructor!
  HidraMaterials();
  void CreateMaterials();

  static HidraMaterials* m_Instance;
  G4NistManager* m_NistMan;
  G4Material* m_Air;
  G4Material* m_Silicon;
  G4Material* m_Teflon;
  G4Material* m_Brass;
  G4Material* m_Polystyrene;
  G4Material* m_PmmaScin;
  G4Material* m_PmmaCher;
  G4Material* m_FluorinatedPolymer;
  G4Material* m_Glass;
};

#endif
