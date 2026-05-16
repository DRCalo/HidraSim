# HiDRa Analysis

This directory contains the shared geometry header and the standard ROOT macro
used to analyze HidraSim simulation ntuples.

## Standard Simulation Analysis

`HidraAna.C` is the general analysis macro for HidraSim simulation ntuples.
It reads the simulation output tree, reconstructs the calorimeter response, and
produces ROOT histograms for the requested beam energy.

The workflow is:

1. Open the input ROOT file and read the `DREMTubes` simulation tree.
2. Load the detector geometry from `HidraGeo.h`.
3. Loop over events and accumulate PMT tower signals and SiPM fiber signals.
4. Convert scintillation and Cherenkov SiPM photoelectron yields to energy.
5. Reconstruct SiPM channel positions from the simulation vector index.
6. Fill calorimeter maps, SiPM coordinate maps, energy histograms, and optional
   event-display histograms.

Inputs:

- Beam energy, passed as the first macro argument.
- HidraSim ROOT ntuple, passed as the second macro argument.
- Geometry constants from `HidraGeo.h`.

Outputs:

- A ROOT histogram file named from the input energy, for example `hidra10.root`.
- Energy-response histograms for scintillation, Cherenkov, and combined signals.
- Calorimeter tower maps and SiPM coordinate maps.
- Optional event-display histograms written at the configured event interval.

Typical usage from this directory:

```bash
root -l -b -q 'HidraAna.C(energy, "input.root")'
```

The macro uses `HidraGeo.h` for the detector geometry constants. SiPM channels
are decoded from the simulation vector index using the same convention used when
the SiPM copy numbers are assigned in the Geant4 geometry. The resulting SiPM
coordinates are filled in the simulation coordinate system.

Generated ROOT files, logs, summaries, compiled ROOT dictionaries, and plots are
analysis outputs and should normally not be committed.

## TB25 Analysis

TB25-specific analysis files live in `tb25_analysis/`. See
`tb25_analysis/README.md` for the TB25 workflow, mapping inputs, and configurable
analysis switches.
