// Testpulse scan
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <iomanip>

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
bool applyTrimDacs(SpidrController* spidrctrl, 
                   std::vector<Device>& devices, const int dev);
bool applyEthernetMask(SpidrController* spidrctrl, 
                       const int dev, const int mask);
bool setShutter(SpidrController* spidrctrl, std::vector<Device>& devices);
bool devId_tostring(int devId, char *devIdstring);

void cout_daqdev_err(const int dev, const std::string& str, SpidrDaq* daq) {
  cout << "Dev " << dev << " " << str << ": " << daq->errorString() << endl;
}

int main(int argc, char *argv[]) {

  const bool dryrun = false;
  // Spacing
  const int spacing = dryrun ? 1 : 8;
  // VTP_COARSE DAC 
  const int tpc = 64; 
  const int tpstep = 5;
  const int tpmax = 512;
  const int nPulses = 100;
  const int period = 50;

  //  const std::string cfg_path = "/home/timepix3/CONFIG";
  const std::string cfg_path = "/mnt/CONFIGS";
  //const std::string data_path = "/data/LABDATA/TP";
  //const std::string data_path = "/data/LABDATA/TP";
  const std::string data_path = "";

  if (argc != 2) {
    cout << "usage: TpScanDual <file-prefix>" << endl;
    return 1;
  }
  const std::string fileprefix = argv[1];

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

  int errstat;
  if (!spidrctrl->reset(&errstat)) {
    cout << "###reset errorstat: " << hex << errstat << dec << endl;
  }

  // Determine number of devices, does not check if devices are active
  int nMaxDevices = 0;
  if (!spidrctrl->getDeviceCount(&nMaxDevices)) {
    cout_spidr_err("###getDeviceCount");
  } 
  cout << "[Note] Number of devices supported by firmware: " 
       << nMaxDevices << endl;
  const unsigned int nDevices = nMaxDevices;
  //const unsigned int nDevices = 1;
  std::vector<Device> devices(nDevices);
  for (unsigned int i = 0; i < nDevices; ++i) {
    // Check link status
    int linkstatus;
    if (!spidrctrl->getLinkStatus(i, &linkstatus)) {
      cout_spidr_err("###getLinkStatus()");
    }
    // Link status: bits 0-7: 0=link enabled; bits 16-23: 1=link locked
    const int links_enabled_mask = (~linkstatus) & 0xFF;
    const int links_locked_mask = (linkstatus & 0xFF0000) >> 16;
    if (links_enabled_mask != 0 && links_locked_mask == links_enabled_mask) {
      // At least one link is enabled, and all links enabled are locked
      cout << "[Note] Device " << i << ": linkstatus = " 
           << hex << linkstatus << dec << endl;
      devices[i].enabled = true;
      // Get device IDs in readable form
      int devIds[16];
      if (!spidrctrl->getDeviceIds(devIds)) cout_spidr_err("###getDevidIds()");
      char devIdstring[16];
      devId_tostring(devIds[i], devIdstring);
      cout << "[Note] Device " << i << ": " << devIdstring << endl;
      devices[i].id = devIdstring;
    } else {
      cout << "###linkstatus: " << hex << linkstatus << dec << endl;
      devices[i].enabled = false;
      continue;
    }
    // Set packet filter: ethernet no filter
    applyEthernetMask(spidrctrl, i, 0xFFFF);
  }
  //devices[0].enabled = false;
  //devices[1].enabled = false;
  //devices[2].enabled = false;
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

  // Select whether or not to let the FPGA do the ToT/ToA decoding
  // gray decoding (for ToA only) has priority over LFSR decoding
  if (!spidrctrl->setDecodersEna(true)) cout_spidr_err("###setDecodersEna");
 
  for (unsigned int i = 0; i < nDevices; ++i) {
    if (!devices[i].enabled) continue;
    const std::string trimfile = devices[i].id + "_trimdacs.txt";
    const std::string dacfile = devices[i].id + "_dacs.txt";
    devices[i].trimfile = cfg_path + "/" + trimfile;
    devices[i].dacfile = cfg_path + "/" + dacfile;
    devices[i].datafile = data_path + "/" + fileprefix + "/";
  }

  bool status = configure(spidrctrl, devices);

  // Mask all but selected pixels
  //for (int selrow = 0; selrow < 1; ++selrow) {
  //  for (int selcol = 0; selcol < 1; ++selcol) {

  for (int selrow = 0; selrow < spacing; ++selrow) {
    for (int selcol = 0; selcol < spacing; ++selcol) {
      for (unsigned int i = 0; i < nDevices; ++i) {
        if (!devices[i].enabled) continue;
        // Clear pixel configuration in chip
        if (!spidrctrl->resetPixels(i)) {
          cout_spidrdev_err(i, "###resetPixels"); 
          status = false;
        }
      }
      for (unsigned int i = 0; i < nDevices; ++i) {
        if (!devices[i].enabled) continue;
        // Clear local pixel config, no return value
        spidrctrl->resetPixelConfig();
        // Disable all testpulse bits
        if (!spidrctrl->setPixelTestEna(ALL_PIXELS, ALL_PIXELS, false)) {
          cout_spidrdev_err(i, "###setPixelTestEna"); 
          status = false;
        }
        // Enable all pixels
        if (!spidrctrl->setPixelMask(ALL_PIXELS, ALL_PIXELS, false)) {
          cout_spidrdev_err(i, "###setPixelMask"); 
          status = false;
        }
        // Enable selected pixels and activate testpulses.
        for (int row = 0; row < 256; ++row) {
          for (int col = 0; col < 256; ++col) {
            if ((row % spacing == selrow) && (col % spacing == selcol)) {
              spidrctrl->setPixelMask(col, row, false);  // enable
              spidrctrl->setPixelTestEna(col, row, true);
            } else {
              spidrctrl->setPixelMask(col, row, true);  // else mask
              spidrctrl->setPixelTestEna(col, row, false);
            }
            if (col % spacing == selcol) {
              spidrctrl->setCtprBit(col, 1);
            }
          }
        }
        // Write column testpulse register.
        spidrctrl->setCtprBits(1);
        if (!spidrctrl->setCtpr(i)) {
          cout_spidrdev_err(i, "###setCtpr"); 
          status = false;
        }

        // Read thresholds from file.
        if (!applyTrimDacs(spidrctrl, devices, i)) return false;

        // Write pixel config to chip.
        if (!spidrctrl->setPixelConfig(i)) {
          cout_spidrdev_err(i, "###setPixelConfig"); 
          status = false;
        }
      }

      // Set acquisition parameters 
      setShutter(spidrctrl, devices);

      for (unsigned int i = 0; i < nDevices; ++i) {
        if (devices[i].enabled) devices[i].daq->setSampling(true);
      }

      // Set Timepix3 into acquisition mode
      if (!spidrctrl->datadrivenReadout()) {
        cout_spidr_err("###datadrivenReadout");
        status = false;
      }

      // Reapply ethernet mask
      for (unsigned int i = 0; i < nDevices; ++i) {
        if (!devices[i].enabled) continue;
        applyEthernetMask(spidrctrl, i, 0xFFFF);
      }

      if (!spidrctrl->restartTimers()) cout_spidr_err("###restartTimers");
     
      std::vector<int> adcc(nDevices, 0); 
      for (unsigned int i = 0; i < nDevices; ++i) {
        if (!devices[i].enabled) continue;

        spidrctrl->selectChipBoard( i + 1 );    // for telescope, 2 chip boards
        // Set coarse testpulse voltage.
        if (!spidrctrl->setDac(i, TPX3_VTP_COARSE, tpc)) {
          cout_spidrdev_err(i, "###setDac (TPX3_VTP_COARSE)");
          status = false;
        }
        // Read coarse ADC.
        spidrctrl->setSenseDac(i, TPX3_VTP_COARSE);
        usleep(200000);
        int adcCoarse = 0;
        spidrctrl->getAdc(&adcCoarse, 1);
        adcc[i] = adcCoarse;
      }
      for (int tpf = tpc; tpf < tpmax; tpf += tpstep) {
        for (unsigned int i = 0; i < nDevices; ++i) {
          if (!devices[i].enabled) continue;
          // Set fine testpulse voltage.
          if (!spidrctrl->setDac(i, TPX3_VTP_FINE, tpf)) {
            cout_spidrdev_err(i, "###setDac (TPX3_VTP_FINE)");
            status = false;
          } 
          unsigned int timer_low, timer_high;
          spidrctrl->getTimer(i, &timer_low, &timer_high); // adds timestamp to datafile
          // Read fine ADC.
          spidrctrl->setSenseDac(i, TPX3_VTP_FINE);
          if (dryrun) usleep(10000000);
          usleep(1000);
          int adcFine = 0;
          spidrctrl->getAdc(&adcFine, 1);
          cout << "[Note] VTP_COARSE = " << tpc << " (ADC = " << adcc[i]
               << "), VTP_FINE = " << tpf << " (ADC = " << adcFine << ")\n";
          // Configure testpulse generator
          if (!spidrctrl->setTpNumber(i, nPulses)) {
            cout_spidrdev_err(i, "###setTpNumber");
            status = false;
          }
          if (!spidrctrl->setTpPeriodPhase(i, period, 0)) {
            cout_spidrdev_err(i, "###setTpPeriodPhase");
            status = false;
          }
          char description[128];
          sprintf(description,
                  "tp_c = %d tp_f = %d adc_c = %d adc_f = %d", 
                  tpc, tpf, adcc[i], adcFine);
          char filename[256];
          sprintf(filename, "%s/CHIP%d/%s_sc%d_sr%d_tpc%d_f%d.dat",
                  devices[i].datafile.c_str(), i, 
                  devices[i].id.c_str(), selcol, selrow, tpc, tpf);
          if (dryrun) continue;
          cout << "[Note] Opening data file" << filename << endl;
          if (!devices[i].daq->startRecording(filename, 0, description)) {
            cout_daqdev_err(i, "###startRecording", devices[i].daq);
            status = false;
          }
        }
        usleep(100);
        if (dryrun) continue;
        // Start triggers.
        cout << "[Note] Starting auto trigger " << endl;
        if (!spidrctrl->startAutoTrigger()) {
          cout_spidr_err("###startAutoTrigger");
          status = false;
        }
        const int shutter_length_us = 50000;
        usleep(shutter_length_us);

        for (unsigned int i = 0; i < nDevices; ++i) {
          if (devices[i].enabled) devices[i].daq->stopRecording();
        }
        int intcntr = 0;
        if (!spidrctrl->getShutterCounter(&intcntr)) {
          cout_spidr_err("###getShutterCounter");
          status= false;
        }
        cout << "[Note] Number of shutters given " << intcntr << endl;
      }
    }
  }

  // Clean up.
  for (unsigned int i = 0; i < nDevices; ++i) {
    devices[i].daq->stop();
    delete devices[i].daq;
  }
  delete spidrctrl;

  if (status) {
    cout << "[Note] Finished OK." << endl;
  }
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

    // Disable all testpulse bits
    if (!spidrctrl->setPixelTestEna(ALL_PIXELS, ALL_PIXELS, false)) {
      cout_spidrdev_err(i, "###setPixelTestEna");
      status = false;
    }
    // Enable all pixels
    if (!spidrctrl->setPixelMask(ALL_PIXELS, ALL_PIXELS, false)) {
      cout_spidrdev_err(i, "###setPixelMask");
      status = false;
    }

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

  setShutter(spidrctrl, devices);
  
  for (unsigned int i = 0; i < nDevices; ++i) {
    // PLL configuration: 40 MHz on pixel matrix
    // int pll_cfg = 0x01E;
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

  // Reapply ethernet mask
  for (unsigned int i = 0; i < nDevices; ++i) {
    if (!devices[i].enabled) continue;
    applyEthernetMask(spidrctrl, i, 0xFFFF);
  }

  return status;
}

//=====================================================
bool applyTrimDacs(SpidrController *spidrctrl, 
                   std::vector<Device>& devices, const int dev) {
//=====================================================

  bool status = true;
  // Read thresholds from file
  FILE* fp = fopen(devices[dev].trimfile.c_str(), "r");
  if (fp == NULL) {
    cout << "[Warning] Cannot open pixel config file " 
         << devices[dev].trimfile << endl;
    return false;
  }
  cout << "[Note] Reading " << devices[dev].trimfile << endl;
  while (!feof(fp)) {
    char line[1024];
    if (fgets(line, 256, fp) == NULL) continue;
    if (line[0] == '#') continue; 
    int row, col, thr, mask, tp_ena;
    sscanf(line, "%d %d %d %d %d", &col, &row, &thr, &mask, &tp_ena);
    if (!spidrctrl->setPixelThreshold(col, row, thr)) {
      cout_spidrdev_err(dev, "###setPixelThreshold");
      status = false;
    }
    if (mask) {
      if (!spidrctrl->setPixelMask(col, row, true)) {
        cout_spidrdev_err(dev, "###setPixelMask");
        status = false;
      }
    }
  }
  fclose(fp);
  return status;

}

//=====================================================
bool applyEthernetMask(SpidrController* spidrctrl, const int dev, const int mask) { 
//=====================================================

  int eth_mask, cpu_mask;
  if (!spidrctrl->getHeaderFilter(dev, &eth_mask, &cpu_mask))
    cout << "Dev " << dev
         << " ###getHeaderFilter: " << spidrctrl->errorString() << endl;
  // cpu_mask = 0x0080;
  if (!spidrctrl->setHeaderFilter(dev, mask, cpu_mask))
    cout << "Dev " << dev
         << " ###setHeaderFilter: " << spidrctrl->errorString() << endl;
  if (!spidrctrl->getHeaderFilter(dev, &eth_mask, &cpu_mask))
    cout << "Dev " << dev
         << " ###getHeaderFilter: " << spidrctrl->errorString() << endl;
  cout << "[Note] dev " << dev << hex << " eth_mask = " << eth_mask
       << "  cpu_mask = " << cpu_mask << dec << endl;
  return true;

}

//=====================================================
bool setShutter(SpidrController* spidrctrl,
                std::vector<Device>& devices) {
//=====================================================

  const unsigned int nDevices = devices.size();
  bool status = true;
  // Configure the shutter trigger
  const int trig_mode = 4;
  const int trig_length_us = 100000;  // 100 ms
  const int trig_freq_hz = 5; // Hz
  const int nr_of_trigs = 1;  // 1 triggers

  if (!spidrctrl->setShutterTriggerConfig(trig_mode, trig_length_us,
                                          trig_freq_hz, nr_of_trigs)) {
    cout_spidr_err("###setShutterTriggerConfig");
    status = false;
  }

  // Read back the configuration
  int i1, i2, i3, i4;
  if (!spidrctrl->getShutterTriggerConfig(&i1, &i2, &i3, &i4)) {
    cout_spidr_err("###getShutterTriggerConfig");
    status = false;
  }
  cout << "[Note] Read trigger config " << hex << i1 << " " << i2 << " " << i3
       << " " << i4 << dec << endl;

  //const int cfg = TPX3_POLARITY_HPLUS | 
  const int cfg = TPX3_POLARITY_EMIN | 
                  TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA |
                  TPX3_FASTLO_ENA | TPX3_TESTPULSE_ENA; // |
                  // TPX3_SELECTTP_EXT_INT |
                  // TPX3_SELECTTP_DIGITAL
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
