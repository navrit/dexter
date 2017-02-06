
#include "color2drecoguided.h"
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QList>

using namespace boost::numeric::ublas;
using namespace std;

// prototype
template<class T> bool InvertMatrix(const matrix<T>& input, matrix<T>& inverse);

Color2DRecoGuided::Color2DRecoGuided(Mpx3GUI * mg) {
    _mpx3gui = mg;
    _datafiles[0] = txt_H20 ;
    _datafiles[1] = txt_Al_Z13;
    _datafiles[2] = txt_Ca_Z20;
    _datafiles[3] = txt_Au_Z79;//txt_Cu_Z29;

    _densities[0] = 1.0;
    _densities[1] = 2.7;
    _densities[2] = 1.55;
    _densities[3] = 19.32;//8.96;
}

Color2DRecoGuided::Color2DRecoGuided() {

    _datafiles[0] = txt_H20 ;
    _datafiles[1] = txt_Al_Z13;
    _datafiles[2] = txt_Ca_Z20;
    _datafiles[3] = txt_Au_Z79;//txt_Cu_Z29;

    _densities[0] = 1.0;
    _densities[1] = 2.7;
    _densities[2] = 1.55;
    _densities[3] = 19.32;//8.96;
}



Color2DRecoGuided::~Color2DRecoGuided() {

    if( _Mu ) delete _Mu;
    if( _MuInv ) delete _MuInv;

    QList<tk::spline *> keys = _splineMap_TotAttWCohScatt.values();
    QList<tk::spline *>::iterator i  = keys.begin();
    QList<tk::spline *>::iterator iE = keys.end();
    for ( ; i != iE ; i++ ) delete (*i);

}

/**
 * @brief Color2DRecoGuided::BuildAndInvertMuMatrix
 * @return
 */
int Color2DRecoGuided::BuildAndInvertMuMatrix() {

    // The matrix Mu_{i}^{j}
    // i: energies
    // j: materials

    // Get the energies
    //QList<int> thresholds = _mpx3gui->getConfig()->getThresholds();
    // To obtain the energies per thresholds we need a calibration, TODO !
    // Probably available through the Config ?
    QList<double> energies;
    //    energies.append(  3.6E-3 ); // !!! NIST data comes in MeV !!!
    //    energies.append(  10.E-3 ); //
    //    energies.append(  15.E-3 ); //
    //    energies.append(  20.E-3 ); //

    //energies.append( 4.8E-3 ); // 0
    //energies.append( 7.2E-3 ); // 2
    //energies.append( 9.6E-3 ); // 4
    //energies.append( 11.9E-3 );  // 6

          energies.append(  4.8E-3 );
          energies.append(  10.0E-3 );
          energies.append( 15.0E-3 );
          energies.append( 20.0E-3 );

    // Materials
    _nMaterials = getNTotAttWCohScattDatasets();
    _nEnergies = energies.size();

    // Check how we make a square matrix here
    if ( _nEnergies != _nMaterials ) {
        QMessageBox::information(0, "Color2DRecoGuided", "The reconstruction needs as many materials as energies.");
        return EXIT_FAILURE;
    }

    // I can now make a square matrix
    // Now build the matrix using the initial values
    //matrix<double> A(nEnergies,nMaterials), Z(nEnergies,nMaterials);
    _Mu    = new matrix<double>(_nEnergies, _nMaterials);
    _MuInv = new matrix<double>(_nEnergies, _nMaterials);

    double e = 0.;
    for(int i = 0 ; i < _nEnergies ; i++) {
        // energy
        e = energies[i];
        for(int j = 0 ; j < _nMaterials ; j++) {

            // material --> _nameMap_TotAttWCohScatt[j]
            // Get the mu for this particular material and energy
            (*_Mu)(i,j) = getMuInterpolation( e, j ) * _densities[j];

            // Run a little check in the Mu matrix.  If there is any negative value something went wrong
            if ( (*_Mu)(i,j) < 0. ) {
                QMessageBox::information(0, "Color2DRecoGuided", "Negative values (from interpolation) input as attenuation coefficients.");
                return EXIT_FAILURE;
            }
        }
    }

    // Invert
    InvertMatrix(*_Mu, *_MuInv);
    cout << "Mu = " << *_Mu << endl << "MuInv = " << *_MuInv << endl;

    return EXIT_SUCCESS;
}


/**
 * @brief Color2DRecoGuided::LoadCrossSectionData
 * Load cross section values from XCOM data
 * @return
 */
int Color2DRecoGuided::LoadCrossSectionData() {

    // From the README file
    // Photon cross section data taken from http://www.nist.gov/pml/data/xcom/
    // - The last two columns correpond to the total attenuation
    // Photon   	Coherent	Incoher.	Photoel.	Nuclear 	Electron	Tot. w/ 	Tot. wo/
    // Energy   	Scatter.	Scatter.	Absorb. 	Pr. Prd.	Pr. Prd.	Coherent	Coherent
    // [MeV]		[cm^2/g]	[cm^2/g]	[cm^2/g]	[cm^2/g]	[cm^2/g]	[cm^2/g]	[cm^2/g]

    int nfiles = sizeof( _datafiles ) / sizeof( QString );

    for ( int fileId = 0 ; fileId < nfiles ; fileId++ ) { // Loop over files

        QString fullfn = __cross_section_data_relpath + _datafiles[fileId];
        QFile file( fullfn );
        qDebug() << "[Color2DRecoGuided] Opening " << fullfn;
        if ( !file.open(QIODevice::ReadOnly) ) {
            QMessageBox::information(0,"File error","Can not open file\n"+fullfn);
            return EXIT_FAILURE;
        }

        // Datasetname
        QString rawfn = _datafiles[fileId];
        rawfn.remove(".txt", Qt::CaseInsensitive);

        // Extract the info
        QTextStream in(&file);

        // I will use the vector to pass to the spline object
        QVector<double> e;
        QVector<double> mu;

        while ( !in.atEnd() ) {

            QString line = in.readLine();
            QStringList fields = line.split("\t");

            e.push_back( fields.at(__energy_indx).toDouble() );
            mu.push_back( fields.at(__total_attenuation_w_coh_scatt_indx).toDouble() );

        }

        file.close();

        // Use as keys are the names are extracted from the filename
        _nameMap_TotAttWCohScatt[fileId] = rawfn;
        qDebug() << "                   " << fileId << " --> " << rawfn;

        // Build the spline here
        _splineMap_TotAttWCohScatt[fileId] = new tk::spline;
        // I select linear interpolation.  I don't have enought data to do cubic spline.  It may give negative values.
        _splineMap_TotAttWCohScatt[fileId]->set_points( e.toStdVector(), mu.toStdVector(), false );

    }


    return EXIT_SUCCESS;
}



double Color2DRecoGuided::getMuInterpolation(double e, int matId) {

    return (*_splineMap_TotAttWCohScatt[matId])(e);
}

/**
 Matrix inversion routine.
 Uses lu_factorize and lu_substitute in uBLAS to invert a matrix
*/
template<class T>
bool InvertMatrix(const matrix<T>& input, matrix<T>& inverse)
{
    typedef permutation_matrix<std::size_t> pmatrix;

    // create a working copy of the input
    matrix<T> A(input);

    // create a permutation matrix for the LU-factorization
    pmatrix pm(A.size1());

    // perform LU-factorization // This seems to return !=0 if the matrix is singular and hence impossible to invert
    int res = lu_factorize(A, pm);
    if (res != 0)
        return false;

    // create identity matrix of "inverse"
    inverse.assign(identity_matrix<T> (A.size1()));

    // backsubstitute to get the inverse
    lu_substitute(A, pm, inverse);

    return true;
}
