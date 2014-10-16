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
#include "TFile.h"
#include "SpidrEqualisation.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "tpx3defs.h"

using namespace std;

int main(int argc, char* argv[])
{
  SpidrController spidrctrl(192, 168, 100, 10);
  SpidrDaq spidrdaq(&spidrctrl);
  SpidrEqualisation equalisation(&spidrctrl, &spidrdaq);

  const string fileprefix;
  extern char *optarg;
  int spacing;


  if (argc ==1 || argc > 3) {
	  cout << "usage: Equalise <-q> <filename prefix>" << endl;
	  cout << "       Equalise <-f> <filename prefix>" << endl;
	  cout << "       Equalise <-l> <trimdacs filename>" << endl;
	  cout << "       Equalise <-t> <filename prefix>" << endl;
	  cout << "       -q starts quick equalisation. Does not check equalised matrix" << endl;
	  cout << "       -f starts full equalisation with <filename prefix>" << endl;
	  cout << "       -l load equalisation from <trimdasc filename>" << endl;
	  cout << "       -t test the equalised matrix" << endl;
	  return -1;
  }

  char c = getopt(argc, argv, "f:q:l:t:");
  switch (c) {
	  case 'f': sscanf( optarg, "%s", fileprefix.c_str());
		    cout << " Enter pixels spacing: "; cin >> spacing;
		    equalisation.enablePixelMask(true, 4);
		    equalisation.setFileName(fileprefix);
		    equalisation.setIkrum(10);
		    equalisation.setThlCoarse(6);
		    equalisation.setTriggerLength(50);
		    equalisation.setTrigger(4, 100, 1);
		    equalisation.setThlScan(1, 512, 1);
		    equalisation.setSpacing(spacing);
		    equalisation.equalise();
		    //break;
		    return 0;

	  case 'q': sscanf( optarg, "%s", fileprefix.c_str());
		    cout << " Enter pixels spacing: "; cin >> spacing;
                    equalisation.setFileName(fileprefix);
                    equalisation.setIkrum(10);
                    equalisation.setThlCoarse(6);
                    equalisation.setTriggerLength(50);
                    equalisation.setTrigger(4, 100, 1);
                    equalisation.setThlScan(1, 512, 1);
                    equalisation.setSpacing(spacing);
                    equalisation.quickEqualisation();
                    //break;
                    return 0;

	  case 'l': sscanf( optarg, "%s", fileprefix.c_str());
                    equalisation.loadEqualisation(fileprefix, 1);
                    //break;
                    return 0;

	  case 't': sscanf( optarg, "%s", fileprefix.c_str());
		    cout << " Enter pixels spacing: "; cin >> spacing;
                    equalisation.enablePixelMask(true, 4);
                    equalisation.setFileName(fileprefix);
                    equalisation.setIkrum(10);
                    equalisation.setThlCoarse(6);
                    equalisation.setTriggerLength(50);
                    equalisation.setTrigger(4, 100, 1);
                    equalisation.setThlScan(1, 512, 1);
                    equalisation.setSpacing(spacing);
                    equalisation.testEqualisation();
                    //break;
                    return 0;
	  //default : run_control = false;
	//	    break;

  }
}
