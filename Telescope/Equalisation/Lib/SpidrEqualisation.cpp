#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <cmath> 
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <dirent.h>

#include "TH1F.h"
#include "TF1.h"
#include "TFile.h"
#include "TH2S.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TROOT.h"

#include "tpx3defs.h"
#include "SpidrController.h"
#include "SpidrDaq.h"

#include "SpidrEqualisation.h"

using namespace std;

// ---------------------------------------------------------------------------
// Constructor 
// ---------------------------------------------------------------------------
SpidrEqualisation::SpidrEqualisation(SpidrController* spidrctrl, 
                                     SpidrDaq* spidrdaq) : 
  m_ctrl(spidrctrl), m_daq(spidrdaq),
  m_device(0),
  m_stddev(4),
  m_spacing(2), 
  m_thlmin(0), m_thlmax(512), m_thlstep(1),
  m_thlcoarse(6), m_ikrum(10),
  m_trig_length_us(50), m_trigmode(4),
  m_trig_freq_hz(100), m_nr_of_trigs(1),
  m_filename(""), m_dacfilename(""),
  m_eminus(true) {

}

// ---------------------------------------------------------------------------
// Main equalisation function 
// ---------------------------------------------------------------------------
bool SpidrEqualisation::equalise(const bool scan0, const bool scan15, 
                                 const bool analyse0, const bool analyse15,
                                 const bool extract,
                                 const bool scanFinal, const bool analyseFinal,
                                 const bool mask, const bool plot) {

  if (scan0 || scan15 || scanFinal) {
    if (!checkCommunication()) return false;
    cout << "[Note] Setting configuration" << endl;
    if (!setConfiguration()) {
      printFinal("FAILED");
      return false;
    }
  }
  std::stringstream ss;
  ss << m_filename << "_spacing_" << m_spacing;
  const std::string filenameBase = ss.str();
  // Noise scan with all pixels at lowest trim value.
  if (scan0) {
    cout << "[Note] Taking data with all pixels at TRIM 0\n";
    if (!setTHLTrimALL(0)) return false;
    if (!takeData(filenameBase + "_0.dat")) {
      printFinal("FAILED");
      return false;
    }
  }
  // Noise scan with all pixels at highest trim value.
  if (scan15) {
    cout << "[Note] Taking data with all pixels at TRIM 15\n";
    if (!setTHLTrimALL(15)) return false;
    if (!takeData(filenameBase + "_15.dat")) {
      printFinal("FAILED");
      return false;
    }
  }

  // Analyse the data for the lowest trim value.  
  if (analyse0) {
    cout << "[Note] Analysing data for TRIM 0\n";
    if (!analyseData(filenameBase + "_0")) {
      printFinal("FAILED");
      return false;
    }
  }
  // Analyse the data for the highest trim value.  
  if (analyse15) {
    cout << "[Note] Analysing data for TRIM 15\n";
    if (!analyseData(filenameBase + "_15")) {
      printFinal("FAILED");
      return false;
    }
  }
  // Find the trim values for each pixel.
  if (extract) { 
    cout << "[Note] Extracting trim values\n";
    if (!extractPars(filenameBase)) {
      printFinal("FAILED");
      return false;
    }
  }
  if (scanFinal) { 
    // Apply the trim values and rerun the acquisition.
    cout << "[Note] Applying trim values\n";
    if (!setTHLTrim(filenameBase + ".txt", false)) {
      printFinal("FAILED");
      return false;
    }
    cout << "[Note] Taking data\n";
    if (!takeData(filenameBase + "_equalised.dat")) {
      printFinal("FAILED");
      return false;
    }
  }
  // Analyse the data after equalisation.
  if (analyseFinal) {
    cout << "[Note] Analysing data\n";
    if (!analyseData(filenameBase + "_equalised")) {
      printFinal("FAILED");
      return false;
    }
  }
  cout << "[Note] Unmasking all pixels" << endl; 
  if (!m_ctrl->setPixelMask(ALL_PIXELS, ALL_PIXELS, false)) {
    printFinal("FAILED");
    return false;
  }
  if (!m_ctrl->setPixelConfig(m_device)) printError("setPixelConfig");
  if (mask) {
    cout << "[Note] Masking pixels out of range\n";
    if (!maskPixels(filenameBase)) {
      printFinal("FAILED");
      return false;
    } 
  }
 
  if (plot) {
    cout << "[Note] Creating plot\n";
    if (!plotEqualisation(filenameBase)) {
      printFinal("FAILED");
      return false;
    }
  }
  printFinal("DONE");
  return true;

}

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
bool SpidrEqualisation::quickEqualisation() {
 
  return equalise(true, true, true, true, true, false, false, false, false);
}

// ---------------------------------------------------------------------------
// Load trim values and mask bits from file.
// ---------------------------------------------------------------------------
bool SpidrEqualisation::loadEqualisation(const std::string& filename, const bool loadmask) {
 
  if (!checkCommunication()) return false;
  std::cout << "[Note] Loading pixel trim values from" << filename << "\n";
  if (!setTHLTrim(filename, loadmask)) {
    printFinal("FAILED");
    return false;
  }

  m_ctrl->setPixelConfig(m_device);
  printFinal("DONE");
  return true;
}

bool SpidrEqualisation::testEqualisation() {
 
  return equalise(false, false, false, false, false, true, true, true, false); 

}

// ---------------------------------------------------------------------------
// Verify the communication with the board.
// ---------------------------------------------------------------------------
bool SpidrEqualisation::checkCommunication() {

  if (!m_ctrl->isConnected()) {
    cerr << "[Error] Could not connect to device" << endl;
    cerr << m_ctrl->ipAddressString() << ": "
         << m_ctrl->connectionStateString() << ", "
         << m_ctrl->connectionErrString() << endl;
    return false;
  }   
  int linkstatus;
  if (!m_ctrl->getLinkStatus(0, &linkstatus)) {
    printError("getLinkStatus");
  }
  // Link status: bits 0-7: 0 = link enabled; bits 16-23: 1 = link locked
  int links_enabled_mask = (~linkstatus) & 0xFF;
  int links_locked_mask  = (linkstatus & 0xFF0000) >> 16;
  if (links_enabled_mask != 0 &&
      links_locked_mask == links_enabled_mask) {
    // At least one link (of 8) is enabled, and all links enabled are locked
    // cout << "[Note] linkstatus: " << hex << linkstatus << dec << endl;
  } else {
    std::stringstream ss;
    ss << std::hex << linkstatus;
    printError("linkstatus", ss.str());
  }
  const std::string errstr = m_daq->errorString();
  if (!errstr.empty()) printError("SpidrDaq", errstr);
  return true;
}

// ---------------------------------------------------------------------------
// Set the chip and readout configuration.
// ---------------------------------------------------------------------------
bool SpidrEqualisation::setConfiguration() {

  // Switch to slower 'serial links' output (instead of GTX links).
  if (!m_ctrl->setSpidrRegBit(0x2D0, 0)) printError("setSpidrRegBit");

  int errstat;
  if (!m_ctrl->reset(&errstat)) {
    std::stringstream ss;
    ss << std::hex << errstat;
    printError("reset", ss.str());
  }
  // Set the ethernet filter.
  int eth_mask, cpu_mask;
  m_ctrl->getHeaderFilter(m_device, &eth_mask, &cpu_mask);
  eth_mask = 0xffff;
  m_ctrl->setHeaderFilter(m_device, eth_mask, cpu_mask);
  // Reset the pixel configuration.
  if (!m_ctrl->resetPixels(m_device)) {
    printError("resetPixels");
    return false;
  }
  m_ctrl->resetPixelConfig();

  if (!m_ctrl->restartTimers()) printError("restartTimers");
  if (m_dacfilename.empty()) {
    cout << "[Note] Applying default DAC settings\n";
    if (!m_ctrl->setDacsDflt(m_device)) printError("setDacsDflt");
  } else {
    loadDacs(m_dacfilename);
  }
  // Set the acquisition mode.
  int config = 0;
  if (m_eminus) {
    std::cout << "[Note] Chip set to electron-collecting mode.\n";
    config = TPX3_POLARITY_EMIN | TPX3_ACQMODE_EVT_ITOT | TPX3_GRAYCOUNT_ENA;
  } else {
    std::cout << "[Note] Chip set to hole-collecting mode.\n";
    config = TPX3_POLARITY_HPLUS | TPX3_ACQMODE_EVT_ITOT | TPX3_GRAYCOUNT_ENA;
  }
  if (!m_ctrl->setGenConfig(m_device, config)) printError("setGenCfg");
  if (!m_ctrl->setDecodersEna(true)) printError("setDecodersEna");
  // Activate sequential readout mode.
  if (!m_ctrl->sequentialReadout(1)) printError("sequentialReadout");
  // Set the trigger configuration. 
  if (!m_ctrl->setShutterTriggerConfig(m_trigmode, m_trig_length_us,
                                       m_trig_freq_hz, m_nr_of_trigs)) {
    printError("setShutterTriggerConfig");
  }
  // Set the Ikrum DAC.
  std::cout << "[Note] Setting IBIAS_IKRUM to " << m_ikrum << "\n";
  if (!m_ctrl->setDac(m_device, TPX3_IBIAS_IKRUM, m_ikrum)) {
    printError("setDac (IBIAS_IKRUM)");
  }
  std::cout << "[Note] Setting VTHRESH_COARSE to " << m_thlcoarse << "\n";
  if (!m_ctrl->setDac(m_device, TPX3_VTHRESH_COARSE, m_thlcoarse)) { 
    printError("setDac (VTHRESH_COARSE)");
  }
  m_daq->setFlush(false);
  m_daq->setSampling(true);
  return true;
}

// ---------------------------------------------------------------------------
// Load the trim DAC mask from a file and apply it to the pixels.
// ---------------------------------------------------------------------------
bool SpidrEqualisation::setTHLTrim(const std::string& filename,
                                   const bool applyMasking) {

  FILE* fp = fopen(filename.c_str(), "r");
  if (fp == NULL) {
    cout << "[Error] Cannot open pixel config file " << filename << endl; 
    return false;
  }
      
  char line[256];
  while (!feof(fp)) {
    if (fgets(line, 256, fp) == NULL) continue;
    // Skip comments.
    if (line[0] == '#') continue;
    int row, col, thr, mask;
    sscanf(line, "%d %d %d %d", &col, &row, &thr, &mask);
    m_ctrl->setPixelThreshold(col, row, thr);
    if (mask && applyMasking) {
      m_ctrl->setPixelMask(col, row, true);
    }
  }
  m_ctrl->setPixelConfig(m_device);
  cout << "[Note] Pixel trim bits written to chip\n";
  return true;
}

// ---------------------------------------------------------------------------
// Set the trim DAC of all pixels to the same value.
// ---------------------------------------------------------------------------
bool SpidrEqualisation::setTHLTrimALL(const unsigned int trim) {

  if (!m_ctrl->setPixelThreshold(ALL_PIXELS, ALL_PIXELS, trim)) {
    printError("setPixelThreshold");
  }
  m_ctrl->setPixelConfig(m_device);
  return true;
}

// ---------------------------------------------------------------------------
// Run a noise scan for a given trim configuration.
// ---------------------------------------------------------------------------
bool SpidrEqualisation::takeData(const std::string& filename) {

  // Open a data file.
  m_daq->startRecording(filename.c_str(), 0);

  const unsigned int nSteps = m_spacing * m_spacing;
  int step = 1;
  const unsigned int sampleSize = 256 * 256 * 8;
  // Disable all testpulse bits.
  m_ctrl->setPixelTestEna(ALL_PIXELS, ALL_PIXELS, false);
  for (unsigned int selrow = 0; selrow < m_spacing; ++selrow) {
    for (unsigned int selcol = 0; selcol < m_spacing; ++selcol) {
      for (unsigned int col = 0; col < 256; ++col) {
        for (unsigned int row = 0; row < 256; ++row) { 
          if ((row % m_spacing == selrow) && (col % m_spacing == selcol)) {
            m_ctrl->setPixelMask(col, row, false);
          } else {
            m_ctrl->setPixelMask(col, row, true);
          }
        }
      }
      // Write the pixel configuration to the chip.
      if (!m_ctrl->setPixelConfig(m_device)) printError("setPixelConfig");
      cout << "[Note] Threshold scan " << step << "/" << nSteps << endl;
      // Threshold scan.
      for (int thl = m_thlmin; thl < m_thlmax; thl += m_thlstep) {
        const bool printProgress = true;
        if (printProgress) {
          cout << "   " << setprecision(1) << fixed 
               << ((float)(thl - m_thlmin) / (float)(m_thlmax - m_thlmin)) * 100. 
	       << "%\r" << flush;
        } 
        // Set the new threshold.
        if (!m_ctrl->setDac(m_device, TPX3_VTHRESH_FINE, thl)) {   
          std::stringstream ss;
          ss << "setDac (VTHRESH_FINE = " << thl << ")";
          printError(ss.str());
        }
        if (!m_ctrl->getDac(m_device, TPX3_VTHRESH_FINE, &thl)) {   
          std::stringstream ss;
          ss << "getDac (VTHRESH_FINE = " << thl << ")";
          printError(ss.str());
        }
        // Wait for the DAC to stabilise.
        usleep(200);
        // Start triggers.
        if (!m_ctrl->startAutoTrigger()) printError("startAutoTrigger");
        bool next_frame = true;
        while (next_frame) next_frame = m_daq->getSample(sampleSize, 100);
      }
      ++step;
    }
  }
  m_daq->stopRecording();
  return true;

}

// ---------------------------------------------------------------------------
// Analyse the data from a noise scan.
// ---------------------------------------------------------------------------
bool SpidrEqualisation::analyseData(const std::string& filename) {

  // Create a ROOT file.
  const std::string rootfilename = filename + ".root";
  TFile froot(rootfilename.c_str(), "RECREATE");
  if (!froot.IsOpen()) { 
    std::cerr << "[Error] Cannot create ROOT file " << rootfilename << "\n";
    return false;
  }
  // Setup the histograms.
  const unsigned int nRows = 256;
  const unsigned int nCols = 256;
  const unsigned int nPixels = nRows * nCols;
  TH2S hthrscan("hthrscan", 
                "Fine threshold scan;Pixel;Threshold;Counts", 
                nPixels, -0.5, nPixels - 0.5, 512, -0.5, 511.5);
  TH2F hitot2d("hitot2d", 
               "Integral Time over Threshold;Col;Row;iToT[counts]", 
               nCols, -0.5, nCols - 0.5, nRows, -0.5, nRows - 0.5);
  TH2F hhitmap("hhitmap", "Pixel Hitmap;Col;Row;Counts", 
               nCols, -0.5, nCols - 0.5, nRows, -0.5, nRows - 0.5);
  hitot2d.SetStats(kFALSE);
  hhitmap.SetStats(kFALSE);
  std::string labels[16] = {"per_ana", "per_opb", "per_pll", "per_gen", 
                            "per_time", "per_free", "per_free", "per_ctrl", 
                            "ld_pix_cfg?", "rd_pix_cfg", "data_seq", "data_dd", 
                            "ld_ctpt_cfg?", "ctpr_cfg", "stop_mtrx1", "stop_mtrx2"};
  TH1F hpackettype("hpackettype", "Packet Type; Type; Counts", 16, -0.5, 15.5);
  for (int i = 0; i < 16; ++i) {
    hpackettype.GetXaxis()->SetBinLabel(i + 1, labels[i].c_str());
  }

  // Open the data file.
  std::vector<std::string> files;
  if (!findDatFiles(filename, files)) return false;
  const std::string datfilename = files.front();
  FILE* fp = fopen(datfilename.c_str(), "rb");
  if (!fp) {
    std::cerr << "[Error] Cannot open data file " << datfilename << "\n";
    return false;
  } 
  // Read the first fields of the header.
  uint32_t headerID;
  if (fread(&headerID, sizeof(headerID), 1, fp) == 0) {
    std::cerr << "[Error] Cannot read header ID" << std::endl;
    return false;
  }
  uint32_t headerSize;
  if (fread(&headerSize, sizeof(headerSize), 1, fp) == 0) {
    std::cerr << "[Error] Cannot read header size" << std::endl;
    return false;
  }
  // Skip the header.
  rewind(fp);
  fseek(fp, headerSize, SEEK_SET);

  // Read the packets.
  ULong64_t pixdata = 0;
  UShort_t thr = 0;   
  while (!feof(fp)) {
    const int retval = fread(&pixdata, sizeof(ULong64_t), 1, fp);
    if (retval == 0) continue;
    const UChar_t header = ((pixdata & 0xF000000000000000) >> 60) & 0xF;
    if (header == 0xA || header == 0xB) {
      const UShort_t dcol = ((pixdata & 0x0FE0000000000000) >> 52);
      const UShort_t spix = ((pixdata & 0x001F800000000000) >> 45);
      const UShort_t pix  = ((pixdata & 0x0000700000000000) >> 44);
      const UShort_t col = (dcol + pix / 4);
      const UShort_t row = (spix + (pix & 0x3));
      const UShort_t pixno = col * 256 + row;
      const UInt_t data = ((pixdata & 0x00000FFFFFFF0000) >> 16);
      const UShort_t iToT = (data & 0x0FFFC000) >> 14;
      const UShort_t evcntr = (data & 0x00003FF0) >> 4;
      // Fill the histograms.
      hthrscan.Fill(pixno, thr, evcntr);
      hitot2d.Fill(col, row, iToT);
      hhitmap.Fill(col, row);
    }
    hpackettype.Fill(header);
    if ((pixdata & 0xFFFF000000000000) == 0x0300000000000000)  {
      thr = (pixdata & 0x000000003FE00000) >> 21;
    }  
  }
  const double mean = hthrscan.ProjectionY()->GetMean();
  TF1* fGauss = new TF1("fGauss", "gaus", mean - 70., mean + 70.);
  fGauss->SetRange(mean - 70., mean + 70.);
  hthrscan.FitSlicesY(fGauss);
  
  TH1D* hthrscan_1 = (TH1D*)gDirectory->Get("hthrscan_1");
  TH1D* hthrscan_2 = (TH1D*)gDirectory->Get("hthrscan_2");
  TH1F hNoiseMean("hNoiseMean", "Threshold;Pixels", 512, -0.5, 511.5);
  TH1F hNoiseRMS("hNoiseRMS", "Threshold;Pixels", 512, -0.5, 511.5);
  for (unsigned int i = 0; i < nPixels; ++i) {
    hNoiseMean.Fill(hthrscan_1->GetBinContent(i + 1));
    hNoiseRMS.Fill(hthrscan_2->GetBinContent(i + 1));
  }
  std::cout << "[Note] Average noise level: " << hNoiseMean.GetMean() << "\n";
  delete fGauss;

  froot.Write();
  froot.Close();
  return true;

}

// ---------------------------------------------------------------------------
// Determine the target threshold and find the trim bits for each pixel 
// ---------------------------------------------------------------------------
bool SpidrEqualisation::extractPars(const std::string& filename) {

  // Open ROOT file with results for trim 0.
  const std::string filename0 = filename + "_0.root";
  TFile f0(filename0.c_str(), "READ");
  f0.cd();
  TH1* hNoiseMean0 = (TH1*)gDirectory->Get("hNoiseMean");
  TH1* hScan0 = (TH1*)gDirectory->Get("hthrscan_1");
  hNoiseMean0->SetDirectory(0);
  hScan0->SetDirectory(0);
  f0.Close();
  // Get the average noise level for trim 0.
  hNoiseMean0->Fit("gaus", "Q");
  const double mu0 = hNoiseMean0->GetFunction("gaus")->GetParameter(1);

  // Open ROOT file with results for trim 15.
  const std::string filename15 = filename + "_15.root";
  TFile f15(filename15.c_str(), "READ");
  f15.cd();
  TH1* hNoiseMean15 = (TH1*)gDirectory->Get("hNoiseMean");
  TH1* hScan15 = (TH1*)gDirectory->Get("hthrscan_1");
  hNoiseMean15->SetDirectory(0);
  hScan15->SetDirectory(0);
  f15.Close(); 
  // Get the average noise level for trim 15.
  hNoiseMean15->Fit("gaus", "Q");
  const double mu15 = hNoiseMean15->GetFunction("gaus")->GetParameter(1);

  // Calculate the target threshold.
  const double target = 0.5 * (mu0 + mu15);
  std::cout << "[Note] Target threshold set to " << target << std::endl;
  TH1F hTrim("htrimdac", "; Trim DAC; Pixels", 16, -0.5, 15.5);
  const std::string filenametrim = filename + ".txt";
  FILE* fdac = fopen(filenametrim.c_str(), "w");
  fprintf(fdac, "#col row trim mask tp_ena\n");
  const unsigned int nPixels = 65536;
  for (unsigned int i = 0; i < nPixels; ++i) {
    const double thr0 = hScan0->GetBinContent(i + 1);
    const double thr15 = hScan15->GetBinContent(i + 1);
    const double deltathr = thr15 - thr0;  
    int trim = 15;
    int masked = 1;
    if (target >= thr0 && target <= thr15) {
      trim = (((target - thr0) / deltathr) * 15 + 0.5) / 1;
      masked = 0;
    }
    hTrim.Fill(trim);
    fprintf(fdac, "%4d %4d %4d %4d %4d\n", i / 256, i % 256, trim, masked, 0);
  }
  fclose(fdac);
  return true;

}

// ---------------------------------------------------------------------------
// Make a plot of the three noise level distributions
// ---------------------------------------------------------------------------
bool SpidrEqualisation::plotEqualisation(const std::string& filename) {

  setupStyle();
  const std::string filename0 = filename + "_0.root";
  TFile f0(filename0.c_str(), "READ");
  if (!f0.IsOpen()) {
    cerr << "[Error] Cannot open " << filename0 << ".\n";
    return false;
  }
  f0.cd();
  TH1* h0 = (TH1*)gDirectory->Get("hNoiseMean");
  h0->SetDirectory(0);

  const std::string filename15 = filename + "_15.root";
  TFile f15(filename15.c_str(), "READ");
  if (!f15.IsOpen()) {
    cerr << "[Error] Cannot open " << filename15 << ".\n";
    return false;
  }
  f15.cd();
  TH1* h15 = (TH1*)gDirectory->Get("hNoiseMean");
  h15->SetDirectory(0);

  const std::string filenameEqualised = filename + "_equalised.root";
  TFile f(filenameEqualised.c_str(), "READ");
  if (!f.IsOpen()) {
    cerr << "[Error] Cannot open " << filenameEqualised << ".\n";
    return false;
  }
  f.cd();
  TH1* h = (TH1*)gDirectory->Get("hNoiseMean");
  h->SetDirectory(0);
  
  ///*
  TCanvas c2("c2", "Fine Threshold Scan", 600, 600);
  c2.cd();
  h0->SetLineColor(kBlue + 2);
  h0->SetFillColor(kBlue + 2);
  h15->SetLineColor(kRed + 2);
  h15->SetFillColor(kRed + 2);
  h->SetLineColor(kBlack);
  h->SetFillColor(kBlack);

  h->GetXaxis()->SetTitle("THL");
  h->GetYaxis()->SetTitle("");
  
  h->Draw();
  h0->Draw("same");
  h15->Draw("same");

  const std::string image_filename = filename + ".png";
  c2.SaveAs(image_filename.c_str());
  //*/

  f.Close();
  f0.Close();
  f15.Close();

  return true;

}

bool SpidrEqualisation::maskPixels(const std::string& filename) {

  // Open the ROOT file with the post-equalisation histograms. 
  std::string rootfilename = filename + "_equalised.root";
  TFile f(rootfilename.c_str(), "READ");
  f.cd();
  TH1* hNoiseMean = (TH1*)gDirectory->Get("hNoiseMean");
  // Get the average noise level after equalisation and the sigma.
  const double meanAvg = hNoiseMean->GetMean();
  // Calculate the tolerance window.
  const double deltaAvg = m_stddev * hNoiseMean->GetRMS();
  TH1* hthrscan_1 = (TH1*)gDirectory->Get("hthrscan_1");
  hthrscan_1->SetDirectory(0);
  const double deltaSigma = 25;
  TH1* hthrscan_2 = (TH1*)gDirectory->Get("hthrscan_2");
  hthrscan_2->SetDirectory(0);
  f.Close();

  // Open the original text file with the trim DACs for each pixel.
  const std::string txtfilename = filename + ".txt"; 
  FILE* fp = fopen(txtfilename.c_str(), "r");
  if (fp == NULL) {
    cerr << "[Error] Cannot open pixel config file " << txtfilename << endl; 
    return false;
  }
  // Open a new text file.
  const std::string filenamemask = filename + "_masked.txt";
  FILE* fnew = fopen(filenamemask.c_str(), "w");
  unsigned int nMasked = 0;
  char line[256];
  int row, col, trim, masked;
  const int tp = 0;
  while (!feof(fp)) {
    if (fgets(line, 256, fp) == NULL) continue;
    if (line[0] == '#') {
      fprintf(fnew, line);
      continue;
    }
    sscanf(line, "%d %d %d %d", &col, &row, &trim, &masked);
    const unsigned int pixel = col * 256 + row;
    const double thr = hthrscan_1->GetBinContent(pixel + 1);
    const double sigma = hthrscan_2->GetBinContent(pixel + 1);
    if (fabs(thr - meanAvg) > deltaAvg || sigma > deltaSigma) {
      masked = 1;
      ++nMasked;
    } 
    fprintf(fnew, "%4d %4d %4d %4d %4d\n", col, row, trim, masked, tp);
  }
  fclose(fp);
  fclose(fnew);
  std::cout << "[Note] Masked " << nMasked << " pixels.\n";
  return true;

}

void SpidrEqualisation::printError(const std::string& header) {

  printError(header, m_ctrl->errorString());

}

void SpidrEqualisation::printError(const std::string& header,
                                   const std::string& message) const {

  std::cerr << "### ERROR ### " << header << ": " << message << std::endl; 

}

void SpidrEqualisation::printFinal(const std::string& message) {

  std::cout << std::endl << std::string(70, '=') << std::endl;
  int pad = 35 - message.length() / 2;
  if (pad < 1) pad = 1;
  std::cout << std::string(pad, ' ') << message << std::endl;
  std::cout << std::string(70, '=') << std::endl;
}

bool SpidrEqualisation::findDatFiles(const std::string& filename, 
                                     std::vector<std::string>& files) {

  files.clear();
  DIR *dir;
  if ((dir = opendir ("./")) == NULL) {
    std::cerr << "[Error] Cannot open current directory.\n";
    return false;
  }
  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    std::string f = ent->d_name;
    // Check if the filename has an extension.
    const std::size_t index = f.rfind('.');
    if (index == std::string::npos) continue;
    // Check if the extension is .dat.
    if (f.substr(index + 1) != "dat") continue;
    // Check if the file contains the requested string.
    const std::size_t found = f.find(filename);
    if (found == std::string::npos) continue;
    files.push_back(f);
  }
  closedir(dir);
  if (files.empty()) {
    std::cerr << "[Error] Could not find any files with filename containing " 
              << filename << "\n";
    return false;
  }
  return true;
  
}

bool SpidrEqualisation::loadDacs(const std::string& filename) {

  FILE *fp = fopen(filename.c_str(), "r");
  if (fp == NULL) {
    cout << "[Warning] Cannot open DAC file " << filename << "\n";
    cout << "[Warning] Applying default DAC settings\n";
    if (!m_ctrl->setDacsDflt(m_device)) printError("setDacsDflt");
    return true;
  } 
  cout << "[Note] Reading DACs from " << filename << endl;
  while (!feof(fp)) {
    char line[1024];
    if (fgets(line, 256, fp) == NULL) continue;
    if (line[0] == '#') continue;
    int dac_nr, dac_val;
    const int numpar = sscanf(line, "%d %d", &dac_nr, &dac_val);
    if (numpar == 2) {
      if (!m_ctrl->setDac(m_device, dac_nr, dac_val)) printError("setDac");
    }
  }
  fclose(fp);
  return true;
}

void SpidrEqualisation::setupStyle() {

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
  gStyle->SetPadRightMargin(0.05);
  gStyle->SetPadBottomMargin(0.16);
  gStyle->SetPadLeftMargin(0.15);
  
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
  gStyle->SetLabelOffset(0.020,"Y");

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
  gStyle->SetStatX(0.5);
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

