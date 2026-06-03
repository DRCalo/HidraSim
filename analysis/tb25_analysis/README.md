# TB25 Analysis

This directory contains the TB25-specific ROOT analysis macro, helper scripts,
and CSV mapping inputs.

Main files:

- `HidraTB25Ana.C`: TB25 analysis macro.
- `run_ana.sh`: helper script to run the macro over the configured
  energy/input-file list.
- `plot_hidra.py`: plotting helper for the TB25 summary outputs.
- `*_sipm_std_run868_HG_4fers.csv`: TB25 SiPM CSV mapping files.

## Workflow

The TB25 workflow is:

1. Open the input HidraSim ROOT file and read the simulated PMT and SiPM signal
   vectors.
2. Load the SiPM hardware/channel map, high-gain pedestal/noise values, and
   ADC-to-GeV calibration constants from JSON files.
3. Convert simulation SiPM indices to the TB25 tower, row, and grouped-column
   convention used by the FERS map.
4. Associate each simulated SiPM signal to a mapped FERS channel.
5. Optionally apply SiPM saturation, pedestal terms, and electronics noise.
6. Count channels above threshold and mark a FERS as active when the configured
   multiplicity requirement is satisfied.
7. Fill both all-signal histograms and FERS-activated histograms, then append
   fit and resolution summaries to `hidra_summary.csv`.

## Configuration

The main behavior switches are defined near the top of `HidraTB25Ana.C`:

- `ApplySaturation`: apply or skip SiPM finite-cell saturation.
- `ApplyTbNoise`: include or exclude the TB pedestal/noise smearing.
- `NoiseMode`: choose `UncorrelatedByChannel` or `CorrelatedWithinFers`.
- `NoiseDistribution`: choose `Gaussian` or `LogNormal` noise sampling.
- `AddPedestalToEnergyContribution`: add the pedestal contribution to the
  channel signal before threshold/output evaluation.
- `ApplyPedestalSubtraction`: subtract the pedestal median from activated
  channels.
- `ScalePedestalSubtractionFactorS` and `ScalePedestalSubtractionFactorC`:
  scale the scintillation and Cherenkov pedestal subtraction.
- `FersThresholdScaleFactor`: scale the FERS activation thresholds.
- `FersMultiplicity`: number of channels above threshold required to activate a
  FERS.
- `EventDisplayEvery`: event-display writing interval; set to `0` to disable.
- `printSmearingLog` and `printFersLog`: enable detailed debug prints.

## Inputs

The macro includes the shared detector geometry from the parent analysis
directory:

```cpp
#include "../HidraGeo.h"
```

It reads JSON mapping and calibration inputs from:

```text
../../../TBDataPreparation/2025_SPS/MapAndCalibration/
```

when run from `HidraSim/analysis/tb25_analysis/`. These JSON files are kept with
the TB data-preparation inputs rather than duplicated here.

The local CSV files in this directory are TB25-specific SiPM mapping inputs.

## Usage

Typical usage:

```bash
cd HidraSim/analysis/tb25_analysis
./run_ana.sh
```

The script writes `hidra_summary.csv`, per-energy ROOT files, and run logs in
this directory. These are generated outputs and should normally not be committed.
