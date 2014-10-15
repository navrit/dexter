#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>

#include "SpidrEqualisation.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "tpx3defs.h"

using namespace std;

int main(int argc, char* argv[])
{
   // If the user didn't provide the command line argument, print an error and exit.
  if (argc <=1){
    cout << "Usage: " << argv[0] << " <spacing>" << endl;
    return(0);
  }
 
  int spacing = atoi(argv[1]);
 
  SpidrController spidrctrl(192, 168, 100, 10);

  SpidrDaq spidrdaq(&spidrctrl);
  SpidrEqualisation equalisation(&spidrctrl, &spidrdaq);

  equalisation.enablePixelMask(true, 4);
  equalisation.setFileName("test");
  equalisation.setIkrum(10);
  equalisation.setThlCoarse(6);
  equalisation.setTriggerLength(50);
  equalisation.setTrigger(4, 100, 1);
  equalisation.setThlScan(1, 511, 1);
  equalisation.setSpacing(spacing);

  equalisation.equalise();

  return 0;
}
