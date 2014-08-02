//(../GUI/)main.cpp - the main when using the GUI (slightly different to usual main).

//Author: Dan Saunders
//Date created: 10/01/13
//Last modified: 20/01/13


#include "DQM_widg.h"
#include <QApplication>
#include <iostream>


int main(int argc, char *argv[]) {
    std::cout<<"Running GUI Main."<<std::endl;
    //Since the DQM is ran during runtime when using the GUI (as we like editing
    //the options as we go along), main just needs to open the GUI window.

    int r;
    if (argc == 2 || argc == 3) r = atoi(argv[1]);
    else r = 0;
    QApplication a(argc, argv);
    MainWindow w(r);

    if (argc > 3) std::cout<<"Too many arguements passed! Ignoring them"<<std::endl;
    else if (argc == 2) {
        std::cout<<"Setting default run number from command line as: "<<w._ops->runNumber<<std::endl;
    }

    else if (argc == 3) {
        w._ops->nPixHitCut = atoi(argv[2]);
        std::cout<<"Setting default run number from command line as: "<<w._ops->runNumber<<std::endl;
        std::cout<<"Setting PSNumFix as: "<<w._ops->PSNumFix<<std::endl;
        std::cout<<"Setting nPixHitCut as: "<<w._ops->nPixHitCut<<std::endl;
    }

    //Setup root and Qt.
    gErrorIgnoreLevel = 5000;
    #if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QApplication::setGraphicsSystem("raster");
    #endif


    //Show the window.
    w.show();

    //When finished...
    return a.exec();
}

