#!/usr/bin/env bash
# file: run_ana.sh
set -euo pipefail

energies=(10 20 30 40 60 80 100 120)
files=(
  "../../build/OutputElectrons_26_04_10/DREMTubesout_Run0.root"
  "../../build/OutputElectrons_26_04_10/DREMTubesout_Run1.root"
  "../../build/OutputElectrons_26_04_10/DREMTubesout_Run2.root"
  "../../build/OutputElectrons_26_04_10/DREMTubesout_Run3.root"
  "../../build/OutputElectrons_26_04_10/DREMTubesout_Run4.root"
  "../../build/OutputElectrons_26_04_10/DREMTubesout_Run5.root"
  "../../build/OutputElectrons_26_04_10/DREMTubesout_Run6.root"
  "../../build/OutputElectrons_26_04_10/DREMTubesout_Run7.root"
)

summary_csv="hidra_summary.csv"
pids=()

rm -f "${summary_csv}"

for i in "${!energies[@]}"; do
  energy="${energies[$i]}"
  input_file="${files[$i]}"
  log_file="run_${i}_${energy}GeV.log"

  echo "Starting ${input_file} at ${energy} GeV"
  root -l -b -q "HidraTB25Ana.C(${energy}, \"${input_file}\")" >"${log_file}" 2>&1 &
  pids+=("$!")
done

status=0

for pid in "${pids[@]}"; do
  if ! wait "${pid}"; then
    status=1
  fi
done

echo "Done. Shared summary written to: ${summary_csv}"
exit "${status}"







#  "DREMTubesout_Run0.root"
#  "DREMTubesout_Run1.root"
#  "DREMTubesout_Run2.root"
#  "DREMTubesout_Run3.root"
#  "DREMTubesout_Run4.root"
#  "DREMTubesout_Run5.root"
#  "DREMTubesout_Run6.root"
#  "DREMTubesout_Run7.root"
