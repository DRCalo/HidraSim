#!/bin/bash
set -a
source "$(dirname "$0")/.env"
set +a

: "${GEANT4_DATASETS_HOST_PATH:?GEANT4_DATASETS_HOST_PATH not defined in .env}"

mkdir -p "${GEANT4_DATASETS_HOST_PATH}"