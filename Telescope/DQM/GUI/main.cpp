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


    QApplication a(argc, argv);
    MainWindow w;

    if (argc > 4) std::cout<<"Too many arguements passed! Ignoring them"<<std::endl;
    else if (argc == 2) {
        w._ops->runNumber = atoi(argv[1]);
        w.ui->RunNumBox->setValue(w._ops->runNumber);
        std::cout<<"Setting default run number from command line as: "<<w._ops->runNumber<<std::endl;
    }

    else if (argc == 3) {
        w._ops->runNumber = atoi(argv[1]);
        w.ui->RunNumBox->setValue(w._ops->runNumber);
        w._ops->PSNumFix = atoi(argv[2]);
        std::cout<<"Setting default run number from command line as: "<<w._ops->runNumber<<std::endl;
        std::cout<<"Setting PSNumFix as: "<<w._ops->PSNumFix<<std::endl;
    }

    else if (argc == 4) {
        w._ops->runNumber = atoi(argv[1]);
        w.ui->RunNumBox->setValue(w._ops->runNumber);
        w._ops->PSNumFix = atoi(argv[2]);
        w._ops->nPixHitCut = atoi(argv[3]);
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

