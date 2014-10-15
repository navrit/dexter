#ifndef SPIDREQUALISATION_H
#define SPIDREQUALISATION_H

#ifdef WIN32
 // On Windows differentiate between building the DLL or using it
 #include "stdint.h"
 #ifdef MY_LIB_EXPORT
 #define MY_LIB_API __declspec(dllexport)
 #else
 #define MY_LIB_API __declspec(dllimport)
 #endif
#else
 // Linux
 #include </usr/include/stdint.h>
 #define MY_LIB_API
#endif // WIN32

#include <vector>
#include <string>

typedef uint32_t u32;
typedef uint8_t u8;

class SpidrController;
class SpidrDaq;

class MY_LIB_API SpidrEqualisation {
 public:
  // Constructor
  SpidrEqualisation(SpidrController* spidrctrl, SpidrDaq* spidrdaq);
  // Destructor
  ~SpidrEqualisation() {}

  // Run the full equalisation.
  bool equalise(const bool scan0 = true, const bool scan15 = true,
                const bool analyse0 = true, const bool analyse15 = true,
                const bool extract = true,
                const bool scanFinal = true, const bool analyseFinal = true,
                const bool mask = true, const bool plot = true);
  // Run a subset of the steps in the full equalisation.
  bool testEqualisation();
  bool quickEqualisation();

  // Load trim values already created.
  bool loadEqualisation(const std::string& filename, const bool loadmask);
  void enablePixelMask(const bool pixelmask, const unsigned int stddev) {
    m_pixelmask = pixelmask;
    m_stddev = stddev;
  }

  // Set the name of the DAC file to be loaded during configuration.
  void setDacFile(const std::string& filename) { m_dacfilename = filename; }
  // Set the shutter trigger configuration.
  void setTrigger(const unsigned int trig_mode, 
                  const unsigned int trig_freq_hz, 
                  const unsigned nr_of_trigs) {
    m_trigmode = trig_mode;
    m_trig_freq_hz = trig_freq_hz;
    m_nr_of_trigs = nr_of_trigs;
  }
  // Set the shutter length (in microseconds).
  void setTriggerLength(const unsigned int l) { m_trig_length_us = l; }
  // Set the output filename (without file extension).
  void setFileName(const std::string filename) { m_filename = filename; }
  void setIkrum(const unsigned int ikrum) { m_ikrum = ikrum; }
  void setThlCoarse(const unsigned int thlcoarse) { m_thlcoarse = thlcoarse; }
  
  // Set the range and step size of the threshold scan.
  void setThlScan(const unsigned int thl_min, const unsigned int thl_max, 
                  const unsigned int thl_step) {
    m_thlmin = thl_min;
    m_thlmax = thl_max;
    m_thlstep = thl_step;
  } 
  // Set the spacing. 
  void setSpacing(const unsigned int spacing) { m_spacing = spacing; }
  // Set the polarity (electron-collecting or hole-collecting mode).
  void setPolarityElectrons() { m_eminus = true; }
  void setPolarityHoles() { m_eminus = false; } 

 private:

  SpidrController* m_ctrl;
  SpidrDaq* m_daq;

  // Device number
  int m_device;
  bool m_pixelmask;
  unsigned int m_stddev; 
  unsigned int m_spacing;
  int m_thlmin;
  int m_thlmax;
  int m_thlstep;
  unsigned int m_thlcoarse;
  unsigned int m_ikrum;
  unsigned int m_trig_length_us;
  unsigned int m_trigmode;
  unsigned int m_trig_freq_hz;
  unsigned int m_nr_of_trigs;
  // Name of the output file(s).
  std::string m_filename;
  // Name of the DAC file to be loaded.
  std::string m_dacfilename;
  // Flag whether to use electron or hole collecting mode.
  bool m_eminus;

  bool checkCommunication();
  bool setConfiguration();

  bool takeData(const std::string& filename);
  bool analyseData(const std::string& filename);
  // Create file with trim values.
  bool extractPars(const std::string& filename);
  // Mask pixels.
  bool maskPixels(const std::string& filename);
  bool plotEqualisation(const std::string& filename);

  bool setTHLTrim(const std::string& filename);
  bool setTHLTrimALL(const unsigned int trim);

  void printError(const std::string& header);
  void printError(const std::string& header, const std::string& message) const;
  void printFinal(const std::string& message);

  bool findDatFiles(const std::string& filename,
                    std::vector<std::string>& files);

  bool loadDacs(const std::string& filename);

};

#endif
