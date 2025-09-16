//**************************************************
// \file DREMTubesSignalHelper.cc
// \brief: Implementation of DREMTubesSignalHelper
//         class
// \author: Lorenzo Pezzotti (CERN EP-SFT-sim)
//          @lopezzot
// \start date: 1 September 2021
//**************************************************

//Includers from project files
//
#include "DREMTubesSignalHelper.hh"

//Includers from Geant4
#include "G4Poisson.hh"
#include "G4Tubs.hh"
#include "G4NavigationHistory.hh"

DREMTubesSignalHelper* DREMTubesSignalHelper::instance = 0;

//Define (private) constructor (singleton)
//
DREMTubesSignalHelper::DREMTubesSignalHelper(){}

//Define Instance() method
//
DREMTubesSignalHelper* DREMTubesSignalHelper::Instance(){
    if (instance==0){
        instance = new DREMTubesSignalHelper;
    }
    return DREMTubesSignalHelper::instance;
}

//Define ApplyBirks() method
//
G4double DREMTubesSignalHelper::ApplyBirks( const G4double& de, const G4double& steplength ) {
		
    const G4double k_B = 0.126; //Birks constant
    return (de/steplength) / ( 1+k_B*(de/steplength) ) * steplength;

}

//Define SmearSSignal() method for PMTs
//
G4int DREMTubesSignalHelper::SmearSSignalPMT( const G4double& satde ) {
		
    return G4Poisson(satde*9.5);
		
}

//Define SmearCSignal() method for PMTs
//
G4int DREMTubesSignalHelper::SmearCSignalPMT( ){
		
    return G4Poisson(0.153);

}

//Define SmearSSignal() method for SiPMs
//
G4int DREMTubesSignalHelper::SmearSSignalSiPM( const G4double& satde ) {
		
    return G4Poisson(satde*9.5);
		
}

//Define SmearCSignal() method for SiPMs
//
G4int DREMTubesSignalHelper::SmearCSignalSiPM( ){
		
    return G4Poisson(0.153);

}


//Define GetDistanceToSiPM() method
//
G4double DREMTubesSignalHelper::GetDistanceToSiPM(const G4Step* step) {

    // Get the pre-step point
    const G4StepPoint* preStepPoint = step->GetPreStepPoint();
    // Get the global position of the pre-step point
    G4ThreeVector globalPos = preStepPoint->GetPosition();
    // Get the local position of the pre-step point in the current volume's coordinate system
    G4ThreeVector localPos = preStepPoint->GetTouchableHandle()->GetHistory()->GetTopTransform().TransformPoint(globalPos);

    // Get the logical volume of the current step
    G4LogicalVolume* currentVolume = preStepPoint->GetTouchableHandle()->GetVolume()->GetLogicalVolume();
    // Get the solid associated with the logical volume
    G4Tubs* solid = dynamic_cast<G4Tubs*>(currentVolume->GetSolid());
    // Get the dimensions of the solid (size of the volume)
    G4double size = solid->GetZHalfLength();

    G4double distance_to_sipm = size - localPos.z();
    return distance_to_sipm;

}

//Define AttenuateHelper() method
G4int DREMTubesSignalHelper::AttenuateHelper(const G4int& signal, const G4double& distance, const G4double& attenuation_length) {
    double probability_of_survival = exp(-distance/attenuation_length);

    G4int survived_photons = 0;
    for (int i=0; i<signal; i++)
    {
        // Simulate drawing between 0 and 1 with probability x of getting 1
        if (G4UniformRand() <= probability_of_survival) survived_photons++;
    }

    return survived_photons;

}

//Define AttenuateSSignal() method
//
G4int DREMTubesSignalHelper::AttenuateSSignal(const G4int& signal, const G4double& distance) {

    return AttenuateHelper(signal, distance, fSAttenuationLength);    

}

//Define AttenuateCSignal() method
//
G4int DREMTubesSignalHelper::AttenuateCSignal(const G4int& signal, const G4double& distance) {

    return AttenuateHelper(signal, distance, fCAttenuationLength);    
    
}



//**************************************************
