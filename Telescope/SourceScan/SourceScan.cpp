// Program for recording source data in the lab,
// based on the Timepix3 telescope DAQ
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <iomanip>

#include <dirent.h>

#include "TApplication.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TFile.h"
 
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "tpx3defs.h"

using namespace std;

struct Device {
  bool enabled;
  SpidrDaq* daq;
  int daq_packets_rec[2];
  int daq_packets_lost[2];
  std::string id;
  std::string dacfile;
  std::string trimfile;
  std::string datafile;
};

// Convenience macros
#define cout_spidr_err(str) \
  cout << str << ": " << spidrctrl->errorString() << endl
#define cout_spidrdev_err(dev, str)                                       \
  cout << "Dev " << dev << " " << str << ": " << spidrctrl->errorString() \
       << endl

bool configure(SpidrController *spidrctrl,
               std::vector<Device>& devices);
bool start_run(SpidrController *spidrctrl, 
               std::vector<Device>& devices,
               const int& run_nr, const std::string& description);
bool stop_run(SpidrController *spidrctrl, 
              std::vector<Device>& devices);
bool timestamp(SpidrController *spidrctrl,
               const std::vector<Device>& devices);
bool devId_tostring(int devId, char *devIdstring);
void get_temperature(SpidrController *spidrctrl,
                     std::vector<Device>& devices);
bool applyEthernetMask(SpidrController* spidrctrl, 
                       const int dev, const int mask);
bool analyse(const std::string& directory);
void setupStyle();

void cout_daqdev_err(const int dev, const std::string& str, SpidrDaq* daq) {
  cout << "Dev " << dev << " " << str << ": " << daq->errorString() << endl;
}

// Pixel packets sent before and after run
int spidr_data_packets_sent[2] = {0, 0};        


int main(int argc, char *argv[]) {

  const std::string cfg_path = "/home/timepix3/CONFIG";
  const std::string data_path = "/data/LABDATA";

  // Get the run name (name of the directory for storing the data files).
  if (argc != 3) {
    cout << "usage: SourceScan <run name> <seconds>\n";
    return -1;
  }
  const std::string run_name = argv[1];
  int time = atoi(argv[2]);
  string name = data_path+"/"+run_name+"/"+run_name+".root";
  
  TApplication app("app", &argc, argv);
  app.SetReturnFromRun(true);
 
  // Open a control connection to SPIDR - TPX3 module
  // with address 192.168.100.10, default port number 50000
  SpidrController *spidrctrl = new SpidrController(192, 168, 100, 10);

  // Are we connected to the SPIDR - TPX3 module ?
  if (!spidrctrl->isConnected()) {
    cout << spidrctrl->ipAddressString() << ": "
         << spidrctrl->connectionStateString() << ", "
         << spidrctrl->connectionErrString() << endl;
    return 1;
  }

  // Get the IP address.
  int addr;
  if (!spidrctrl->getIpAddrDest(0, &addr)) cout_spidr_err("###getIpAddrDest");

  // Select internal or external clock
  if (!spidrctrl->setExtRefClk(false)) cout_spidr_err("###setExtRefClk");
 
  /* 
  */ 

  // Determine number of devices, does not check if devices are active
  int nMaxDevices = 0;
  if (!spidrctrl->getDeviceCount(&nMaxDevices)) {
    cout_spidr_err("###getDeviceCount");
  } 
  cout << "[Note] Number of devices supported by firmware: " 
       << nMaxDevices << endl;

  const unsigned int nDevices = 3;
  std::vector<Device> devices(nDevices);
  for (unsigned int i = 0; i < nDevices; ++i) {
    // spidrctrl->setReadoutSpeed(i, 160);
    if (!spidrctrl->setOutputMask(i, 0x3)) {
      cout_spidr_err("###setOutputMask()");
    }
    // Check link status
    int linkstatus;
    if (!spidrctrl->getLinkStatus(i, &linkstatus)) {
      cout_spidr_err("###getLinkStatus()");
    }
    // Link status: bits 0-7: 0=link enabled; bits 16-23: 1=link locked
    const int links_enabled_mask = (~linkstatus) & 0xFF;
    const int links_locked_mask = (linkstatus & 0xFF0000) >> 16;
    std::cout << "[Note] Link status\n";
    std::cout << "  Link  Enabled  Locked\n";
    for (unsigned int j = 0; j < 8; ++j) {
      const int enabled = (links_enabled_mask >> j) & 0x1;
      const int locked = (links_locked_mask >> j) & 0x1;
      std::cout << "    " << j << "      " << enabled
                << "       " << locked << "\n";
    }
    if (links_enabled_mask != 0 && links_locked_mask == links_enabled_mask) {
      // At least one link is enabled, and all links enabled are locked
      cout << "[Note] Device " << i << ": linkstatus = " 
           << hex << linkstatus << dec << endl;
 
     devices[i].enabled = true;

      // Get device IDs in readable form
      int devIds[16];
      if (!spidrctrl->getDeviceIds(devIds)) cout_spidr_err("###getDeviceIds()");
      char devIdstring[16];
      devId_tostring(devIds[i], devIdstring);
      cout << "[Note] Device " << i << ": " << devIdstring << endl;
      devices[i].id = devIdstring;
    } else {
      cout << "###linkstatus: " << hex << linkstatus << dec << endl;
      devices[i].enabled = false;
      continue;
    }

    int errstat;
    if (!spidrctrl->reset(&errstat)) {
      cout << "###reset errorstat: " << hex << errstat << dec << endl;
    }
    // Set packet filter: ethernet no filter
    applyEthernetMask(spidrctrl, i, 0xFFFF);
  }
  // devices[1].enabled = false;

  // Select whether or not to let the FPGA do the ToT/ToA decoding
  // gray decoding (for ToA only) has priority over LFSR decoding
  if (!spidrctrl->setDecodersEna(true)) cout_spidr_err("###setDecodersEna");

  for (unsigned int i = 0; i < nDevices; ++i) {
    if (!devices[i].enabled) continue;
    // Interface to Timepix3 pixel data acquisition
    devices[i].daq = new SpidrDaq(spidrctrl, 0x10000000, i);
    const std::string errstr = devices[i].daq->errorString();
    if (!errstr.empty())
      cout << "Dev " << i << " ### SpidrDaq: " << errstr << endl;
    // Don't flush when no file is open
    devices[i].daq->setFlush(false);
    // No sampling
    devices[i].daq->setSampling(false);
  }
  // SPIDR-TPX3 and Timepix3 timers
  if (!spidrctrl->restartTimers()) cout_spidr_err("###restartTimers");

  get_temperature(spidrctrl, devices);

  for (unsigned int i = 0; i < nDevices; ++i) {
    if (!devices[i].enabled) continue;
    const std::string trimfile = devices[i].id + "_trimdacs.txt";
    const std::string dacfile = devices[i].id + "_dacs.txt";
    devices[i].trimfile = cfg_path + "/" + trimfile;
    devices[i].dacfile = cfg_path + "/" + dacfile;
    const std::string datafile = devices[i].id + ".dat";
    devices[i].datafile = data_path + "/" + run_name + "/" + datafile;
  }
  bool status = configure(spidrctrl, devices);
  get_temperature(spidrctrl, devices);

  const std::string description = "Source scan";
  // Open the data files.
  start_run(spidrctrl, devices, 0, description);
  // Open the shutter.
  if (!spidrctrl->openShutter()) {
    cout_spidr_err("###openShutter");
    status = false;
  }
  const int maxtime = time;     //TEMPO 
  for (int j = 0; j < maxtime; ++j) {
    sleep(1);
    timestamp(spidrctrl, devices);
    cout << "Second " << j << " of " << maxtime << endl;
  }
  // Close the shutter.
  if (!spidrctrl->closeShutter()) {
    cout_spidr_err("###closeShutter");
    status = false;
  }
  stop_run(spidrctrl, devices);
  get_temperature(spidrctrl, devices);
  // Clean up.
  for (unsigned int i = 0; i < nDevices; ++i) {
    if (devices[i].enabled) devices[i].daq->stop();
    delete devices[i].daq;
  }
  delete spidrctrl;
  TFile *rootfile = new TFile(name.c_str(),"RECREATE");

  if (!analyse(data_path + "/" + run_name)) {
    status = false;
  } else {
    app.Run(kTRUE);
  }
  if (status) {
    cout << "[Note] Finished OK." << endl;
  }
  rootfile->Close();
  return 0;
}

//=====================================================
bool configure(SpidrController *spidrctrl,
               std::vector<Device>& devices) { 
//=====================================================

  bool status = true;
  const unsigned int nDevices = devices.size();
  for (unsigned int i = 0; i < nDevices; ++i) {
    if (!devices[i].enabled) continue;
    // Read the TPX3 DACs configuration.
    FILE *fp = fopen(devices[i].dacfile.c_str(), "r");
    if (fp == NULL) {
      cout << "[Warning] Cannot open DAC file " << devices[i].dacfile << endl;
      status = false;
      continue;
    }
    cout << "[Note] Reading " << devices[i].dacfile << endl;
    while (!feof(fp)) {
      char line[1024];
      if (fgets(line, 256, fp) == NULL) continue;
      if (line[0] == '#') continue;
      int dac_nr, dac_val;
      if (sscanf(line, "%d %d", &dac_nr, &dac_val) == 2) {
        if (!spidrctrl->setDac(i, dac_nr, dac_val)) {
          cout_spidrdev_err(i, "###setDac");
          status = false;
        }
      }
    }
    fclose(fp);

    // Clear pixel configuration in chip
    if (!spidrctrl->resetPixels(i)) {
      cout_spidrdev_err(i, "###resetPixels");
      status = false;
    }

    // Clear local pixel config (testpulse disabled, not masked, threshold = 0).
    spidrctrl->resetPixelConfig();

    // Read thresholds from file
    fp = fopen(devices[i].trimfile.c_str(), "r");
    if (fp == NULL) {
      cout << "[Warning] Cannot open pixel config file " 
           << devices[i].trimfile << endl;
      status = false;
      continue;
    }
    cout << "[Note] Reading " << devices[i].trimfile << endl;
    while (!feof(fp)) {
      char line[1024];
      if (fgets(line, 256, fp) == NULL) continue;
      if (line[0] == '#') continue; 
      int row, col, thr, mask, tp_ena;
      sscanf(line, "%d %d %d %d %d", &col, &row, &thr, &mask, &tp_ena);
      if (!spidrctrl->setPixelThreshold(col, row, thr)) {
        cout_spidrdev_err(i, "###setPixelThreshold");
        status = false;
      }
      if (mask) {
        if (!spidrctrl->setPixelMask(col, row, true)) {
          cout_spidrdev_err(i, "###setPixelMask");
          status = false;
        }
      }
    }
    fclose(fp);

    // Write pixel config to chip
    if (!spidrctrl->setPixelConfig(i)) {
      if (!strstr(spidrctrl->errorString().c_str(), "ERR_UNEXP")) {
        cout_spidrdev_err(i, "###setPixelConfig");
        status = false;
      }
    }

    // Read back configuration
    int gen_cfg;
    if (!spidrctrl->getGenConfig(i, &gen_cfg)) {
      cout_spidrdev_err(i, "###getGenConfig");
      status = false;
    }
  }

  // ----------------------------------------------------------
  // Trigger configuration, parameter not relevant if shutter is opened/close
  // 'manually'
  // ----------------------------------------------------------

  // Configure the shutter trigger
  int trig_mode = 0;           // external shutter
  int trig_length_us = 10000;  // us
  int trig_freq_hz = 5;        // Hz
  int nr_of_trigs = 1;         // 1 triggers

  if (!spidrctrl->setShutterTriggerConfig(trig_mode, trig_length_us,
                                          trig_freq_hz, nr_of_trigs)) {
    cout_spidr_err("###setShutterTriggerConfig");
    status = false;
  }

  // Read back the configuration.
  int i1, i2, i3, i4;
  if (!spidrctrl->getShutterTriggerConfig(&i1, &i2, &i3, &i4)) {
    cout_spidr_err("###getShutterTriggerConfig");
    status = false;
  }
  cout << "[Note] Read trigger config " << hex << i1 << " " << i2 << " " << i3
       << " " << i4 << dec << endl;

  // const int cfg = TPX3_POLARITY_HPLUS | 
  const int cfg = TPX3_POLARITY_EMIN | 
                  TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA |
                  TPX3_TESTPULSE_ENA | TPX3_SELECTTP_EXT_INT |
                  TPX3_SELECTTP_DIGITAL | TPX3_FASTLO_ENA;
  for (unsigned int i = 0; i < nDevices; ++i) {
    if (!devices[i].enabled) continue; 
    // Set Timepix3 acquisition mode
    if (!spidrctrl->setGenConfig(i, cfg)) {
      cout_spidrdev_err(i, "###setGenCfg");
      status = false;
    }
    int gen_cfg;
    // Read back configuration
    if (!spidrctrl->getGenConfig(i, &gen_cfg)) {
      cout_spidrdev_err(i, "###getGenConfig");
      status = false;
    }
    cout << "[Note] Device " << i << ": gen config = " << hex << gen_cfg << dec
         << endl;

    // PLL configuration: 40 MHz on pixel matrix
    int pll_cfg = 0x01E | 0x100;  // 40 MHz, 16 clock phases
    if (!spidrctrl->setPllConfig(i, pll_cfg)) {
      cout_spidrdev_err(i, "###setPllConfig");
      status = false;
    }
    if (!spidrctrl->getPllConfig(i, &pll_cfg)) {
      cout_spidrdev_err(i, "###setPllConfig");
      status = false;
    }
    cout << "[Note] Device " << i << ": PLL config = " << hex << pll_cfg << dec
         << endl;
  }

  // Set Timepix3 into acquisition mode
  if (!spidrctrl->datadrivenReadout()) {
    cout_spidr_err("###datadrivenReadout");
    status = false;
  }

  for (unsigned int i = 0; i < nDevices; ++i) {
    // Reapply ethernet mask
    if (!applyEthernetMask(spidrctrl, i, 0xFFFF)) status = false;
  }
  return status;
}

//=============================================================================
bool start_run(SpidrController *spidrctrl, 
               std::vector<Device>& devices,
               const int& run_nr, const std::string& description) {
//=============================================================================

  bool status = true;
  const unsigned int nDevices = devices.size();
  cout << "[Note] Starting run " << endl;
  // Reset shutter counters a.o.
  if (!spidrctrl->resetCounters()) {
    cout_spidr_err("###resetCounters");
    status = false;
  }
  // Reset counters for statistic of packets
  if (!spidrctrl->resetPacketCounters()) {
    cout_spidr_err("###resetPacketCounters");
    status = false;
  }
  // Get packet counters (before start of run).
  if (!spidrctrl->getDataPacketCounter(&spidr_data_packets_sent[0])) {
    cout_spidr_err("###getDataPacketCounter");
    status = false;
  }
  int spidr_mon_packets_sent = 0;
  if (!spidrctrl->getMonPacketCounter(&spidr_mon_packets_sent)) {
    cout_spidr_err("###getMonPacketCounter");
    status = false;
  }
  int spidr_pixel_packets_sent = 0;
  if (!spidrctrl->getPixelPacketCounter(&spidr_pixel_packets_sent)) {
    cout_spidr_err("###getPixelPacketCounter");
    status = false;
  }
  int spidr_pause_packets_rec = 0;
  if (!spidrctrl->getPausePacketCounter(&spidr_pause_packets_rec)) {
    cout_spidr_err("###getPausePacketCounter");
    status = false;
  }
  cout << "[Note] Spidr packet count before run:\n";
  cout << "  Ethernet pause packets      " << spidr_pause_packets_rec << endl;
  cout << "  Ethernet monitoring packets " << spidr_mon_packets_sent << endl;
  cout << "  Ethernet data packets       " << spidr_data_packets_sent[0] << endl;
  cout << "  Spidr pixel packets:        " << spidr_pixel_packets_sent << endl;

  // Get packet counters for each device.
  for (unsigned int i = 0; i < nDevices; ++i) {
    if (!devices[i].enabled) continue;
    devices[i].daq_packets_rec[0] = devices[i].daq->packetsReceivedCount();
    devices[i].daq_packets_lost[0] = devices[i].daq->packetsLostCount();
    cout << "[Note] DAQ " << i << " receive packet count before run: "
         << devices[i].daq_packets_rec[0] << " (and lost " 
         << devices[i].daq_packets_lost[0] << ")\n";
  }

  // Open data files.
  for (unsigned int i = 0; i < nDevices; ++i) {
    if (!devices[i].enabled) continue;
    cout << "[Note] Opening data file " << devices[i].datafile << endl;
    if (!devices[i].daq->startRecording(devices[i].datafile, run_nr, 
                                        description, true)) {
      cout_daqdev_err(i, "###startRecording", devices[i].daq);
      status = false;
    }
    // Add timestamp to data file.
    unsigned int timer_lo1, timer_hi1;
    if (!spidrctrl->getTimer(i, &timer_lo1, &timer_hi1)) {
      cout_spidrdev_err(i, "###getTimer");
      status = false;
    }
  }

  return status;
}

//=============================================================
bool stop_run(SpidrController *spidrctrl, 
              std::vector<Device>& devices) {
//=============================================================

  bool status = true;
  const unsigned int nDevices = devices.size();
  timestamp(spidrctrl, devices);
  // Wait some time to collect last ethernet packets.
  sleep(1);  
  for (unsigned int i = 0; i < nDevices; ++i) {
    if (!devices[i].enabled) continue;
    unsigned int timer_lo1, timer_hi1;
    if (!spidrctrl->getShutterStart(i, &timer_lo1, &timer_hi1)) {
      cout_spidrdev_err(i, "###getShutterStart");
      status = false;
    }
    unsigned int timer_lo2, timer_hi2;
    if (!spidrctrl->getShutterEnd(i, &timer_lo2, &timer_hi2)) {
      cout_spidrdev_err(i, "###getShutterEnd");
      status = false;
    }
  }

  for (unsigned int i = 0; i < nDevices; ++i) {
    if (!devices[i].enabled) continue;
    if (!devices[i].daq->stopRecording()) {
      cout_daqdev_err(i, "###stopRecording", devices[i].daq);
      status = false;
    }
  }

  if (!spidrctrl->getDataPacketCounter(&spidr_data_packets_sent[1])) {
    cout_spidr_err("###getDataPacketCounter");
    status = false;
  }
  int spidr_mon_packets_sent = 0;
  if (!spidrctrl->getMonPacketCounter(&spidr_mon_packets_sent)) {
    cout_spidr_err("###getMonPacketCounter");
    status = false;
  }
  int spidr_pixel_packets_sent = 0;
  if (!spidrctrl->getPixelPacketCounter(&spidr_pixel_packets_sent)) {
    cout_spidr_err("###getPixelPacketCounter");
    status = false;
  }
  int spidr_pause_packets_rec = 0;
  if (!spidrctrl->getPausePacketCounter(&spidr_pause_packets_rec)) {
    cout_spidr_err("###getPausePacketCounter");
    status = false;
  }
  cout << "[Note] Spidr packet count after run:\n";
  cout << "  Ethernet pause packets      " << spidr_pause_packets_rec << endl;
  cout << "  Ethernet monitoring packets " << spidr_mon_packets_sent << endl;
  cout << "  Ethernet data packets       " << spidr_data_packets_sent[1] << endl;
  cout << "  Spidr pixel packets:        " << spidr_pixel_packets_sent << endl;

  int missing_packets = spidr_data_packets_sent[1] - spidr_data_packets_sent[0];
  for (unsigned int i = 0; i < nDevices; ++i) {
    if (!devices[i].enabled) continue;
    devices[i].daq_packets_rec[1] = devices[i].daq->packetsReceivedCount();
    devices[i].daq_packets_lost[1] = devices[i].daq->packetsLostCount();
    cout << "[Note] DAQ " << i << " receive packet count after run: "
         << devices[i].daq_packets_rec[1] << " (and lost " 
         << devices[i].daq_packets_lost[1] << ")\n";
    int nPackets = devices[i].daq_packets_rec[1] - devices[i].daq_packets_rec[0];
    missing_packets -= nPackets;
  }
  if (missing_packets != 0) {
    cout << "[Warning] Missing ethernet packets after run: " << missing_packets
         << endl;
  } else {
    cout << "[Note] No missing ethernet packets after run " << endl;
  }
  return status;
}

//=============================================================
bool timestamp(SpidrController *spidrctrl, 
               const std::vector<Device>& devices) {
//=============================================================

  bool status = true;
  // Add timestamp to data file.
  const unsigned int nDevices = devices.size();
  for (unsigned int i = 0; i < nDevices; ++i) {
    unsigned int timer_lo1, timer_hi1;
    if (!spidrctrl->getTimer(i, &timer_lo1, &timer_hi1)) {
      cout_spidr_err("###getTimer");
      status = false;
    }
  }
  return status;
}

//=============================================================
bool devId_tostring(int devId, char *devIdstring) {
//=============================================================
  const int waferno = (devId >> 8) & 0xFFF;
  const int id_y = (devId >> 4) & 0xF;
  const int id_x = (devId >> 0) & 0xF;
  // make readable device identifier
  sprintf(devIdstring, "W%04d_%c%02d", waferno, (char)(id_x - 1) + 'A', id_y);
  return true;
}

//=============================================================
void get_temperature(SpidrController *spidrctrl,
                     std::vector<Device>& devices) {
//=============================================================

  const unsigned int nDevices = devices.size();
  const int navg = 1;
  // Conversion factor (1.5 V correspond to ADC 4095).
  const double conversion = 1.5 / 4095.;
  for (unsigned int i = 0; i < nDevices; ++i) {
    if (!devices[i].enabled) continue;
    spidrctrl->setSenseDac(i, TPX3_BANDGAP_OUTPUT);
    usleep(200000);
    int adc1;
    spidrctrl->getAdc(&adc1, navg); 
    usleep(200000);
    spidrctrl->setSenseDac(i, TPX3_BANDGAP_TEMP);
    usleep(200000);
    int adc2;
    spidrctrl->getAdc(&adc2, navg);
    usleep(200000);
    const double bandGapOutput = conversion * adc1 / navg;
    const double bandGapTemp = conversion * adc2 / navg; 
    const double temperature = 88.75 - 607.3 * (bandGapTemp - bandGapOutput);
    cout << "[Note] Device " << i << ": temperature = " << temperature << "\n";
  }

}

//=====================================================
bool applyEthernetMask(SpidrController* spidrctrl, 
                       const int dev, const int mask) { 
//=====================================================

  bool status = true;
  int eth_mask, cpu_mask;
  if (!spidrctrl->getHeaderFilter(dev, &eth_mask, &cpu_mask))
    cout_spidrdev_err(dev, "###getHeaderFilter");
  if (!spidrctrl->setHeaderFilter(dev, mask, cpu_mask)) {
    cout_spidrdev_err(dev, "###setHeaderFilter");
    status = false;
  }
  if (!spidrctrl->getHeaderFilter(dev, &eth_mask, &cpu_mask)) {
    cout_spidrdev_err(dev, "###getHeaderFilter");
    status = false;
  }
  cout << "[Note] Device " << dev << hex << ": eth_mask = " << eth_mask
       << "  cpu_mask = " << cpu_mask << dec << endl;
  return status;

}

//=============================================================
bool analyse(const std::string& directory) {
//=============================================================
  const uint64_t oneSecond = 4096 * 40000000UL;
  DIR *dir = opendir(directory.c_str());
  if (dir == NULL) {
    cout << "[Error] Cannot open directory " << directory << ".\n";
    return false;
  }
  std::vector<std::string> files;
  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    std::string filename = ent->d_name;
    // Check if the filename has an extension.
    const std::size_t index = filename.rfind('.');
    if (index == std::string::npos) continue;
    // Check if the extension is .dat.
    if (filename.substr(index + 1) != "dat") continue;
    // Add the file to the list.
    filename = filename.substr(0, index);
    files.push_back(filename);
  }
  closedir(dir);
  if (files.empty()) {
    cout << "[Error] Could not find any data files in " << directory << endl; 
    return false;
  }
  bool status = true;
  std::vector<std::string>::const_iterator it;
  for (it = files.begin(); it != files.end(); ++it) {
    std::string filename = directory + "/" + *it + ".dat";
    FILE *fp = fopen(filename.c_str(), "rb");
    if (fp == NULL) {
      cout << "[Error] Cannot open file " << filename << endl;
      status = false;
      continue;
    }
    // Read the first fields of the header.
    uint32_t headerID;
    if (fread(&headerID, sizeof(headerID), 1, fp) == 0) {
      cout << "[Error] Cannot read header ID" << endl;
      fclose(fp);
      status = false;
      continue;
    }
    uint32_t headerSize;
    if (fread(&headerSize, sizeof(headerSize), 1, fp) == 0) {
      cout << "[Error] Cannot read header size" << endl;
      fclose(fp);
      status = false;
      continue;
    }
    rewind(fp);
    // Read the complete header
    uint32_t* spidrHeader = (uint32_t*)malloc(sizeof(uint32_t) * headerSize);
    if (fread(&spidrHeader, sizeof(spidrHeader), 1, fp) == 0) {
      cout << "[Error] Cannot read header" << endl;
      fclose(fp);
      status = false;
      continue;
    }
    // Setup the histograms.
    setupStyle();
    std::string name = "hToT" + *it; 
    std::string title = *it + ";TOT;entries"; 
    TH1F* hToT = new TH1F(name.c_str(), title.c_str(), 
                          500, 0.5, 500.5);
    name = "hHitMap" + *it;
    title = *it + ";column;row;entries";
    TH2F* hHitMap = new TH2F(name.c_str(), title.c_str(), 
                             256, -0.5, 255.5, 256, -0.5, 255.5);
    name = "hRate" + *it;
    title = *it + ";time [s];packets";
    TH1F* hRate = new TH1F(name.c_str(), title.c_str(),
                           100, -0.5, 99.5); 
    uint64_t data_packet = 0;
    while (!feof(fp)) {
      if (fread(&data_packet, sizeof(uint64_t), 1, fp) == 0) continue;
      const unsigned int header = 0xF & (data_packet >> 60);
      if (header == 0xA || header == 0xB) {  
        unsigned int pixelAddress = 0xFFFF & (data_packet >> 44);
        // Decode the pixel address, first get the double column. 
        const unsigned int dcol = (0xFE00 & pixelAddress) >> 8;
        // Get the super pixel address.
        const unsigned int spix = (0x01F8 & pixelAddress) >> 1;
        // Get the address of the pixel within the super pixel. 
        const unsigned int pix = (0x0007 & pixelAddress); 
        // Calculate the row and column numbers.
        unsigned int col = dcol + pix / 4;
        unsigned int row = spix + (pix & 0x3);
        const unsigned int data = (data_packet & 0x00000FFFFFFF0000) >> 16;
        const unsigned int tot = (data & 0x00003FF0) >> 4;
        const uint64_t spidrTime(data_packet & 0x000000000000FFFF); 
        const uint64_t ftoa(data & 0x0000000F);
        const uint64_t toa((data & 0x0FFFC000) >> 14);
        // Calculate the timestamp.
        const uint64_t t = ((spidrTime << 18) + (toa << 4) + (15 - ftoa)) << 8;
        // Fill the histograms.
        hToT->Fill(tot);
        hHitMap->Fill(col, row);
        hRate->Fill(double(t) / oneSecond);
      }
    }
    name = "c" + *it;
    title = *it;
    TCanvas* c = new TCanvas(name.c_str(), title.c_str(), 800, 400);
    c->Divide(2, 1);
    c->cd(1);
    hToT->Draw();
    c->cd(2);
    hHitMap->Draw("colz");
    hHitMap->Write("hHitMap");
    hToT->Write("hToT");
  }

  return status;

}

void setupStyle() {

  // Use times new roman, precision 2 
  int lhcbFont = 132;
  // Line thickness
  double lhcbWidth = 2.00;
  // Text size
  double lhcbTSize = 0.06; 
  
  // use plain black on white colors
  gROOT->SetStyle("Plain"); 
  
  gStyle->SetFillColor(1);
  gStyle->SetFillStyle(1001);   // solid
  gStyle->SetFrameFillColor(0);
  gStyle->SetFrameBorderMode(0);
  gStyle->SetPadBorderMode(0);
  gStyle->SetPadColor(0);
  gStyle->SetCanvasBorderMode(0);
  gStyle->SetCanvasColor(0);
  gStyle->SetStatColor(0);
  gStyle->SetLegendBorderSize(0);
  gStyle->SetLegendFont(132);

  // If you want the usual gradient palette (blue -> red)
  gStyle->SetPalette(1);

  // set the paper & margin sizes
  gStyle->SetPaperSize(20,26);
  gStyle->SetPadTopMargin(0.05);
  gStyle->SetPadRightMargin(0.15); // default: 0.05
  gStyle->SetPadBottomMargin(0.16);
  gStyle->SetPadLeftMargin(0.14);
  
  // use large fonts
  gStyle->SetTextFont(lhcbFont);
  gStyle->SetTextSize(lhcbTSize);
  gStyle->SetLabelFont(lhcbFont,"x");
  gStyle->SetLabelFont(lhcbFont,"y");
  gStyle->SetLabelFont(lhcbFont,"z");
  gStyle->SetLabelSize(lhcbTSize,"x");
  gStyle->SetLabelSize(lhcbTSize,"y");
  gStyle->SetLabelSize(lhcbTSize,"z");
  gStyle->SetTitleFont(lhcbFont);
  gStyle->SetTitleFont(lhcbFont,"x");
  gStyle->SetTitleFont(lhcbFont,"y");
  gStyle->SetTitleFont(lhcbFont,"z");
  gStyle->SetTitleSize(1.2*lhcbTSize,"x");
  gStyle->SetTitleSize(1.2*lhcbTSize,"y");
  gStyle->SetTitleSize(1.2*lhcbTSize,"z");

  // use medium bold lines and thick markers
  gStyle->SetLineWidth(lhcbWidth);
  gStyle->SetFrameLineWidth(lhcbWidth);
  gStyle->SetHistLineWidth(lhcbWidth);
  gStyle->SetFuncWidth(lhcbWidth);
  gStyle->SetGridWidth(lhcbWidth);
  gStyle->SetLineStyleString(2,"[12 12]"); // postscript dashes
  gStyle->SetMarkerStyle(20);
  gStyle->SetMarkerSize(1.0);

  // label offsets
  gStyle->SetLabelOffset(0.010,"X");
  gStyle->SetLabelOffset(0.010,"Y");

  // by default, do not display histogram decorations:
  gStyle->SetOptStat(0);  
  gStyle->SetOptStat("emr");  // show only nent -e , mean - m , rms -r
  // full opts at http://root.cern.ch/root/html/TStyle.html#TStyle:SetOptStat
  gStyle->SetStatFormat("6.3g"); // specified as c printf options
  gStyle->SetOptTitle(0);
  gStyle->SetOptFit(0);
  //gStyle->SetOptFit(1011); // order is probability, Chi2, errors, parameters
  //titles
  gStyle->SetTitleOffset(0.95,"X");
  gStyle->SetTitleOffset(0.95,"Y");
  gStyle->SetTitleOffset(1.2,"Z");
  gStyle->SetTitleFillColor(0);
  gStyle->SetTitleStyle(0);
  gStyle->SetTitleBorderSize(0);
  gStyle->SetTitleFont(lhcbFont,"title");
  gStyle->SetTitleX(0.0);
  gStyle->SetTitleY(1.0); 
  gStyle->SetTitleW(1.0);
  gStyle->SetTitleH(0.05);
  
  // look of the statistics box:
  gStyle->SetStatBorderSize(0);
  gStyle->SetStatFont(lhcbFont);
  gStyle->SetStatFontSize(0.05);
  gStyle->SetStatX(0.9);
  gStyle->SetStatY(0.9);
  gStyle->SetStatW(0.25);
  gStyle->SetStatH(0.15);

  // put tick marks on top and RHS of plots
  gStyle->SetPadTickX(1);
  gStyle->SetPadTickY(1);

  // histogram divisions: only 5 in x to avoid label overlaps
  gStyle->SetNdivisions(505,"x");
  gStyle->SetNdivisions(510,"y");
  
  gROOT->ForceStyle();

}

