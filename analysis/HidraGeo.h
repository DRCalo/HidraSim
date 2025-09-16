/*    const int NofmodulesX = 24;
    const int NofmodulesY = 5;
    const int modflag[120]={-1,-1,-1,-1,-1,-1,-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,-1,
                       -1,-1,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,-1,-1,
                       30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,
                       -1,-1,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,-1,-1,
                       -1,-1,-1,-1,-1,-1,-1,74,75,76,77,78,79,80,81,82,83,-1,-1,-1,-1,-1,-1,-1};*/





// 70 modules -> TB25
const int NofModulesX = 5;
const int NofModulesY = 18;
// 80 miniM -> But different geometry-> move one minimodules from row to corner, horizontal placement
const int modflag[90]={-1, -1, 0, -1, -1,
                         -1,  1, 2, 3, -1, 
                         -1,  4, 5,  6, -1, 
                         -1,  7, 8,  9, -1,
                         10,  1, 12, 13, 14, 
                         15,  16, 17, 18, 19, 
                         20,  21, 22, 23, 24,
                         25,  26, 27, 28, 29,
                         30,  31, 32, 33, 34, 
                         35,  36, 37, 38, 39, 
                         40,  41, 42, 43, 44, 
                         45,  46, 47, 48, 49, 
                         50,  51, 52, 53, 54,
                         55,  56, 57, 58, 59, 
                         -1,  60, 61, 62, -1, 
                         -1,  63, 64, 65, -1,
                         -1,  66, 67, 68, -1,
                         -1, -1,  69, -1, -1};  
const int NoModulesSiPM=8;
const int SiPMMod[8]={17,22,27,32,37,42,47,52};
const int NofSiPMTowersX = 1;
const int NofSiPMTowersY = 8;
const int NofFiberscolumn = 64;
const int NofFibersrow = 16;
const int NoModulesActive=80;
const double moduleZ = (2500.);
const bool irot=false;
const int NoFibersTower=NofFiberscolumn*NofFibersrow/2;


