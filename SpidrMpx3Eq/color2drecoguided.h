#ifndef COLOR2DRECOGUIDED_H
#define COLOR2DRECOGUIDED_H

#include <QString>
#include <QMap>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/numeric/ublas/io.hpp>
//#include "storage_adaptors.hpp"


#include "mpx3gui.h"
#include "spline.h" // taken from http://kluge.in-chemnitz.de/opensource/spline/

#define __cross_section_data_relpath "./data/photon_cross_section_data_xcom/"
#define __total_attenuation_w_coh_scatt_indx    6
#define __total_attenuation_wo_coh_scatt_indx   7
#define __energy_indx                           0

class Color2DRecoGuided {


public:

    Color2DRecoGuided(Mpx3GUI *);
    Color2DRecoGuided();
    ~Color2DRecoGuided();

    int getNTotAttWCohScattDatasets(){ return _nameMap_TotAttWCohScatt.size(); }
    double getMuInterpolation(double e, int matId);

    bool isRecoArmed() { return _recoArmed; } // if the reco is ready to run

    int LoadCrossSectionData();
    int BuildAndInvertMuMatrix();

    boost::numeric::ublas::matrix<double> * getMuInvMatrix() { return _MuInv; }
    int getNMaterials() { return _nMaterials; }
    int getNEnergies() { return _nEnergies; }

    QMap<int, QString> getMaterialMap(){ return _nameMap_TotAttWCohScatt; }

private:

    Mpx3GUI * _mpx3gui;
    QString _datafiles[4] = { "H2O.txt", "Al_Z13.txt", "Ca_Z20.txt", "Cu_Z29.txt" };
    double  _densities[4] = { 1.0, 2.7, 1.55, 8.96 };

    QMap<int, QString> _nameMap_TotAttWCohScatt; // Contains the names
    QMap<int, tk::spline *> _splineMap_TotAttWCohScatt;  // Contain the interpolations
    boost::numeric::ublas::matrix<double> * _Mu = nullptr;
    boost::numeric::ublas::matrix<double> * _MuInv = nullptr;

    bool _recoArmed = false;

    int _nMaterials = -1;
    int _nEnergies = -1;

};

#endif // COLOR2DRECOGUIDED_H

