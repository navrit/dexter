#include "dataset.h"
#include "mpx3gui.h"
#include <QDataStream>
#include <QDebug>

#include <iostream>
#include <iomanip>

using namespace std;

#include "ui_qcstmglvisualization.h"

Dataset::Dataset(int x, int y, int framesPerLayer)
{
    m_nx = x; m_ny = y;
    m_nFrames = 0;
    setFramesPerLayer(framesPerLayer);

    obCorrection = 0x0;

    reloadScores();

}

Dataset::Dataset() : Dataset(1,1,1) {
    reloadScores();
}

Dataset::~Dataset()
{
    if(obCorrection) delete obCorrection;
    clear();
}

void Dataset::loadCorrection(QByteArray serialized) {
    delete obCorrection;
    obCorrection  = new Dataset(0,0,0);
    obCorrection->fromByteArray(serialized);//TODO: add error checking on correction to see if it is relevant to the data.
}

void Dataset::reloadScores() {
    _scores.packetsLost = 0;
}

int64_t Dataset::getTotal(int threshold){
    int index = thresholdToIndex(threshold);
    if(index == -1)
        return 0;
    int64_t count = 0;
    for(int j = 0; j < getPixelsPerLayer(); j++)
        count += m_layers[index][j];
    return count;
}

uint64_t Dataset::getActivePixels(int threshold){
    int index = thresholdToIndex(threshold);
    if(index == -1)
        return 0;
    uint64_t count  =0;
    for(int j = 0; j <getPixelsPerLayer(); j++){
        if(0 != m_layers[index][j])
            count++;
    }
    return count;
}

Dataset::Dataset( const Dataset& other ): m_boundingBox(other.m_boundingBox), m_frameLayouts(other.m_frameLayouts), m_frameOrientation(other.m_frameOrientation), m_thresholdsToIndices(other.m_thresholdsToIndices), m_layers(other.m_layers){
    // copy the dimensions
    m_nx = other.x(); m_ny = other.y();
    // And copy the layers
    m_nFrames = other.getFrameCount();
    for(int i = 0; i < m_layers.size(); i++){
        m_layers[i] = new int[getPixelsPerLayer()];
        for(int j = 0; j < getPixelsPerLayer(); j++)
            m_layers[i][j] = other.m_layers[i][j];
    }
    obCorrection = 0x0;
}

Dataset& Dataset::operator =( const Dataset& rhs){
    Dataset copy(rhs);
    std::swap(this->m_layers, copy.m_layers);
    return *this;
}

void Dataset::zero(){
    for(int i = 0; i < m_layers.size(); i++){
        for(int j = 0; j < getPixelsPerLayer(); j++)
            m_layers[i][j] = 0;
    }
}

int Dataset::getLayerIndex(int threshold){
    int layerIndex = thresholdToIndex(threshold);
    if(layerIndex == -1)
        layerIndex = newLayer(threshold);
    return layerIndex;
}

QByteArray Dataset::toByteArray() {

    QByteArray ret(0);
    ret += QByteArray::fromRawData((const char*)&m_nx, (int)sizeof(m_nx));
    ret += QByteArray::fromRawData((const char*)&m_ny, (int)sizeof(m_ny));
    ret += QByteArray::fromRawData((const char*)&m_nFrames, (int)sizeof(m_nFrames));
    int layerCount = m_layers.size();
    ret += QByteArray::fromRawData((const char*)&layerCount, (int)sizeof(layerCount));
    ret += QByteArray::fromRawData((const char*)m_frameLayouts.data(),(int)(m_nFrames*sizeof(*m_frameLayouts.data())));
    ret += QByteArray::fromRawData((const char*)m_frameOrientation.data(), (int)(m_nFrames*sizeof(*m_frameOrientation.data())));
    QList<int> keys = m_thresholdsToIndices.keys();
    ret += QByteArray::fromRawData((const char*)keys.toVector().data(),(int)(keys.size()*sizeof(int))); //thresholds
    for(int i = 0; i < keys.length(); i++)
        ret += QByteArray::fromRawData((const char*)this->getLayer(keys[i]), (int)(sizeof(float)*getLayerSize()));

    return ret;
}

/**
 * This serializes all the layers in a single vector of integers.
 */
QVector<int> Dataset::toQVector() {

    QVector<int> tovec;

    QList<int> keys = m_thresholdsToIndices.keys();
    for(int i = 0; i < keys.length(); i++) {
        int * layer = this->getLayer(keys[i]);
        for ( uint64_t j = 0 ; j < this->getPixelsPerLayer() ; j++) {
            tovec.append( layer[j] );
        }
    }

    return tovec;
}

void Dataset::calcBasicStats(QPoint pixel_init, QPoint pixel_end) {

    QList<int> keys = m_thresholdsToIndices.keys();

    QSize isize = QSize(computeBoundingBox().size().width()*this->x(), computeBoundingBox().size().height()*this->y());


    // Region of interest
    QRectF RoI;
    RoI.setRect(pixel_init.x(), pixel_init.y(), pixel_end.x() - pixel_init.x(),  pixel_end.y() - pixel_init.y() );

    cout << "-- Basic stats -- ";
    for(int i = 0; i < keys.length(); i++) {
        cout << "\t" << keys[i];
    }
    cout << endl;

    // Mean
    vector<double> mean_v;
    cout << "   mean\t\t";
    for(int i = 0; i < keys.length(); i++) {

        int* currentLayer = getLayer(keys[i]);

        // Calc mean on the interesting pixels
        double mean = 0.;
        double nMean = 0.;
        for(int j = 0; j < getPixelsPerLayer(); j++) {
            // See if the pixel is inside the region
            QPointF pix = XtoXY(j, isize.width());
            if ( RoI.contains( pix ) ) {
                mean += currentLayer[j];
                nMean++;
            }
        }
        if(nMean != 0) mean /= nMean;

        //cout.precision(1);
        cout << "\t" << mean;
        mean_v.push_back( mean ); // save the man values for calculation of stdev

    }
    cout << endl;

    // Standard Deviation
    cout << "   stdev\t";
    for(int i = 0; i < keys.length(); i++) {

        int* currentLayer = getLayer(keys[i]);
        double stdev = 0.;
        double nMean = 0.;
        for(int j = 0; j < getPixelsPerLayer(); j++) {
            // See if the pixel is inside the region
            QPointF pix = XtoXY(j, isize.width());
            if ( RoI.contains( pix ) ) {
                stdev += (currentLayer[j] - mean_v[i])*(currentLayer[j] - mean_v[i]);
                nMean++;
            }
        }
        if ( stdev != 0 ) stdev /= nMean;
        stdev = sqrt(stdev);
        //cout.precision(1);
        cout << "\t" << stdev;
    }

    cout << endl;

}

QPointF Dataset::XtoXY(int X, int dimX){
    return QPointF(X % dimX, X/dimX);
}

bool Dataset::isBorderPixel(int p, QSize isize) {

    if (
            p < isize.width() 	// lower edge: 0 --> width-1
            ||
            p >= ( isize.width()*isize.height() - isize.width() ) // upper edge: width*height-width --> width*height
            ||
            ( ( p % (isize.width()-1) ) == 0 ) // right edge
            ||
            ( ( p %  isize.width()    ) == 0 ) // left edge
            ) return true;

    return false;
}

bool Dataset::isBorderPixel(int x, int y, QSize isize) {

    if ( y <= 0 ) return true;					// lower edge
    if ( y >= isize.height() - 1 ) return true;	// upper edge
    if ( x <= 0 ) return true;					// left edge
    if ( x >= isize.width()  - 1 ) return true;	// right edge

    return false;
}

double Dataset::calcPadMean(int thlkey, QSize isize) {

    double mean = 0.0;
    int meanCntr = 0;

    for(int y = 0 ; y < isize.height() ; y++) {

        for(int x = 0 ; x < isize.width() ; x++) {

            // Around pixel (29,30)(dead) for info
            if ( x == 29 && y == 30 ) {
                cout
                        << sample(x, y+1, thlkey)
                        << ", " << sample(x+1, y, thlkey)
                        << ", " << sample(x, y-1, thlkey)
                        << ", " << sample(x-1, y, thlkey)
                        << endl;
            }

            // sample and skip if it's a dead pixel
            int sampling = sample(x, y, thlkey);
            if ( sampling <= 0 ) continue;

            // skip borders
            if ( isBorderPixel(x,y, isize) ) continue;

            // calculate the mean
            mean += sampling;
            meanCntr++;


        }

    }
    // no borders !
    mean /= (double)meanCntr;

    return mean;
}

void Dataset::applyHighPixelsInterpolation(double meanMultiplier, QMap<int, double> meanvals) {

    QList<int> keys = m_thresholdsToIndices.keys();
    // computeBoundingBox().size().width() 	--> gives the number of chips
    // this->x() 							--> gives the number of pixels per chip
    QSize isize = QSize( computeBoundingBox().size().width()*this->x(), computeBoundingBox().size().height()*this->y() );

    for(int i = 0; i < keys.length(); i++) {

        double mean = meanvals[ keys[i] ];


        for(int y = 0 ; y < isize.height() ; y++) {

            for(int x = 0 ; x < isize.width() ; x++) {

                // If a pixel is higher than meanMultiplier times the mean
                if ( sample(x, y, keys[i]) > qRound(meanMultiplier * mean) ) {

                    // Then count how many pixels around this pixel have a value lower than its own value

                    // Check first how many of it's neighbors are active beyond the multiplier * mean
                    map< pair<int, int>, int > activeBellow = activeNeighbors(x, y, keys[i], isize, __less, qRound(meanMultiplier * mean) );

                    // Request at least three active neighbors to consider filling up by averaging.
                    // In the dead-pixel correction we request only 2. We may end up eating corners
                    //   of a distinctive structure which is an actual part of the image.
                    // This means we request a hot pixel almost fully isolated.
                    if ( activeBellow.size() >= 3 ) {
                        setPixel(x, y, keys[i], averageValues( activeBellow ) );
                    }

                }

            } // x

        } // y

    } // layers

}

void Dataset::applyDeadPixelsInterpolation(double meanMultiplier, QMap<int, double> meanvals) {

    // The keys are the thresholds
    QList<int> keys = m_thresholdsToIndices.keys();

    QSize isize = QSize(computeBoundingBox().size().width()*this->x(), computeBoundingBox().size().height()*this->y());

    for(int i = 0; i < keys.length(); i++) {

        double mean = meanvals[ keys[i] ];


        for(int y = 0 ; y < isize.height() ; y++) {

            for(int x = 0 ; x < isize.width() ; x++) {

                // is dead pixel
                if ( sample(x, y, keys[i]) == 0 ) {

                    // Check first how many of its neighbors are active
                    map< pair<int, int>, int > actives = activeNeighbors(x, y, keys[i], isize, __bigger, 0);
                    // And check how many of its neighbors are not noisy
                    map< pair<int, int>, int > notNoisy = activeNeighbors(x, y, keys[i], isize, __less, qRound(meanMultiplier * mean) );
                    // The average will be made on the intersection of actives and notNoisy
                    vector<int> toAverage = getIntersection(actives, notNoisy);

                    // Request at least two active neighbors to consider filling up by averaging
                    if ( toAverage.size() >= 2 ) {
                        // It is a problem when a dead pixel is close to a noisy pixel.
                        // The noisy pixel should not be taken into account.
                        setPixel(x, y, keys[i], vectorAverage( toAverage ) );
                    }

                }

            } // x
        } // y

    } // layers


}

void Dataset::DumpSmallMap(map< pair<int, int>, int > m1) {

    map< pair<int, int>, int >::iterator i = m1.begin();
    map< pair<int, int>, int >::iterator iE = m1.end();
    for ( ; i != iE ; i++ ) {
        cout << " | " << (*i).first.first << "," << (*i).first.second << "=" << (*i).second;
    }
    //cout << endl;
}

vector<int> Dataset::getIntersection(map< pair<int, int>, int > m1, map< pair<int, int>, int > m2) {

    vector<int> intersection;

    // loop over m1 and find the matches in m2
    map< pair<int, int>, int >::iterator i = m1.begin();
    map< pair<int, int>, int >::iterator iE = m1.end();
    map< pair<int, int>, int >::iterator f;

    for ( ; i != iE ; i++ ) {

        // If match found
        if ( m2.find( (*i).first ) != m2.end() ) intersection.push_back( (*i).second );

    }

    return intersection;
}

int Dataset::averageValues(map< pair<int, int>, int > m1) {

    double av = 0;

    map< pair<int, int>, int >::iterator i = m1.begin();
    map< pair<int, int>, int >::iterator iE = m1.end();

    for ( ; i != iE ; i++ ) {
        av += (*i).second;
    }
    av /= ( (double)(m1.size()) );

    return qRound(av);
}

int Dataset::vectorAverage(vector<int> v) {

    double av = 0;
    vector<int>::iterator i  = v.begin();
    vector<int>::iterator iE = v.end();
    for ( ; i != iE ; i++ ) {
        av += *i;
    }
    av /= ( (double)(v.size()) );

    return qRound(av);
}


void Dataset::appendSelection(int x, int y, int thl, int compareto, comp c, map< pair<int, int>, int > & activeNeighbors) {

    int val = sample(x, y, thl);

    switch ( c ) {
    case __less:
        if ( val < compareto ) {
            activeNeighbors[ make_pair(x,y) ] = val;
        }
        break;
    case __lesseq:
        if ( val <= compareto ) {
            activeNeighbors[ make_pair(x,y) ] = val;
        }
        break;
    case __equal:
        if ( val == compareto ) {
            activeNeighbors[ make_pair(x,y) ] = val;
        }
        break;
    case __bigger:
        if ( val > compareto ) {
            activeNeighbors[ make_pair(x,y) ] = val;
        }
        break;
    case __biggereq:
        if ( val >= compareto ) {
            activeNeighbors[ make_pair(x,y) ] = val;
        }
        break;
    default:
        if ( val > compareto ) {		// __bigger
            activeNeighbors[ make_pair(x,y) ] = val;
        }
        break;
    }

}

map< pair<int, int>, int > Dataset::activeNeighbors(int x, int y, int thl, QSize isize, comp c, int activeValue) {

    map< pair<int, int>, int > activeNeighbors;

    // If the pixel is not in the border.  Most probable.
    if ( ! isBorderPixel(x, y, isize) ) {

        appendSelection( x, y+1, thl, activeValue, c, activeNeighbors );
        appendSelection( x, y-1, thl, activeValue, c, activeNeighbors );
        appendSelection( x+1, y, thl, activeValue, c, activeNeighbors );
        appendSelection( x-1, y, thl, activeValue, c, activeNeighbors );
        //if ( sample(x, y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x, y+1, thl) );
        //if ( sample(x, y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x, y-1, thl) );
        //if ( sample(x+1, y, thl) > activeValue ) activeNeighbors.push_back( sample(x+1, y, thl) );
        //if ( sample(x-1, y, thl) > activeValue ) activeNeighbors.push_back( sample(x-1, y, thl) );
        return activeNeighbors;

    }
    // Bottom edge
    if ( y == 0 && x > 0 && x < (isize.width()-1) ) {

        appendSelection( x-1,   y, thl, activeValue, c, activeNeighbors );
        appendSelection( x  , y+1, thl, activeValue, c, activeNeighbors );
        appendSelection( x+1,   y, thl, activeValue, c, activeNeighbors );
        //if ( sample(x-1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x-1,   y, thl) );
        //if ( sample(x  , y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y+1, thl) );
        //if ( sample(x+1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x+1,   y, thl) );
        return activeNeighbors;
    }
    // Left edge
    if (  y > 0 && y < (isize.height()-1) && x == 0 ) {
        appendSelection( x  , y+1, thl, activeValue, c, activeNeighbors );
        appendSelection( x+1,   y, thl, activeValue, c, activeNeighbors );
        appendSelection( x  , y-1, thl, activeValue, c, activeNeighbors );

        //if ( sample(x  , y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y+1, thl) );
        //if ( sample(x+1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x+1,   y, thl) );
        //if ( sample(x  , y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y-1, thl) );
        return activeNeighbors;
    }
    // Top edge
    if (  y == (isize.height()-1) && x > 0 && x < (isize.width()-1) ) {
        appendSelection( x-1,   y, thl, activeValue, c, activeNeighbors );
        appendSelection( x+1,   y, thl, activeValue, c, activeNeighbors );
        appendSelection( x  , y-1, thl, activeValue, c, activeNeighbors );
        //if ( sample(x-1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x-1,   y, thl) );
        //if ( sample(x+1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x+1,   y, thl) );
        //if ( sample(x  , y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y-1, thl) );
        return activeNeighbors;
    }
    // Right edge
    if (  x == (isize.width()-1) && y > 0 && y < (isize.height()-1) ) {
        appendSelection( x  , y+1, thl, activeValue, c, activeNeighbors );
        appendSelection( x  , y-1, thl, activeValue, c, activeNeighbors );
        appendSelection( x-1,   y, thl, activeValue, c, activeNeighbors );
        //if ( sample(x  , y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y+1, thl) );
        //if ( sample(x  , y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y-1, thl) );
        //if ( sample(x-1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x-1,   y, thl) );
        return activeNeighbors;
    }

    // Four corners
    if (  (x % (isize.width()-1) == 0)  &&  (y % (isize.height()-1) == 0)  ) {

        if ( x == 0 && y == 0) {  // left bottom
            appendSelection( x+1,   y, thl, activeValue, c, activeNeighbors );
            appendSelection( x,   y+1, thl, activeValue, c, activeNeighbors );
            appendSelection( x+1, y+1, thl, activeValue, c, activeNeighbors );
            //if ( sample(x+1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x+1,   y, thl) );
            //if ( sample(x,   y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x,   y+1, thl) );
            //if ( sample(x+1, y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x+1, y+1, thl) );
        } else if ( x == 0 && y == (isize.height()-1) ) {  // left top
            appendSelection( x+1,   y, thl, activeValue, c, activeNeighbors );
            appendSelection( x,   y-1, thl, activeValue, c, activeNeighbors );
            appendSelection( x+1, y-1, thl, activeValue, c, activeNeighbors );
            //if ( sample(x+1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x+1,   y, thl) );
            //if ( sample(x,   y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x,   y-1, thl) );
            //if ( sample(x+1, y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x+1, y-1, thl) );
        } else if ( x == (isize.width()-1) && y == (isize.height()-1) ) {  // right top
            appendSelection( x-1,   y, thl, activeValue, c, activeNeighbors );
            appendSelection( x-1, y-1, thl, activeValue, c, activeNeighbors );
            appendSelection( x  , y-1, thl, activeValue, c, activeNeighbors );
            //if ( sample(x-1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x-1,   y, thl) );
            //if ( sample(x-1, y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x-1, y-1, thl) );
            //if ( sample(x  , y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y-1, thl) );
        } else if ( x == (isize.width()-1) && y == 0 ) {  // right bottom
            appendSelection( x-1,   y, thl, activeValue, c, activeNeighbors );
            appendSelection( x-1, y+1, thl, activeValue, c, activeNeighbors );
            appendSelection( x  , y+1, thl, activeValue, c, activeNeighbors );
            //if ( sample(x-1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x-1,   y, thl) );
            //if ( sample(x-1, y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x-1, y+1, thl) );
            //if ( sample(x  , y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y+1, thl) );
        }
    }

    return activeNeighbors;
}

QMap<int, double> Dataset::GetPadMean() {

    QMap<int, double> meanvals;

    QList<int> keys = m_thresholdsToIndices.keys();
    QSize isize = QSize(computeBoundingBox().size().width()*this->x(), computeBoundingBox().size().height()*this->y());
    for ( int i = 0; i < keys.length(); i++ ) {
        meanvals[keys[i]] = calcPadMean(keys[i], isize); // this function is CPU consuming and this scope is blocking !
    }

    // Print
    cout << '\t';
    for(int i = 0; i < keys.length(); i++) {
        cout <<  "[" << keys[i] << "]" << '\t';
    }
    cout << endl;

    cout << fixed;
    cout.precision(0);
    cout << "mean:" << '\t';
    for(int i = 0; i < keys.length(); i++) {
        cout << meanvals[ keys[i] ] << '\t';
    }
    cout << endl;



    return meanvals;
}


void Dataset::applyCorrections(Ui::QCstmGLVisualization * ui) {

    if ( isAnyCorrectionActive( ui ) ) {

        QMap<int, double> meanvals = Dataset::GetPadMean();

        // Corrections
        if ( ui->obcorrCheckbox->isChecked() ) applyOBCorrection();
        if ( ui->deadpixelsinterpolationCheckbox->isChecked() ) applyDeadPixelsInterpolation( ui->noisyPixelMeanMultiplier->value(), meanvals );
        if ( ui->highinterpolationCheckbox->isChecked() ) applyHighPixelsInterpolation( ui->noisyPixelMeanMultiplier->value(), meanvals );
        if (ui->bhcorrCheckbox->isChecked()) applyBHCorrection();

    }

}

bool Dataset::isAnyCorrectionActive(Ui::QCstmGLVisualization * ui) {

    if (
            ui->bhcorrCheckbox->isChecked()
            ||
            ui->obcorrCheckbox->isChecked()
            ||
            ui->deadpixelsinterpolationCheckbox->isChecked()
            ||
            ui->highinterpolationCheckbox->isChecked()

            ) return true;

    return false;
}


void Dataset::applyOBCorrection(){

    // Check that the OB correction data has been loaded by the user
    if (obCorrection == nullptr)
        return;

    QList<int> keys = m_thresholdsToIndices.keys();
    for (int i = 0; i < keys.length(); i++) {
        //double currentTotal = getTotal(keys[i]), correctionTotal = correction->getTotal(keys[i]);
        int * currentLayer = getLayer(keys[i]);
        int * correctionLayer = obCorrection->getLayer(keys[i]);
        if (correctionLayer == nullptr) {
            qDebug() << "[WARN] flat-field correction does not contain a threshold" << keys[i];
            continue;
        }

        // Let the operation happen in float point and then we'll normalize to pass to the int map.
        // Allocate some scratch memory
        double * normFrame = new double[getPixelsPerLayer()];
        // Find the smallest value.  Initialize it for the search.
        double min = (double)correctionLayer[0];
        if (currentLayer[0] > correctionLayer[0]) min = currentLayer[0];

        for (int j = 0; j < getPixelsPerLayer(); j++) {

            //
            if (currentLayer[j] != 0)
            {
                if (correctionLayer[j] > 0) {

                    normFrame[j] = ((double)currentLayer[j]) / ((double)correctionLayer[j]);

                    //avoid negative values
                    if (normFrame[j] < 1.)
                        normFrame[j] = -log(normFrame[j]);
                    if (normFrame[j] >= 1.)
                        normFrame[j] = log(normFrame[j]);
                    if (j<20) std::cout << std::setprecision(10) << normFrame[j] << endl;

                    //set Minimum
                    if (normFrame[j] < min && normFrame[j]!= 0) min = normFrame[j];
                }
                else {
                    currentLayer[j] = 0;
                }

            }

        }

        int correctionFactor = (int)-floor(log10(min));
        cout << std::setprecision(10) << "minimum : " << (double)min << endl;
        cout << std::setprecision(10) << "correction : " << correctionFactor << endl;

        for (int j = 0; j < getPixelsPerLayer(); j++) {
            if (currentLayer[j] != 0)
                currentLayer[j] = round(normFrame[j] * pow(10.0, correctionFactor));


        }

        delete[] normFrame;

    }

}

void Dataset::applyBHCorrection(){
    //TODO implement

}

void Dataset::fromByteArray(QByteArray serialized){
    QDataStream in(&serialized, QIODevice::ReadOnly);
    in.readRawData((char*)&m_nx, (int)sizeof(m_nx));
    in.readRawData((char*)&m_ny, (int)sizeof(m_ny));
    in.readRawData((char*)&m_nFrames, (int)sizeof(m_nFrames));
    setFramesPerLayer(m_nFrames);
    int layerCount;
    in.readRawData((char*)&layerCount, (int)sizeof(layerCount));
    //setLayerCount(layerCount);
    in.readRawData((char*)m_frameLayouts.data(), m_nFrames*(int)sizeof(*m_frameLayouts.data()));
    in.readRawData((char*)m_frameOrientation.data(), m_nFrames*(int)sizeof(*m_frameOrientation.data()));
    QVector<int> keys(layerCount);
    in.readRawData((char*)keys.data(), keys.size()*(int)sizeof(int));
    QVector<int> frameBuffer(m_nx*m_ny);
    for(int i = 0; i < keys.size(); i++){
        for(int j = 0; j < m_nFrames; j++){
            in.readRawData((char*)frameBuffer.data(), (int)sizeof(float)*frameBuffer.size());
            this->setFrame(frameBuffer.data(), j, keys[i]);
        }
    }
}

void Dataset::clear() {

    for(int i =0; i < m_layers.size(); i++){
        delete[] m_layers[i];
    }
    m_layers.clear();
    m_thresholdsToIndices.clear();

    //setFramesPerLayer(1);
}

int Dataset::getNChipsX() {
    QRectF cb = computeBoundingBox();
    return (int)cb.width();
}

int Dataset::getNChipsY() {
    QRectF cb = computeBoundingBox();
    return (int)cb.height();
}

QRectF Dataset::computeBoundingBox(){
    m_boundingBox.setRect(0,0,0,0);
    int min_x = INT_MAX, min_y = INT_MAX, max_x = INT_MIN, max_y = INT_MIN;
    for(int i =0; i < m_frameLayouts.length();i++){
        if(m_frameLayouts[i].x() < min_x)
            min_x = m_frameLayouts[i].x();
        if(m_frameLayouts[i].y() < min_y)
            min_y = m_frameLayouts[i].y();
        if(m_frameLayouts[i].x() > max_x)
            max_x = m_frameLayouts[i].x();
        if(m_frameLayouts[i].y() > max_y)
            max_y = m_frameLayouts[i].y();
    }
    m_boundingBox.setRect(min_x,min_y, max_x+1, max_y+1);

    return m_boundingBox;
}

int Dataset::newLayer(int threshold){
    m_thresholdsToIndices[threshold] = m_layers.size();
    m_layers.append(new int[getPixelsPerLayer()]);
    for(int j = 0; j < getLayerSize(); j++)
        m_layers.last()[j] = 0;
    return m_layers.size()-1;
}

void Dataset::setFrame(int *frame, int index, int threshold){
    if(!m_thresholdsToIndices.contains(threshold))
        newLayer(threshold);
    int *newFrame = getFrame(index, threshold);
    for(int i = 0 ; i < m_nx*m_ny;i++)
        newFrame[i]= frame[i];
}

void Dataset::sumFrame(int *frame, int index, int threshold){
    if(!m_thresholdsToIndices.contains(threshold))
        newLayer(threshold);
    int *newFrame = getFrame(index, threshold);
    for(int i = 0 ; i < m_nx*m_ny;i++)
        newFrame[i] += frame[i];
}

int* Dataset::getFrame(int index, int threshold){
    if(!m_thresholdsToIndices.contains(threshold))
        return nullptr;
    else {
        //qDebug() << "th:" << threshold << ", index: " << thresholdToIndex(threshold);
        int N = m_thresholdsToIndices.size();
        for(int i = 0 ; i < N ; i++) qDebug() << m_layers[i] << " ";
        return &m_layers[thresholdToIndex(threshold)][index*m_nx*m_ny];
        //return m_layers[thresholdToIndex(threshold)];
    }
}

int* Dataset::getFrameAt(int index, int layer){
    return &m_layers[layer][index*m_nx*m_ny];
}

int Dataset::getContainingFrame(QPoint pixel){
    QPoint layoutSample((pixel.x()+m_nx)/m_nx -1, (pixel.y()+m_ny)/m_ny-1);
    for(int i = 0; i < m_frameLayouts.length();i++){
        if(layoutSample == m_frameLayouts[i])//TODO: orientation messes up sampling!
            return i;
    }
    return -1;
}

QPoint Dataset::getNaturalCoordinates(QPoint pixel, int index){
    int x = pixel.x() % m_nx;
    int y = pixel.y() % m_ny;
    int orientation = m_frameOrientation[index];
    if(!(orientation&1))
        x = m_nx -x-1;
    if(orientation&2)
        y = m_ny -y-1;
    if(orientation&4){
        int tmp = x;
        x = y;
        y = tmp;
    }
    return QPoint(x,y);
}

int Dataset::sample(int x, int y, int threshold){
    int layerIndex = thresholdToIndex(threshold);
    if(layerIndex == -1)
        return 0;
    int frameIndex  = getContainingFrame(QPoint(x,y));
    if(frameIndex == -1)
        return 0;
    int* frame = getFrameAt(frameIndex, layerIndex);
    QPoint coordinate = getNaturalCoordinates(QPoint(x,y), frameIndex);
    return frame[coordinate.y()*m_nx+coordinate.x()];
}

void Dataset::setPixel(int x, int y, int threshold, int val) {

    int layerIndex = thresholdToIndex(threshold);
    if(layerIndex == -1)
        return; // couldn't find a layer, no changes applied.
    int frameIndex  = getContainingFrame(QPoint(x,y));
    if(frameIndex == -1)
        return; // couldn't find the frame, no changes applied.
    int * frame = getFrameAt(frameIndex, layerIndex);
    QPoint coordinate = getNaturalCoordinates(QPoint(x,y), frameIndex);
    // set the value
    frame[coordinate.y()*m_nx+coordinate.x()] = val;

}

int  Dataset::sampleFrameAt(int index, int layer, int x, int y){
    int* frame = getFrameAt(index, layer);
    int orientation = m_frameOrientation[index];
    if(!(orientation&1))
        x = m_nx -x-1;
    if(orientation&2)
        y = m_ny -y-1;
    if(orientation&4){
        int tmp = x;
        x = y;
        y = tmp;
    }
    return frame[y*m_nx+x];//TODO:check math
}

void Dataset::setFramesPerLayer(int newFrameCount){
    int oldFrameCount =m_nFrames;
    m_nFrames = newFrameCount;
    m_frameOrientation.resize(m_nFrames);
    m_frameLayouts.resize(m_nFrames);
    for(int i = oldFrameCount; i < newFrameCount; i++){
        m_frameOrientation[i] = Dataset::orientationLtRTtB;
        m_frameLayouts[i] = QPoint(0,0);
    }
}

void Dataset::setLayer(int *data, int threshold){
    int layerIndex = getLayerIndex(threshold);
    for(int i = 0; i < m_nFrames*m_nx*m_ny;i++)
        m_layers[layerIndex][i] = data[i];
}

void Dataset::addLayer(int *data, int threshold){
    int layerIndex = getLayerIndex(threshold);
    for(int i = 0; i < m_nFrames*m_nx*m_ny;i++)
        m_layers[layerIndex][i] += data[i];
}

int * Dataset::getFullImageAsArrayWithLayout(int threshold, Mpx3GUI * mpx3gui) {

    // This two members carry all the information about the layout
    // QVector<QPoint>  m_frameLayouts // positions in the pad
    // QVector<int> m_frameOrientation // orientations

    // I want the layout of the whole chip.  I will build it again

    int nChips = getNChipsX() * getNChipsY();
    vector<QPoint> frameLayouts = mpx3gui->getLayout();
    vector<int> frameOrientation = mpx3gui->getOrientation();

    // - Now work out the offsets
    QVector<QPoint> offsets;
    for ( int i = 0 ; i < nChips ; i++ ) {
        QPoint point = frameLayouts[i];
        offsets.push_back( QPoint( point.x() * x(), point.y() * y() ) );
        qDebug() << point.x() * x() << ", " << point.y() * y();
    }
    // - Work out the orientation
    //   Here we decide where the loop starts and in which direction
    QVector<int> directionx;
    QVector<int> directiony;
    QVector<int> startx;
    QVector<int> starty;
    QVector<int> endx;
    QVector<int> endy;

    for ( int i = 0 ; i < nChips ; i++ ) {
        if ( frameOrientation[i] == orientationTtBRtL ) {
            directionx.push_back(  1 );
            directiony.push_back(  1 );
            startx.push_back( 0 + offsets[i].x() );
            starty.push_back( 0 + offsets[i].y() );
            endx.push_back( x() + offsets[i].x() );
            endy.push_back( y() + offsets[i].y() );

        }
        if ( frameOrientation[i] == orientationBtTLtR ) {
            directionx.push_back( -1 );
            directiony.push_back( -1 );
            startx.push_back( x() + offsets[i].x() - 1 );
            starty.push_back( y() + offsets[i].y() - 1 );
            endx.push_back( 0 + offsets[i].x() );
            endy.push_back( 0 + offsets[i].y() );
        }
    }

    // - Create a buffer for the whole image
    if ( _plainImageBuff ) delete [] _plainImageBuff;
    _plainImageBuff = new int[nChips * x() * y()];

    // - Fill it according to layout
    // - Take one chip
    int pixIdTranslate = 0, pixCntr = 0;
    int sizex_full = getNChipsX() * x();
    int dataIndx = 0;
    int * chipdata = nullptr;

    for ( int i = 0 ; i < nChips ; i++ ) {

        // - Get the layer
        // The data comes organized per chip.
        dataIndx = mpx3gui->getConfig()->getIDIndex(i);
        if ( dataIndx >= 0 ) chipdata = getFrame( dataIndx , threshold);
        else chipdata = nullptr;

        int x = startx[i];
        int y = starty[i];
        bool gox = true;
        bool goy = true;
        pixCntr = 0;

        for ( ; gox ; ) {

            // rewind
            goy = true;
            y = starty[i];

            for ( ; goy ; ) {

                //////////////////////
                // Data !
                pixIdTranslate = XYtoX(x, y, sizex_full);

                if ( chipdata ) { // There's data for this chip

                    //qDebug() << "[" << i << "]" << x << "," << y << " : " << pixIdTranslate << " | " << pixCntr;

                    _plainImageBuff[pixIdTranslate] = chipdata[pixCntr++];
                } else {

                    _plainImageBuff[pixIdTranslate] = 0;

                }

                // direction and stop
                if ( directiony[i] > 0 ) {
                    y++;
                    if ( y >= endy[i] ) goy = false;
                }
                if ( directiony[i] < 0 ) {
                    y--;
                    if ( y < endy[i] ) goy = false;
                }
            }
            // direction and stop
            if ( directionx[i] > 0 ) {
                x++;
                if ( x >= endx[i] ) gox = false;
            }
            if ( directionx[i] < 0 ) {
                x--;
                if ( x < endx[i] ) gox = false;
            }
        }


    }

    /*
    for ( int i = 0 ; i < getPixelsPerLayer() ; i++ ) {
        if ( i<10 || (i>127&&i<137) ) qDebug() << "[" << i << "] " << layer[i];
        if ( i>=16384 && i<(16384+10) ) qDebug() << "[" << i << "] " << layer[i];
    }
    */

    return _plainImageBuff;
}

int* Dataset::getLayer(int threshold){
    int layerIndex = thresholdToIndex(threshold);
    if(layerIndex == -1)
        return nullptr;
    return m_layers[layerIndex];
}
