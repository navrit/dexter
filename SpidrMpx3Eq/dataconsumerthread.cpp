#include "dataconsumerthread.h"
#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"

#include <QVector>

DataConsumerThread::DataConsumerThread(Mpx3GUI * mpx3gui, QObject * parent)
    : QThread( parent )
{

    _restart = false;
    _abort = false;

    _mpx3gui = mpx3gui;

    _nChips = mpx3gui->getConfig()->getNActiveDevices();
    _bothCounters = mpx3gui->getConfig()->getReadBothCounters();

    _bufferSize = _nChips * MPX_PIXELS; // nChips * 256*256
    _bufferSizeOneFrame = _bufferSize;
    if ( _bothCounters ) _bufferSize *= 2; // two counters if necessary
    _bufferSize *= _nFramesBuffer;

    // Circular buffer
    buffer = new int[ _bufferSize ];
    descriptor = 0;         // in number of integers
    qDebug() << "[INFO] Consumer. Buffer size: " << _bufferSize;

    // The Semaphores
    // _nFramesBuffer is the number of resources
    freeFrames = new QSemaphore( _nFramesBuffer * _nChips ); // nChips per frame
    usedFrames = new QSemaphore;

    // When working in color mode.
    // The user might select ColorMode in the middle of an operation.
    // Allocating the data just in case.
    _colordata = new int*[ __max_colors ]; // 8 thresholds(colors)
    for (int i = 0 ; i < __max_colors ; i++) {
        _colordata[i] = new int[ __matrix_size_color * _nChips ];
    }

}

DataConsumerThread::~DataConsumerThread() {

    _mutex.lock();
    _abort = true;          // will stop run as soon as possible
    _condition.wakeOne();   // wake up if sleeping
    _mutex.unlock();

    wait(); // wait 'til run has exited before the base class destructor is invoked

    // color structure
    for (int i = 0 ; i < __max_colors ; i++) delete [] _colordata[i];
    delete [] _colordata;

    qDebug() << "   DataConsumerThread finished";
}

void DataConsumerThread::consume()
{
    QMutexLocker locker(&_mutex);

    if ( !isRunning() ) {
        start( HighPriority );
    } else {
        _restart = true;
        _condition.wakeOne();
    }
}

void DataConsumerThread::copydata(int * source, size_t num )
{

    //qDebug() << "   Copy --> descriptor : " << descriptor;

    memcpy( buffer + descriptor, source, num );
    descriptor += num/4;            // 4 bytes per integer

    // rewind descriptor -- circular buffer
    if ( descriptor >= _bufferSize ) descriptor = 0;

}

void DataConsumerThread::run()
{

    int bothCountersMod = 1;

    forever {

        // When abort execution. Triggered as the destructor is called.
        if( _abort ) return;

        //_mutex.lock();
        // local variables to scope variables
        //_mutex.unlock();

        // Go chasing the producer
        while ( readdescriptor != descriptor ) {

            /////////////////
            // Colour Mode //
            if ( _mpx3gui->getConfig()->getColourMode() ) {

                // SeparateThresholds -> use the Semaphores (here's where I use the share resource)
                for ( int ci = 0 ; ci < _nChips ; ci++ ) {
                    usedFrames->acquire(); // ! it's one per chip to keep the right counting with the producer !
                    SeparateThresholds(0,
                                       buffer + readdescriptor,
                                       ci);
                    freeFrames->release();

                }

                // Check single or both counters
                if ( ! _mpx3gui->getConfig()->getReadBothCounters() ) bothCountersMod = 2; // do 0,2,4,6
                else bothCountersMod = 1; // do all of them (8)

                for ( int i = 0 ; i < __max_colors ; i++ ) {
                    if ( i % bothCountersMod == 0 ) _mpx3gui->addLayer( _colordata[i], i );
                }

                /////////////
                // FP Mode //
            } else {

                // Check single or both counters
                if ( ! _mpx3gui->getConfig()->getReadBothCounters() ) bothCountersMod = 1; // do 0 only
                else bothCountersMod = 2; // do 0,1

                // Send the info -> use the Semaphores (here's where I use the share resource)
                usedFrames->acquire();
                for ( int i = 0 ; i < bothCountersMod ; i++ ) {
                    _mpx3gui->addLayer( buffer + readdescriptor, i );
                }
                freeFrames->release();

            }

            // Move the reading descriptor
            if ( _bothCounters ) readdescriptor += 2*_bufferSizeOneFrame;
            else readdescriptor += _bufferSizeOneFrame;
            // or rewind
            if ( readdescriptor >= _bufferSize ) readdescriptor = 0;

            qDebug() << "   Occupancy : "
                     << 100.0*(descriptor/(double)_bufferSize)
                     << "\% | readdescriptor --> " << readdescriptor << " | descriptor : " << descriptor << " | buffer : " << _bufferSize;

        }

        qDebug() << "   lock DataConsumerThread";
        _mutex.lock();
        if (!_restart)
            _condition.wait(&_mutex);
        _restart = false;
        _mutex.unlock();
        //qDebug() << "   +++ unlock DataConsumerThread";

    }

}


void DataConsumerThread::SeparateThresholds(int /*id*/,
                                            int * data,
                                            int chipOffset) {

    // Layout of 110um pixel
    //  -------------   ---------------------
    //  | P3  |  P1 |   | thl 4,5 | thl 0,1 |
    //	-------------   ---------------------
    //  | P4  |  P2 |   | thl 6,7 | thl 2,3 |
    //  -------------   ---------------------
    //  Where:
    //  	P1 --> TH0, TH1
    //		P2 --> TH2, TH3
    //		P3 --> TH4, TH5
    //		P4 --> TH6, TH7

    int indx = 0, indxRed = 0, redi = 0, redj = 0;

    for (int j = 0 ; j < __matrix_size_y ; j++) {

        redi = 0;
        for (int i = 0 ; i < __matrix_size_x  ; i++) {

            // Depending on which chip are we taking care of, consider the offset.
            // 'data' bring the informatio nof all 4 chips
            indx = XYtoX( i, j, __matrix_size_x);
            indx += chipOffset*__matrix_size;

            indxRed = XYtoX( redi, redj, __matrix_size_x / 2); // This index should go up to 128*128
            indxRed += chipOffset*__matrix_size_color;

            if( (i % 2) == 0 && (j % 2) == 0) {
                _colordata[2][indxRed] = data[indx]; // P2 // TH2 !
            }
            if( (i % 2) == 0 && (j % 2) == 1) {
                _colordata[0][indxRed] = data[indx]; // P1 // TH0 !
            }
            if( (i % 2) == 1 && (j % 2) == 0) {
                _colordata[6][indxRed] = data[indx]; // P4 // TH6 !
            }
            if( (i % 2) == 1 && (j % 2) == 1) {
                _colordata[4][indxRed] = data[indx]; // P3 // TH4 !
            }

            if (i % 2 == 1) redi++;

        }

        if (j % 2 == 1) redj++;

    }

}

