#include "dataconsumerthread.h"
#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"

#include <QVector>

DataConsumerThread::DataConsumerThread(Mpx3GUI * mpx3gui, QObject * parent)
    : QThread( parent )
{

    _restart = false;
    _abort = false;
    _stop = false;
    _frameId = 0;

    _mpx3gui = mpx3gui;

    _nChips = mpx3gui->getConfig()->getNActiveDevices();
    _bothCounters = mpx3gui->getConfig()->getReadBothCounters();

    _bufferSize = _nChips * MPX_PIXELS; // nChips * 256*256
    _bufferSizeOneFrame = _bufferSize;
    if ( _bothCounters ) _bufferSize *= 2; // two counters if necessary
    _bufferSize *= _nFramesBuffer;
    _bufferSizeHalf = _bufferSize/2;

    // Circular buffer
    buffer = new uint32_t[ _bufferSize ];
    descriptor = 0;         // in number of integers
    //qDebug() << "[INFO] Consumer. Buffer size: " << _bufferSize/1024.0/1024.0 << "Mb";

    // The Semaphores
    // _nFramesBuffer is the number of resources
    _semaphoreSize = _nFramesBuffer * _nChips;
    freeFrames = new QSemaphore( _nFramesBuffer * _nChips ); // nChips per frame
    usedFrames = new QSemaphore;

    // When working in color mode.
    // The user might select ColorMode in the middle of an operation.
    // Allocating the data just in case.
    _colordata = new int*[ __max_colors ]; // 8 thresholds(colors)
    for (int i = 0 ; i < __max_colors ; i++) {
        _colordata[i] = new int[ __matrix_size_color * _nChips ];
        memset(_colordata[i], 0, (sizeof(int) * __matrix_size_color * _nChips ));
    }

}

DataConsumerThread::~DataConsumerThread() {

    _mutex.lock();
    _abort = true;          // will stop run as soon as possible
    _mutex.unlock();
    _condition.wakeOne();   // wake up if sleeping

    wait(); // wait until run has exited before the base class destructor is invoked

    // Signals
    disconnect( this, &DataConsumerThread::bufferOccupancySig,
                _mpx3gui->getVisualization(), &QCstmGLVisualization::bufferOccupancySlot );
    disconnect ( this, &DataConsumerThread::doneWithOneFrame,
              _mpx3gui->getVisualization(),
              &QCstmGLVisualization::consumerFinishedOneFrame
              );


    // Color structure
    for (int i = 0 ; i < __max_colors ; i++) delete [] _colordata[i];
    delete [] _colordata;
    delete [] buffer;

    qDebug() << "[DEBUG]\tDataConsumerThread finished";
}

void DataConsumerThread::consume()
{
    QMutexLocker locker(&_mutex);

    if ( !isRunning() ) {

        connect( this, &DataConsumerThread::bufferOccupancySig,
                 _mpx3gui->getVisualization(),
                 &QCstmGLVisualization::bufferOccupancySlot
                 );
        connect ( this, &DataConsumerThread::doneWithOneFrame,
                  _mpx3gui->getVisualization(),
                  &QCstmGLVisualization::consumerFinishedOneFrame,
                  Qt::DirectConnection
                  // needs to finish before dataset is overwritten (#266)
                  // see also #244
                  );

        // Start !
        start( HighestPriority );

    } else {
        _restart = true;
        locker.unlock();
        _condition.wakeOne();
    }
}

void DataConsumerThread::inc(uint& var, uint increase) {
    var += increase;
    assert (var <= _bufferSize);
    if (var == _bufferSize) var = 0;
}

void DataConsumerThread::copydata(FrameSet * source, int chipIndex, bool counterH)
{

    size_t num = 4 * 256 * 256;
    source->copyTo32(chipIndex, counterH, buffer + descriptor);

    inc(descriptor, num/4);            // 4 bytes per integer
}

void DataConsumerThread::run()
{

    int bothCountersMod = 1;
    int delvrCounters = 1;

    forever {

        // When abort execution. Triggered as the destructor is called.
        if ( _abort ) return;

        //_mutex.lock();
        //_frameId = 0;
        // local variables to scope variables
        //_mutex.unlock();


        // Go chasing the producer
        while ( readdescriptor != descriptor ) {

            // Check single or both counters
            if ( _mpx3gui->getConfig()->getReadBothCounters() ) {
                bothCountersMod = 2;
                delvrCounters = 1; // do all of them (8)
            } else {
                bothCountersMod = 1;
                delvrCounters = 2; // do 0,2,4,6
            }

            /////////////////
            // Colour Mode //
            if ( _mpx3gui->getConfig()->getColourMode() ) {

                if (_bothCounters) {
                    for ( int i = 0 ; i < bothCountersMod ; i++ ) {
                        // SeparateThresholds -> I can do it on a chip per chip basis
                        for ( uint ci = 0 ; ci < _nChips ; ci++ ) {
                            usedFrames->acquire();
                            SeparateThresholds(i,
                                               buffer + readdescriptor,
                                               ci);
                            freeFrames->release();
                        }
                        // Move the reading descriptor
                        inc(readdescriptor, _bufferSizeOneFrame);
                    }
                } else {
                    // SeparateThresholds -> I can do it on a chip per chip basis
                    for ( uint ci = 0 ; ci < _nChips ; ci++ ) {
                        usedFrames->acquire();
                        SeparateThresholds(0,
                                           buffer + readdescriptor,
                                           ci);
                        freeFrames->release();
                    }
                    // Move the reading descriptor
                    inc(readdescriptor, _bufferSizeOneFrame);
                }

                // Add the corresponding layers
                if ( _colordata != nullptr ) {
                    for ( int i = 0 ; i < __max_colors ; i+= delvrCounters ) {
                        _mpx3gui->addLayer( _colordata[i], i );
                    }
                }

                /////////////
                // FP Mode //
            } else {

                // Send the info -> use the Semaphores (here's where I use the share resource)
                // Acquire and release for N chips
                for ( int i = 0 ; i < bothCountersMod ; i++ ) {
                    // I need the info for ALL the 4 chips acquired first
                    for ( uint ci = 0 ; ci < _nChips ; ci++ ) usedFrames->acquire();
                    // Now I can work on the layer
                    _mpx3gui->addLayer(reinterpret_cast<int*>(buffer + readdescriptor), i );
                    // Move the reading descriptor
                    inc(readdescriptor, _bufferSizeOneFrame);
                    // Then I can release
                    for ( uint ci = 0 ; ci < _nChips ; ci++ ) freeFrames->release();
                }

            }

            // Fraction
            emit bufferOccupancySig( (int)(100*(usedFrames->available()/ (double)(_semaphoreSize) ) ) );

            emit doneWithOneFrame( _frameId++ );

            if ( _stop ) break;
        }


        //qDebug() << "   --- lock DataConsumerThread";
        _mutex.lock();
        if (!_restart)
            _condition.wait(&_mutex);
        _restart = false;
        _mutex.unlock();
        //qDebug() << "   +++ unlock DataConsumerThread";

    }

}

void DataConsumerThread::dataTakingSaysIFinished()
{
    // Then rewind the counter
    _mutex.lock();
    _frameId = 0;
    // local variables to scope variables
    _mutex.unlock();
}

void DataConsumerThread::SeparateThresholds(int th,
                                            uint32_t * data,
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
            // 'data' bring the information of all 4 chips
            indx = XYtoX( i, j, __matrix_size_x);
            indx += chipOffset*__matrix_size;

            indxRed = XYtoX( redi, redj, __matrix_size_x / 2); // This index should go up to 128*128
            indxRed += chipOffset*__matrix_size_color;

            if( (i % 2) == 0 && (j % 2) == 0) {
                _colordata[2+th][indxRed] = data[indx]; // P2 // TH2 !
            }
            if( (i % 2) == 0 && (j % 2) == 1) {
                _colordata[0+th][indxRed] = data[indx]; // P1 // TH0 !
            }
            if( (i % 2) == 1 && (j % 2) == 0) {
                _colordata[6+th][indxRed] = data[indx]; // P4 // TH6 !
            }
            if( (i % 2) == 1 && (j % 2) == 1) {
                _colordata[4+th][indxRed] = data[indx]; // P3 // TH4 !
            }

            if (i % 2 == 1) redi++;

        }

        if (j % 2 == 1) redj++;

    }

}

