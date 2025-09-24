//**************************************************
// \file DREMTubesSignalHelper.hh
// \brief: Definition of DREMTubesSignalHelper class
// \author: Lorenzo Pezzotti (CERN EP-SFT-sim)
//          @lopezzot
// \start date: 1 September 2021
//**************************************************

//Prevent including header multiple times
//
#ifndef DREMTubesSignalHelper_h
#define DREMTubesSignalHelper_h

//Includers from Geant4
//
#include "globals.hh"
#include "G4Step.hh"

class DREMTubesSignalHelper {

    private:

        static DREMTubesSignalHelper* instance;

		const G4double fk_B = 0.126; //Birks constant
		const G4double fSAttenuationLength = 367.0*CLHEP::cm; // from TB24 data
		const G4double fCAttenuationLength = 380.9*CLHEP::cm; // from TB24 data
		//const G4double fSAttenuationLength = 191.6*CLHEP::cm; // from TB23 data
		//const G4double fCAttenuationLength = 388.9*CLHEP::cm; // from TB23 data


	//Private constructor (singleton)
        //
	DREMTubesSignalHelper();

    public:

    	static DREMTubesSignalHelper* Instance();

    	G4double ApplyBirks( const G4double& de, const G4double& steplength );

		G4int SmearSSignalPMT( const G4double& de );

    	G4int SmearCSignalPMT( );

		G4int SmearSSignalSiPM( const G4double& de );

    	G4int SmearCSignalSiPM( );


    	G4double GetDistanceToSiPM(const G4Step* step);

		G4int AttenuateHelper(const G4int& signal, const G4double& distance, const G4double& attenuation_length);

		G4int AttenuateSSignal(const G4int& signal, const G4double& distance);

		G4int AttenuateCSignal(const G4int& signal, const G4double& distance);

};

#endif

//**************************************************
