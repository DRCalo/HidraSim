// file: HidraTB25Ana.C

#include <TTree.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TF1.h>
#include <iostream>
#include <array>
#include <stdint.h>
#include <string>
#include <fstream>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <iomanip>
#include <cmath>
#include <limits>
#include <nlohmann/json.hpp>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "HidraGeo.h"
#include <map>
#include <TRandom3.h>
#include <cctype>

using json = nlohmann::json;

// Global Constants and parameters
//
// Minimum visible signal per channel (>=)
// Set to 0.0 or 1.0 to disable the cut.
const double MinVisiblePheS = 0.0; // photoelectrons
const double MinVisiblePheC = 0.0; // photoelectrons

// Scintillator on even rows (start from 0), Cerenkov on odd rows
const unsigned int grouping = 8;
// Write event display every N entries (0 to disable)
const unsigned int EventDisplayEvery = 100;
// A FERS is ON if at least 2 smeared channels across S or C (both are good) are above that FERS threshold.
const unsigned int FersMultiplicity = 2;

const double chi = 0.38;
const double sciPheGeV = 119.001; // tb24
const double cerPheGeV = 29.4;    // tb24
//const double sciPheGeV = 178.501;
//const double cerPheGeV = 43;

const double NofSipmCells_sci = 7772;
const double NofSipmCells_cer = 3443;
const double sci_pde = 0.22;
const double cer_pde = 0.38;

const double elcont = 1.005;
const double picont = 1.028;

// Separate fallback noise sigmas for S and C
const unsigned int NoiseRandomSeed = 12345; // fixed seed for reproducibility

// In this parametrised simulation, rawPhe is stored as "impinging optical photons"
// To account for multiple photons impinging on the same SiPM cell,
// we apply saturation correction to get "fired cells", which is the actual observable.
const bool ApplySaturation = true;
//
const bool ApplyTbNoise = true;
// decide whether to print per-event FERS channel counts (for debugging)
bool printFersLog = false;



// lambda = mean number of loss opportunities per channel.
// Probability to lose 1 pe = 1 - exp(-lambda).
const double SciOnePeLossLambda = 3; // set > 0 to enable
const double CerOnePeLossLambda = 3; // set > 0 to enable




double apply_sipm_saturation_from_pe(double pe,
                                     double phePerGeV,
                                     double nCells)
{
  if (pe <= 0.0 || phePerGeV <= 0.0 || nCells <= 0.0) {
    return 0.0;
  }

  const double firedCells = -nCells * std::expm1(-pe / nCells);
  return firedCells / phePerGeV;
}

std::map<unsigned int, double> fers_to_thr_map = {
    { 1, 0.17 },
    { 2, 0.20 },
    { 3, 0.30 },
    { 4, 0.28 },
    { 5, 0.105},
    { 6, 0.095},
    { 7, 0.08 },
    { 8, 0.08 },
    { 9, 0.05 },
    {10, 0.07 },
    {11, 0.05 },
    {12, 0.025},
    {13, 0.06 },
    {14, 0.05 },
    {15, 0.12 },
    {16, 0.06 }
};

using ChannelNoiseVector = std::vector<double>;

std::string Trim(const std::string& value)
{
  const std::size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return "";
  }

  const std::size_t last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

ChannelNoiseVector LoadNoiseValuesFromCsv(const std::string& csvPath)
{
  std::ifstream in(csvPath);
  if (!in) {
    throw std::runtime_error("Cannot open noise CSV: " + csvPath);
  }

  ChannelNoiseVector values;
  std::string line;

  while (std::getline(in, line)) {
    const std::string trimmed = Trim(line);
    if (trimmed.empty()) {
      continue;
    }

    const std::size_t commaPos = trimmed.find(',');
    const std::string token =
      Trim(trimmed.substr(0, commaPos == std::string::npos ? trimmed.size() : commaPos));

    values.push_back(std::stod(token));
  }

  if (values.empty()) {
    throw std::runtime_error("Noise CSV is empty: " + csvPath);
  }

  return values;
}

double GetChannelNoiseSigma(const ChannelNoiseVector& noiseValues, int ch)
{
  if (ch < 0 || static_cast<std::size_t>(ch) >= noiseValues.size()) {
    std::ostringstream os;
    os << "Missing per-channel noise sigma for channel " << ch
       << " (CSV size = " << noiseValues.size() << ")";
    throw std::runtime_error(os.str());
  }

  return noiseValues[static_cast<std::size_t>(ch)];
}

struct SipmMapEntry {
  int boardID = -1;
  std::string type;
  int row = -1;
  int column = -1;
  double x = 0.0;
  double y = 0.0;
  int fersId = -1;
  int ch = -1;
  std::string module_name;
  double x_local = 0.0;
  double y_local = 0.0;
};

struct FersChannelHit {
  uint64_t key = 0;
  double rawPhe = 0.0;
  double signal = 0.0;
};

struct EventDisplayHit {
  uint64_t key = 0;
  double x = 0.0;
  double y = 0.0;
  double rawPhe = 0.0;
  double signal = 0.0;
};

struct GaussianFitSummary {
  double mean = std::numeric_limits<double>::quiet_NaN();
  double meanErr = std::numeric_limits<double>::quiet_NaN();
  double rms = std::numeric_limits<double>::quiet_NaN();
  double rmsErr = std::numeric_limits<double>::quiet_NaN();
};

struct HistMomentSummary {
  double mean = std::numeric_limits<double>::quiet_NaN();
  double meanErr = std::numeric_limits<double>::quiet_NaN();
  double rms = std::numeric_limits<double>::quiet_NaN();
  double rmsErr = std::numeric_limits<double>::quiet_NaN();
};

using SipmLookup = std::unordered_map<std::string, SipmMapEntry>;
using FersKey = uint64_t;

struct ChannelKey {
  FersKey fersKey = 0;
  int ch = -1;

  bool operator==(const ChannelKey& other) const
  {
    return fersKey == other.fersKey && ch == other.ch;
  }
};

struct ChannelKeyHash {
  std::size_t operator()(const ChannelKey& key) const noexcept
  {
    const std::size_t h1 = std::hash<FersKey>{}(key.fersKey);
    const std::size_t h2 = std::hash<int>{}(key.ch);
    return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6U) + (h1 >> 2U));
  }
};

struct ChannelAccumulatedSignal {
  FersKey fersKey = 0;
  int fersId = -1;
  int ch = -1;
  double threshold = 0.0;
  double thresholdSignal = 0.0;
  double outputSignal = 0.0;
  double smearedThresholdSignal = 0.0;
  double smearedOutputSignal = 0.0;
};

using ChannelSignalMap =
  std::unordered_map<ChannelKey, ChannelAccumulatedSignal, ChannelKeyHash>;

void SmearAndCountChannelsOverThreshold(
  ChannelSignalMap& channelSignals,
  std::unordered_map<FersKey, unsigned int>& fersChannelsOverThreshold,
  TRandom3& rng,
  const ChannelNoiseVector& channelNoiseValues)
{
  for (auto& [channelKey, channel] : channelSignals) {
    const double sigma = GetChannelNoiseSigma(channelNoiseValues, channel.ch);

    double noise = 0.;
    if(ApplyTbNoise){
      noise = (sigma > 0.0) ? rng.Gaus(0.0, sigma) : 0.0;
    }


    channel.smearedThresholdSignal = channel.thresholdSignal + noise;
    channel.smearedOutputSignal = channel.outputSignal + noise;

    // FERS activation logic: if smeared threshold signal is above threshold,
    // count this channel for FERS multiplicity
    if (channel.smearedThresholdSignal > channel.threshold) {
      ++fersChannelsOverThreshold[channel.fersKey];
    }
  }
}








int board_id_from_fers_key(FersKey key)
{
  return static_cast<int>(static_cast<uint32_t>(key >> 32));
}

void PrintPerEventFersChannelCounts(
  unsigned int eventIndex,
  const ChannelSignalMap& sciChannelSignals,
  const ChannelSignalMap& cerChannelSignals,
  const std::unordered_map<FersKey, unsigned int>& fersChannelsOverThreshold)
{
  std::map<std::pair<int, int>, FersKey> fersSeen;

  auto collectFers = [&](const ChannelSignalMap& channelSignals) {
    for (const auto& [channelKey, channel] : channelSignals) {
      const int boardID = board_id_from_fers_key(channel.fersKey);
      fersSeen[{boardID, channel.fersId}] = channel.fersKey;
    }
  };

  collectFers(sciChannelSignals);
  collectFers(cerChannelSignals);

  if (printFersLog) {
    std::cout << "Event " << eventIndex << ":\n";
    for (const auto& [id, key] : fersSeen) {
      const int boardID = id.first;
      const int fersId = id.second;

      const auto it = fersChannelsOverThreshold.find(key);
      const unsigned int nChannelsOverThreshold =
        (it != fersChannelsOverThreshold.end()) ? it->second : 0U;

        std::cout
          << "  board " << boardID
          << ", FERS " << fersId
          << " -> channels above threshold = " << nChannelsOverThreshold;
      
      if (nChannelsOverThreshold >= FersMultiplicity) {
        std::cout << " [ON]";
      } else {
        std::cout << " [OFF]";
      }

      std::cout << '\n';
    }
  }
}

std::string make_sipm_key(const std::string& tower,
                          const std::string& type,
                          int row,
                          int column)
{
  std::ostringstream os;
  os << tower << '|' << type << '|' << row << '|' << column;
  return os.str();
}

FersKey make_fers_key(int boardID, int fersId)
{
  return (static_cast<FersKey>(static_cast<uint32_t>(boardID)) << 32) |
         static_cast<uint32_t>(fersId);
}

int grouped_column(int colID_original, unsigned int groupingValue)
{
  return static_cast<int>(colID_original / groupingValue);
}

int grouped_to_json_column(int groupedCol, unsigned int groupingValue)
{
  return groupedCol * static_cast<int>(groupingValue) + static_cast<int>(groupingValue) / 2;
}

double GetFersThreshold(int fersId)
{
  if (fersId < 0) {
    throw std::runtime_error("Invalid FERS id: " + std::to_string(fersId));
  }

  const auto it = fers_to_thr_map.find(static_cast<unsigned int>(fersId));
  if (it == fers_to_thr_map.end()) {
    throw std::runtime_error("Missing threshold for FERS id: " + std::to_string(fersId));
  }

  return it->second;
}

void AccumulateChannelSignal(ChannelSignalMap& channelSignals,
                             const SipmMapEntry& info,
                             double thresholdSignalContribution,
                             double outputSignalContribution)
{
  if (info.boardID < 0 || info.fersId < 0 || info.ch < 0) {
    return;
  }

  const FersKey fersKey = make_fers_key(info.boardID, info.fersId);
  const ChannelKey channelKey{fersKey, info.ch};

  auto [it, inserted] = channelSignals.emplace(
    channelKey,
    ChannelAccumulatedSignal{
      fersKey,
      info.fersId,
      info.ch,
      GetFersThreshold(info.fersId),
      0.0,
      0.0,
      0.0,
      0.0
    }
  );

  it->second.thresholdSignal += thresholdSignalContribution;
  it->second.outputSignal += outputSignalContribution;
}

bool IsActivatedFers(const std::unordered_map<FersKey, unsigned int>& fersChannelsOverThreshold,
                     FersKey key)
{
  const auto it = fersChannelsOverThreshold.find(key);
  return it != fersChannelsOverThreshold.end() &&
         it->second >= FersMultiplicity;
}

double SumActivatedSmearedChannelOutput(
  const ChannelSignalMap& channelSignals,
  const std::unordered_map<FersKey, unsigned int>& fersChannelsOverThreshold)
{
  double total = 0.0;

  for (const auto& [channelKey, channel] : channelSignals) {
    if (!IsActivatedFers(fersChannelsOverThreshold, channel.fersKey)) {
      continue;
    }

    total += channel.smearedOutputSignal;
  }

  return total;
}

unsigned int CountActivatedFers(
  const std::unordered_map<FersKey, unsigned int>& fersChannelsOverThreshold)
{
  unsigned int activated = 0;

  for (const auto& [fersKey, nChannels] : fersChannelsOverThreshold) {
    if (nChannels >= FersMultiplicity) {
      ++activated;
    }
  }

  return activated;
}



void ApplyPoissonOnePeLossToActivatedChannels(
  ChannelSignalMap& channelSignals,
  const std::unordered_map<FersKey, unsigned int>& fersChannelsOverThreshold,
  TRandom3& rng,
  double phePerGeV,
  double lossLambda)
{
  if (lossLambda <= 0.0 || phePerGeV <= 0.0) {
    return;
  }

  const double onePeInSignalUnits = 1.0 / phePerGeV;

  for (auto& [channelKey, channel] : channelSignals) {
    if (!IsActivatedFers(fersChannelsOverThreshold, channel.fersKey)) {
      continue;
    }

    const int nLosses = rng.Poisson(lossLambda);
    if (nLosses <= 0) {
      continue;
    }

    //channel.smearedOutputSignal -= onePeInSignalUnits;
    channel.smearedOutputSignal -= nLosses * onePeInSignalUnits;

    if (channel.smearedOutputSignal < 0.0) {
      channel.smearedOutputSignal = 0.0;
    }
  }
}




/**************************************************************************/
/************ Utility functions for SiPM map and event display ************/
SipmLookup LoadSipmMap(const std::string& jsonPath)
{
  std::ifstream in(jsonPath);
  if (!in) {
    throw std::runtime_error("Cannot open SiPM map JSON: " + jsonPath);
  }

  json j;
  in >> j;

  SipmLookup lookup;
  for (auto it = j.begin(); it != j.end(); ++it) {
    const json& node = it.value();

    SipmMapEntry entry;
    entry.boardID     = node.value("boardID", -1);
    entry.type        = node.value("type", "");
    entry.row         = node.value("row", -1);
    entry.column      = node.value("column", -1);
    entry.x           = node.value("x", 0.0);
    entry.y           = node.value("y", 0.0);
    entry.fersId      = node.value("fersId", -1);
    entry.ch          = node.value("ch", -1);
    entry.module_name = node.value("module_name", "");
    entry.x_local     = node.value("x_local", 0.0);
    entry.y_local     = node.value("y_local", 0.0);

    lookup[make_sipm_key(entry.module_name, entry.type, entry.row, entry.column)] = entry;
  }

  return lookup;
}

const SipmMapEntry* FindSipmInfo(const SipmLookup& lookup,
                                 const std::string& tower,
                                 const std::string& type,
                                 int row,
                                 int groupedCol,
                                 unsigned int /*groupingValue*/)
{
  const int jsonColumn = groupedCol;
  const auto key = make_sipm_key(tower, type, row, jsonColumn);
  const auto it = lookup.find(key);
  return (it == lookup.end()) ? nullptr : &it->second;
}

std::string make_event_display_name(const std::string& prefix, unsigned int entry)
{
  std::ostringstream os;
  os << prefix << "_evt" << std::setw(6) << std::setfill('0') << entry;
  return os.str();
}

TH2F* CreateEventDisplayHist(const std::string& name,
                             const std::string& title,
                             unsigned int groupingValue)
{
  const int xBins = static_cast<int>(NofSiPMTowersX * NofFiberscolumn / groupingValue);
  const int yBins = NofSiPMTowersY * NofFibersrow;

  return new TH2F(name.c_str(), title.c_str(),
                  xBins,
                  -NofSiPMTowersX * moduleX / 2.0, NofSiPMTowersX * moduleX / 2.0,
                  yBins,
                  -NofSiPMTowersY * moduleY / 2.0, NofSiPMTowersY * moduleY / 2.0);
}

void FillActivatedEventDisplay(TH2F* hist,
                               const std::vector<EventDisplayHit>& hits,
                               const std::unordered_map<FersKey, unsigned int>& fersChannelsOverThreshold,
                               double minVisiblePhe)
{
  if (!hist) {
    return;
  }

  for (const auto& hit : hits) {
    if (IsActivatedFers(fersChannelsOverThreshold, hit.key) &&
        hit.rawPhe >= minVisiblePhe) {
      hist->Fill(hit.x, hit.y, hit.signal);
    }
  }
}

void GetSiPMcoordinate(int TowID,
                       int rowID,
                       int colID_original,
                       double& SiPM_X,
                       double& SiPM_Y,
                       std::string fiber,
                       unsigned int groupingValue)
{
  double TowerOffsetY = -((NoModulesSiPM - 1) * moduleY) / 2 + TowID * moduleY;
  unsigned int channel = static_cast<unsigned int>(colID_original / groupingValue);
  double colID = (static_cast<double>(groupingValue) - 1) / 2 + channel * groupingValue;

  if (fiber == "S") {
    SiPM_X = +moduleX / 2 - tuberadius - (tuberadius * 2) * colID;
    SiPM_Y = -moduleY / 2 + tuberadius + (sq3 * tuberadius) * rowID + tuberadius * (2. * sq3m1 - 1.);
  }

  if (fiber == "C") {
    SiPM_X = +moduleX / 2 + tuberadius - (tuberadius * 2) * colID;
    SiPM_Y = -moduleY / 2 + tuberadius + (sq3 * tuberadius) * rowID + tuberadius * (2. * sq3m1 - 1.);
  }

  SiPM_Y = TowerOffsetY + SiPM_Y;
}




/***********************************************************************/
/************ Utility functions for analysis and output ************/
std::string simTower_to_tbTower(std::string simTower)
{
  return std::to_string(306 + std::stoi(simTower));
}

GaussianFitSummary FitGaussianSummary(TH1* hist)
{
  GaussianFitSummary out;
  if (!hist || hist->GetEntries() < 10) {
    return out;
  }

  hist->Fit("gaus", "Q0");
  TF1* fit = hist->GetFunction("gaus");
  if (!fit) {
    return out;
  }

  out.mean = fit->GetParameter(1);
  out.meanErr = fit->GetParError(1);
  out.rms = fit->GetParameter(2);
  out.rmsErr = fit->GetParError(2);
  return out;
}

HistMomentSummary GetHistMomentSummary(const TH1* hist)
{
  HistMomentSummary out;
  if (!hist || hist->GetEntries() < 1) {
    return out;
  }

  out.mean = hist->GetMean();
  out.meanErr = hist->GetMeanError();
  out.rms = hist->GetRMS();
  out.rmsErr = hist->GetRMSError();
  return out;
}

std::string CsvEscape(const std::string& value)
{
  std::string escaped = "\"";
  for (char c : value) {
    if (c == '"') {
      escaped += "\"\"";
    } else {
      escaped += c;
    }
  }
  escaped += "\"";
  return escaped;
}

void AppendSummaryCsvLocked(const std::string& csvPath,
                            const std::string& inputFile,
                            double truthEnergy,
                            int nentries,
                            const GaussianFitSummary& sFit,
                            const GaussianFitSummary& cFit,
                            const GaussianFitSummary& combFit,
                            const GaussianFitSummary& combChiFit,
                            const HistMomentSummary& sciX,
                            const HistMomentSummary& sciY,
                            const HistMomentSummary& cerX,
                            const HistMomentSummary& cerY,
                            const GaussianFitSummary& sFitFersOn,
                            const GaussianFitSummary& cFitFersOn)
{
  const int fd = open(csvPath.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
  if (fd == -1) {
    throw std::runtime_error("Cannot open summary CSV: " + csvPath);
  }

  if (flock(fd, LOCK_EX) != 0) {
    close(fd);
    throw std::runtime_error("Cannot lock summary CSV: " + csvPath);
  }

  struct stat st;
  if (fstat(fd, &st) != 0) {
    flock(fd, LOCK_UN);
    close(fd);
    throw std::runtime_error("Cannot stat summary CSV: " + csvPath);
  }

  std::ostringstream out;
  out << std::setprecision(17);

  if (st.st_size == 0) {
    out
      << "input_file,truth_energy,nentries,"
      << "s_fit_mean,s_fit_mean_err,s_fit_rms,s_fit_rms_err,"
      << "c_fit_mean,c_fit_mean_err,c_fit_rms,c_fit_rms_err,"
      << "comb_fit_mean,comb_fit_mean_err,comb_fit_rms,comb_fit_rms_err,"
      << "combchi_fit_mean,combchi_fit_mean_err,combchi_fit_rms,combchi_fit_rms_err,"
      << "res_sci_x_mean,res_sci_x_mean_err,res_sci_x_rms,res_sci_x_rms_err,"
      << "res_sci_y_mean,res_sci_y_mean_err,res_sci_y_rms,res_sci_y_rms_err,"
      << "res_cer_x_mean,res_cer_x_mean_err,res_cer_x_rms,res_cer_x_rms_err,"
      << "res_cer_y_mean,res_cer_y_mean_err,res_cer_y_rms,res_cer_y_rms_err,"
      << "s_fit_fers_on_mean,s_fit_fers_on_mean_err,s_fit_fers_on_rms,s_fit_fers_on_rms_err,"
      << "c_fit_fers_on_mean,c_fit_fers_on_mean_err,c_fit_fers_on_rms,c_fit_fers_on_rms_err\n";
  }

  out
    << CsvEscape(inputFile) << ','
    << truthEnergy << ','
    << nentries << ','
    << sFit.mean << ',' << sFit.meanErr << ',' << sFit.rms << ',' << sFit.rmsErr << ','
    << cFit.mean << ',' << cFit.meanErr << ',' << cFit.rms << ',' << cFit.rmsErr << ','
    << combFit.mean << ',' << combFit.meanErr << ',' << combFit.rms << ',' << combFit.rmsErr << ','
    << combChiFit.mean << ',' << combChiFit.meanErr << ',' << combChiFit.rms << ',' << combChiFit.rmsErr << ','
    << sciX.mean << ',' << sciX.meanErr << ',' << sciX.rms << ',' << sciX.rmsErr << ','
    << sciY.mean << ',' << sciY.meanErr << ',' << sciY.rms << ',' << sciY.rmsErr << ','
    << cerX.mean << ',' << cerX.meanErr << ',' << cerX.rms << ',' << cerX.rmsErr << ','
    << cerY.mean << ',' << cerY.meanErr << ',' << cerY.rms << ',' << cerY.rmsErr << ','
    << sFitFersOn.mean << ',' << sFitFersOn.meanErr << ',' << sFitFersOn.rms << ',' << sFitFersOn.rmsErr << ','
    << cFitFersOn.mean << ',' << cFitFersOn.meanErr << ',' << cFitFersOn.rms << ',' << cFitFersOn.rmsErr << '\n';

  const std::string text = out.str();
  const ssize_t written = write(fd, text.c_str(), text.size());
  if (written != static_cast<ssize_t>(text.size())) {
    flock(fd, LOCK_UN);
    close(fd);
    throw std::runtime_error("Cannot write summary CSV: " + csvPath);
  }

  flock(fd, LOCK_UN);
  close(fd);
}





/*********************************/
/***
 * Main analysis function
 * @param energy: beam energy in GeV (used for histogram ranges and output naming)
 * @param input: name of the input ROOT file (relative to ../build/)
 */
/*********************************/
void HidraTB25Ana(double energy, const std::string& input)
{
  const std::string sipmMapPath =
    "/home/apareti/HidraSim2025/TBDataPreparation/2025_SPS/MapAndCalibration/sipm_map.json";

  SipmLookup sipmLookup;
  try {
    sipmLookup = LoadSipmMap(sipmMapPath);
    std::cout << "Loaded " << sipmLookup.size() << " SiPM map entries from "
              << sipmMapPath << std::endl;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return;
  }

  const std::string infile = "../build/" + input;
  std::cout << "Using file: " << infile << std::endl;

  TFile* simfile = TFile::Open(infile.c_str(), "READ");
  if (!simfile || simfile->IsZombie()) {
    std::cerr << "Cannot open input file " << infile << std::endl;
    return;
  }

  TTree* simtree = static_cast<TTree*>(simfile->Get("DREMTubesout"));
  if (!simtree) {
    std::cerr << "Cannot find TTree DREMTubesout in " << infile << std::endl;
    simfile->Close();
    delete simfile;
    return;
  }

  std::ostringstream os;
  os << energy;
  const std::string enstr = os.str();
  const std::string outfile = "hidra" + enstr + ".root";
  TFile f(outfile.c_str(), "RECREATE");

  TDirectory* eventDisplayDir = f.mkdir("EventDisplays");
  f.cd();

  TRandom3 rng(NoiseRandomSeed);

  const std::string sciNoiseCsvPath = "S_sipm_std_run868_HG_4fers.csv";
  const std::string cerNoiseCsvPath = "C_sipm_std_run868_HG_4fers.csv";

  ChannelNoiseVector sciChannelNoiseValues;
  ChannelNoiseVector cerChannelNoiseValues;

  try {
    sciChannelNoiseValues = LoadNoiseValuesFromCsv(sciNoiseCsvPath);
    cerChannelNoiseValues = LoadNoiseValuesFromCsv(cerNoiseCsvPath);

    std::cout << "Loaded " << sciChannelNoiseValues.size()
              << " S-channel noise values from " << sciNoiseCsvPath << std::endl;
    std::cout << "Loaded " << cerChannelNoiseValues.size()
              << " C-channel noise values from " << cerNoiseCsvPath << std::endl;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return;
  }

  int modcol[NofModulesX * NofModulesY];
  int modrow[NofModulesX * NofModulesY];
  for (int i = 0; i < NofModulesX * NofModulesY; i++) {
    int row = i / NofModulesX;
    int col = i % NofModulesX;
    int imod = modflag[i];
    if (imod >= 0) {
      modcol[imod] = col;
      modrow[imod] = row;
    }
  }

  const double bmin = energy - 0.4 * std::sqrt(energy) * 10.;
  const double bmax = energy + 0.4 * std::sqrt(energy) * 10.;

  auto sciene = new TH1F("sciene", "sciene", 100, bmin, bmax);
  auto cerene = new TH1F("cerene", "cerene", 100, bmin, bmax);
  auto totene = new TH1F("totene", "totene", 100, bmin, bmax);
  auto totenec = new TH1F("totenec", "totenec", 100, bmin, bmax);
  auto totdep = new TH1F("totdep", "totdep", 100, 0., bmax);
  auto leakene = new TH1F("leakene", "leakene", 100, 0., 0.1);
  auto chidist = new TH1F("chidist", "chidist", 100, 0., 1.);
  auto mapcalo  = new TH2F("mapcalo", "mapcalo",
                           NofModulesX, 0., NofModulesX,
                           NofModulesY, 0., NofModulesY);
  auto SipmMapS = new TH2F("SipmMapS", "SipmS; Col; Row",
                           NofSiPMTowersX * NofFiberscolumn, 0, NofSiPMTowersX * NofFiberscolumn,
                           NofSiPMTowersY * NofFibersrow, 0, NofSiPMTowersY * NofFibersrow);
  auto SipmMapC = new TH2F("SipmMapC", "SipmC; Col; Row",
                           NofSiPMTowersX * NofFiberscolumn, 0, NofSiPMTowersX * NofFiberscolumn,
                           NofSiPMTowersY * NofFibersrow, 0, NofSiPMTowersY * NofFibersrow);

  auto SciSiPMCoordinates = new TH2F("SciSiPMCoordinates", "Sci SiPM Coordinates; X [mm]; Y[mm]",
                                     NofSiPMTowersX * NofFiberscolumn / grouping,
                                     -NofSiPMTowersX * moduleX / 2, NofSiPMTowersX * moduleX / 2,
                                     NofSiPMTowersY * NofFibersrow,
                                     -NofSiPMTowersY * moduleY / 2, NofSiPMTowersY * moduleY / 2);
  auto CerSiPMCoordinates = new TH2F("CerSiPMCoordinates", "Cer SiPM Coordinates; X [mm]; Y[mm]",
                                     NofSiPMTowersX * NofFiberscolumn / grouping,
                                     -NofSiPMTowersX * moduleX / 2, NofSiPMTowersX * moduleX / 2,
                                     NofSiPMTowersY * NofFibersrow,
                                     -NofSiPMTowersY * moduleY / 2, NofSiPMTowersY * moduleY / 2);

  auto ResidualHistSciX = new TH1F("ResidualHistSciX", "Residual Sci X; Residual [mm]; Entries",
                                   100, -10, 10);
  auto ResidualHistSciY = new TH1F("ResidualHistSciY", "Residual Sci Y; Residual [mm]; Entries",
                                   100, -10, 10);
  auto ResidualHistCerX = new TH1F("ResidualHistCerX", "Residual Cer X; Residual [mm]; Entries",
                                   100, -10, 10);
  auto ResidualHistCerY = new TH1F("ResidualHistCerY", "Residual Cer Y; Residual [mm]; Entries",
                                   100, -10, 10);

  auto SignalSfersID = new TH1F("SignalSfersID", "Signal S fibers FERS ID; FERS; Signal", 16, 0.5, 16.5);
  auto SignalCfersID = new TH1F("SignalCfersID", "Signal C fibers FERS ID; FERS; Signal", 16, 0.5, 16.5);

  auto scieneFersOn = new TH1F("scieneFersOn", "S energy for Activated FERS", 100, -50., bmax);
  auto cereneFersOn = new TH1F("cereneFersOn", "C energy for Activated FERS", 100, -50., bmax);

  const int nentries = simtree->GetEntries();
  std::cout << "Entries " << nentries << std::endl;

  int pdg; simtree->SetBranchAddress("PrimaryPDGID", &pdg);
  double venergy; simtree->SetBranchAddress("PrimaryParticleEnergy", &venergy);
  double lenergy; simtree->SetBranchAddress("EscapedEnergyl", &lenergy);
  double denergy; simtree->SetBranchAddress("EscapedEnergyd", &denergy);
  double edep; simtree->SetBranchAddress("EnergyTot", &edep);
  double Stot; simtree->SetBranchAddress("NofPMTScinDet", &Stot);
  double Ctot; simtree->SetBranchAddress("NofPMTCherDet", &Ctot);
  double PSdep; simtree->SetBranchAddress("PSEnergy", &PSdep);
  double beamX; simtree->SetBranchAddress("PrimaryX", &beamX);
  double beamY; simtree->SetBranchAddress("PrimaryY", &beamY);
  std::vector<double>* TowerE = nullptr; simtree->SetBranchAddress("VecTowerE", &TowerE);
  std::vector<double>* SPMT = nullptr; simtree->SetBranchAddress("VecSPMT", &SPMT);
  std::vector<double>* CPMT = nullptr; simtree->SetBranchAddress("VecCPMT", &CPMT);
  std::vector<double>* SSiPM = nullptr; simtree->SetBranchAddress("VectorSignals", &SSiPM);
  std::vector<double>* CSiPM = nullptr; simtree->SetBranchAddress("VectorSignalsCher", &CSiPM);

  for (unsigned int i = 0; i < static_cast<unsigned int>(nentries); i++) {
    simtree->GetEntry(i);

    const bool writeEventDisplay =
      (EventDisplayEvery > 0) && (((i + 1) % EventDisplayEvery) == 0);

    double ecalo = energy - lenergy / 1000.0;
    double totsci = 0.;
    double totcer = 0.;
    double tottow = 0.;

    double barX_sci = 0.;
    double barY_sci = 0.;
    double barX_cer = 0.;
    double barY_cer = 0.;

    std::unordered_map<FersKey, unsigned int> fersChannelsOverThreshold;
    std::vector<EventDisplayHit> eventDisplaySciHits;
    std::vector<EventDisplayHit> eventDisplayCerHits;

    ChannelSignalMap sciChannelSignals;
    ChannelSignalMap cerChannelSignals;

    sciChannelSignals.reserve(SSiPM->size());
    cerChannelSignals.reserve(CSiPM->size());

    if (writeEventDisplay) {
      eventDisplaySciHits.reserve(SSiPM->size());
      eventDisplayCerHits.reserve(CSiPM->size());
    }

    for (unsigned int j = 0; j < SPMT->size(); j++) {
      totsci += SPMT->at(j) / sciPheGeV;
      totcer += CPMT->at(j) / cerPheGeV;
      tottow += TowerE->at(j);
      mapcalo->Fill(modcol[j], modrow[j], TowerE->at(j) / 1000. / nentries);
    }

    double sciPosWeight = 0.0;
    double cerPosWeight = 0.0;


    // Start loop over SiPM signals - S channels
    for (unsigned int n = 0; n < SSiPM->size(); n++) {
      const double rawPhe = SSiPM->at(n);
      const double content = rawPhe / sciPheGeV;

      totsci += content;

      const unsigned int towID  = static_cast<unsigned int>(n / (NofFiberscolumn * NofFibersrow / 2));
      const unsigned int SiPMID = n % (NofFiberscolumn * NofFibersrow / 2);
      const unsigned int colID  = NofFiberscolumn - static_cast<unsigned int>(SiPMID / (NofFibersrow / 2));
      const unsigned int rowID  = 2 * static_cast<unsigned int>(SiPMID % (NofFibersrow / 2));

      double SiPM_X = 0.0;
      double SiPM_Y = 0.0;

      GetSiPMcoordinate(towID, rowID, colID, SiPM_X, SiPM_Y, "S", grouping); // Not used after JSON SiPM map implementation, kept for reference

      const std::string tbTower = simTower_to_tbTower(std::to_string(towID));
      const int groupedCol = grouped_column(static_cast<int>(colID), grouping);

      SipmMapS->Fill(colID, towID * NofFibersrow + rowID, content);

      const SipmMapEntry* info = FindSipmInfo(sipmLookup, tbTower, "S", static_cast<int>(rowID), groupedCol, grouping);

      if (info) {
        SciSiPMCoordinates->Fill(info->x, info->y, content);
        SignalSfersID->Fill(info->fersId, content / nentries);
        barX_sci += info->x * content;
        barY_sci += info->y * content;
        sciPosWeight += content;

        if (info->boardID >= 0 && info->fersId >= 0) {
          const FersKey key = make_fers_key(info->boardID, info->fersId);
          const double outputSignal = ApplySaturation ? apply_sipm_saturation_from_pe(rawPhe, sciPheGeV, NofSipmCells_sci) : content;

          AccumulateChannelSignal(
            sciChannelSignals,
            *info,
            content,
            outputSignal
          );

          if (writeEventDisplay) {
            eventDisplaySciHits.push_back({key, info->x, info->y, rawPhe, content});
          }
        }
      }
    } // end loop over S SiPM channels



    // Start loop over SiPM signals - C channels
    for (unsigned int n = 0; n < CSiPM->size(); n++) {
      const double rawPhe = CSiPM->at(n);
      const double content = rawPhe / cerPheGeV;

      totcer += content;

      const unsigned int towID  = static_cast<unsigned int>(n / (NofFiberscolumn * NofFibersrow / 2));
      const unsigned int SiPMID = n % (NofFiberscolumn * NofFibersrow / 2);
      const unsigned int colID  = NofFiberscolumn - static_cast<unsigned int>(SiPMID / (NofFibersrow / 2));
      const unsigned int rowID  = 2 * static_cast<unsigned int>(SiPMID % (NofFibersrow / 2)) + 1;

      SipmMapC->Fill(colID, towID * NofFibersrow + rowID, content); // Not used after JSON SiPM map implementation, kept for reference

      const std::string tbTower = simTower_to_tbTower(std::to_string(towID));
      const int groupedCol = grouped_column(static_cast<int>(colID), grouping);

      const SipmMapEntry* info =
        FindSipmInfo(sipmLookup, tbTower, "C", static_cast<int>(rowID), groupedCol, grouping);

      if (info) {
        CerSiPMCoordinates->Fill(info->x, info->y, content);
        SignalCfersID->Fill(info->fersId, content / nentries);
        barX_cer += info->x * content;
        barY_cer += info->y * content;
        cerPosWeight += content;

        if (info->boardID >= 0 && info->fersId >= 0) {
          const FersKey key = make_fers_key(info->boardID, info->fersId);
          const double outputSignal =
            ApplySaturation
              ? apply_sipm_saturation_from_pe(rawPhe, cerPheGeV, NofSipmCells_cer)
              : content;

          AccumulateChannelSignal(
            cerChannelSignals,
            *info,
            content,
            outputSignal
          );

          if (writeEventDisplay) {
            eventDisplayCerHits.push_back({key, info->x, info->y, rawPhe, content});
          }
        }
      }
    } // end loop over C SiPM channels

    SmearAndCountChannelsOverThreshold(
      sciChannelSignals,
      fersChannelsOverThreshold,
      rng,
      sciChannelNoiseValues
    );

    SmearAndCountChannelsOverThreshold(
      cerChannelSignals,
      fersChannelsOverThreshold,
      rng,
      cerChannelNoiseValues
    );



    // Apply 1-pe loss only after FERS activation is known.
    ApplyPoissonOnePeLossToActivatedChannels(
      sciChannelSignals,
      fersChannelsOverThreshold,
      rng,
      sciPheGeV,
      SciOnePeLossLambda
    );

    ApplyPoissonOnePeLossToActivatedChannels(
      cerChannelSignals,
      fersChannelsOverThreshold,
      rng,
      cerPheGeV,
      CerOnePeLossLambda
    );



    PrintPerEventFersChannelCounts(
      i,
      sciChannelSignals,
      cerChannelSignals,
      fersChannelsOverThreshold
    );

    const double totsciFersOn =
      SumActivatedSmearedChannelOutput(sciChannelSignals, fersChannelsOverThreshold);

    const double totcerFersOn =
      SumActivatedSmearedChannelOutput(cerChannelSignals, fersChannelsOverThreshold);

    if (sciPosWeight > 0.) {
      ResidualHistSciX->Fill((barX_sci / sciPosWeight) - beamX);
      ResidualHistSciY->Fill((barY_sci / sciPosWeight) - beamY);
    }

    if (cerPosWeight > 0.) {
      ResidualHistCerX->Fill((barX_cer / cerPosWeight) - beamX);
      ResidualHistCerY->Fill((barY_cer / cerPosWeight) - beamY);
    }

    if (writeEventDisplay && eventDisplayDir) {
      eventDisplayDir->cd();

      std::ostringstream sciTitle;
      sciTitle << "Sci event display, entry " << i
               << " (grouped channels, activated FERS only); X [mm]; Y [mm]";
      TH2F* sciEventDisplay = CreateEventDisplayHist(
        make_event_display_name("EventDisplaySci", i),
        sciTitle.str(),
        grouping
      );

      std::ostringstream cerTitle;
      cerTitle << "Cer event display, entry " << i
               << " (grouped channels, activated FERS only); X [mm]; Y [mm]";
      TH2F* cerEventDisplay = CreateEventDisplayHist(
        make_event_display_name("EventDisplayCer", i),
        cerTitle.str(),
        grouping
      );

      FillActivatedEventDisplay(
        sciEventDisplay,
        eventDisplaySciHits,
        fersChannelsOverThreshold,
        MinVisiblePheS
      );
      FillActivatedEventDisplay(
        cerEventDisplay,
        eventDisplayCerHits,
        fersChannelsOverThreshold,
        MinVisiblePheC
      );

      sciEventDisplay->Write();
      cerEventDisplay->Write();

      delete sciEventDisplay;
      delete cerEventDisplay;

      f.cd();
    }

    scieneFersOn->Fill(totsciFersOn);
    cereneFersOn->Fill(totcerFersOn);

    sciene->Fill(totsci);
    cerene->Fill(totcer);
    totene->Fill(0.5 * (totsci + totcer));
    totenec->Fill(picont * (totsci - chi * totcer) / (1 - chi));
    totdep->Fill(tottow / 1000.);
    leakene->Fill(lenergy / 1000. / energy);
    chidist->Fill((totsci - ecalo) / (totcer - ecalo));
  }

  const GaussianFitSummary sFit = FitGaussianSummary(sciene);
  const GaussianFitSummary cFit = FitGaussianSummary(cerene);
  const GaussianFitSummary combFit = FitGaussianSummary(totene);
  const GaussianFitSummary combChiFit = FitGaussianSummary(totenec);

  const HistMomentSummary resSciX = GetHistMomentSummary(ResidualHistSciX);
  const HistMomentSummary resSciY = GetHistMomentSummary(ResidualHistSciY);
  const HistMomentSummary resCerX = GetHistMomentSummary(ResidualHistCerX);
  const HistMomentSummary resCerY = GetHistMomentSummary(ResidualHistCerY);

  const GaussianFitSummary sFitFersOn = FitGaussianSummary(scieneFersOn);
  const GaussianFitSummary cFitFersOn = FitGaussianSummary(cereneFersOn);

  try {
    AppendSummaryCsvLocked("hidra_summary.csv",
                           input,
                           energy,
                           nentries,
                           sFit,
                           cFit,
                           combFit,
                           combChiFit,
                           resSciX,
                           resSciY,
                           resCerX,
                           resCerY,
                           sFitFersOn,
                           cFitFersOn);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  f.Write();
  f.Close();

  simfile->Close();
  delete simfile;
}