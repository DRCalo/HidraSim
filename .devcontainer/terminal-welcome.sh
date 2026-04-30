#!/usr/bin/env bash

# Colors
RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
CYAN='\033[1;36m'
NC='\033[0m' # No Color

clear

cat << "EOF"

░██     ░██ ░██       ░██                                        ░██                
░██     ░██           ░██                                                           
░██     ░██ ░██ ░████████ ░██░████  ░██████            ░███████  ░██░█████████████  
░██████████ ░██░██    ░██ ░███           ░██  ░██████ ░██        ░██░██   ░██   ░██ 
░██     ░██ ░██░██    ░██ ░██       ░███████           ░███████  ░██░██   ░██   ░██ 
░██     ░██ ░██░██   ░███ ░██      ░██   ░██                 ░██ ░██░██   ░██   ░██ 
░██     ░██ ░██ ░█████░██ ░██       ░█████░██          ░███████  ░██░██   ░██   ░██ 
                                                                                    
                                                                                    
                                                                                    

EOF




if [ ! -d "/opt/geant4/data" ] || [ -z "$(ls -A /opt/geant4/data 2>/dev/null)" ]; then
	DATASETS_STATUS="${RED}⚠️  /opt/geant4/data empty or not mounted!${NC}"
else
	DATASETS_STATUS="${GREEN}✅  /opt/geant4/data${NC}"
fi

G4VER="$(source /opt/geant4/bin/geant4.sh >/dev/null 2>&1 && geant4-config --version 2>/dev/null || echo 'not sourced')"


printf "${CYAN}🧪  Welcome to the HidraSim Geant4 Dev Container!  🧪${NC}\n"
printf "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"
printf "${CYAN}📦  Project repo:${NC}   https://github.com/lopezzot/DREMTubes\n"
printf "${CYAN}📄  Docs:${NC}           README.md in /workspace\n"
printf "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"
printf "${CYAN}🔬  Geant4 version:${NC} ${GREEN}$G4VER${NC}\n"
printf "${CYAN}🗄️   Datasets mount:${NC} $DATASETS_STATUS\n"
printf "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"
printf "${GREEN}⚡  Reminder:${NC} Before building or running, type:\n"
printf "   ${BLUE}source /opt/geant4/bin/geant4.sh${NC}\n\n"
printf "${CYAN}🔧  Build commands:${NC}\n"
printf "   ${GREEN}mkdir /workspace/build${NC}\n   ${GREEN}cd /workspace/build${NC}\n   ${GREEN}cmake ..${NC}\n   ${GREEN}make -j\$(nproc)${NC}\n\n"
printf "${CYAN}🚀  Run commands:${NC}\n"
printf "   ${GREEN}cd /workspace/build${NC}\n   ${GREEN}./DREMTubes -m DREMTubes_run.mac -t 2 -pl FTFP_BERT${NC}\n\n"

printf "${GREEN}🎉  Happy simulating!${NC} 🚀\n\n"
