# file: run_hidra_parallel.sh
#!/usr/bin/env bash
set -euo pipefail

energies=(10 20 30 40 60 80 100 120)
files=(
  "OutputElectrons_26_04_08/DREMTubesout_Run0.root"
  "OutputElectrons_26_04_08/DREMTubesout_Run1.root"
  "OutputElectrons_26_04_08/DREMTubesout_Run2.root"
  "OutputElectrons_26_04_08/DREMTubesout_Run3.root"
  "OutputElectrons_26_04_08/DREMTubesout_Run4.root"
  "OutputElectrons_26_04_08/DREMTubesout_Run5.root"
  "OutputElectrons_26_04_08/DREMTubesout_Run6.root"
  "OutputElectrons_26_04_08/DREMTubesout_Run7.root"
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