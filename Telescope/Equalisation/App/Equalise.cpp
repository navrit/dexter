#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <iostream>
#include "SpidrEqualisation.h"
#include "SpidrController.h"
#include "SpidrDaq.h"

using namespace std;

int main(int argc, char* argv[]) {

  if (argc == 1 || argc > 3) {
    cout << "usage: Equalise <-q> <filename prefix>" << endl;
    cout << "       Equalise <-f> <filename prefix>" << endl;
    cout << "       Equalise <-l> <trimdacs filename>" << endl;
    cout << "       Equalise <-t> <filename prefix>" << endl;
    cout << "       Equalise <-c> <filename prefix>" << endl;
    cout << "       -q starts quick equalisation. Does not check equalised "
            "matrix" << endl;
    cout << "       -f starts full equalisation with <filename prefix>" << endl;
    cout << "       -l load equalisation from <trimdacs filename>" << endl;
    cout << "       -t test the equalised matrix" << endl;
    cout << "       -q scan the coarse threshold" << endl;
    return -1;
  }

  SpidrController spidrctrl(192, 168, 100, 10);
  SpidrDaq spidrdaq(&spidrctrl);
  SpidrEqualisation equalisation(&spidrctrl, &spidrdaq);

  extern char* optarg;
  int spacing = 4;
  std::string fileprefix = "";
  char c = getopt(argc, argv, "f:q:l:t:c:");
  switch (c) {
    case 'f':
      fileprefix = optarg;
      cout << " Enter pixels spacing: ";
      cin >> spacing;
      equalisation.enablePixelMask(true, 4);
      equalisation.setFileName(fileprefix);
      equalisation.setIkrum(10);
      equalisation.setThlCoarse(6);
      equalisation.setTriggerLength(50);
      equalisation.setTrigger(4, 100, 1);
      equalisation.setThlScan(1, 512, 1);
      equalisation.setSpacing(spacing);
      equalisation.equalise();
      // break;
      return 0;

    case 'q':
      fileprefix = optarg;
      cout << " Enter pixels spacing: ";
      cin >> spacing;
      equalisation.setFileName(fileprefix);
      equalisation.setIkrum(10);
      equalisation.setThlCoarse(6);
      equalisation.setTriggerLength(50);
      equalisation.setTrigger(4, 100, 1);
      equalisation.setThlScan(1, 512, 1);
      equalisation.setSpacing(spacing);
      equalisation.quickEqualisation();
      // break;
      return 0;

    case 'l':
      fileprefix = optarg;
      equalisation.loadEqualisation(fileprefix, 1);
      // break;
      return 0;

    case 't':
      fileprefix = optarg;
      cout << " Enter pixels spacing: ";
      cin >> spacing;
      equalisation.enablePixelMask(true, 4);
      equalisation.setFileName(fileprefix);
      equalisation.setIkrum(10);
      equalisation.setThlCoarse(6);
      equalisation.setTriggerLength(50);
      equalisation.setTrigger(4, 100, 1);
      equalisation.setThlScan(1, 512, 1);
      equalisation.setSpacing(spacing);
      equalisation.testEqualisation();
      // break;
      return 0;

    case 'c':
      fileprefix = optarg;
      equalisation.setFileName(fileprefix);
      equalisation.setIkrum(10);
      equalisation.setTriggerLength(50);
      equalisation.setTrigger(4, 100, 1);
      equalisation.setThlScan(1, 512, 1);
      equalisation.setSpacing(1);
      equalisation.findCoarse();
    default:
      break;
  }
}
