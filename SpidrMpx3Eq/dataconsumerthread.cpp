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
    _colourdata = new int*[ __max_colours ]; // 8 thresholds(colors)
    for (int i = 0 ; i < __max_colours ; i++) {
        _colourdata[i] = new int[ __matrix_size_colour * _nChips ];
        memset(_colourdata[i], 0, (sizeof(int) * __matrix_size_colour * _nChips ));
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
    for (int i = 0 ; i < __max_colours ; i++) delete [] _colourdata[i];
    delete [] _colourdata;
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

        // Go chasing the producer
        while ( readdescriptor != descriptor ) {

            _bothCounters = _mpx3gui->getConfig()->getReadBothCounters();

            // Check single or both counters
            if ( _bothCounters ) {
                bothCountersMod = 2;
                delvrCounters = 1; // do all of them (8)
            } else {
                bothCountersMod = 1;
                delvrCounters = 2; // do 0,2,4,6
            }

            /////////////////
            // Colour Mode //
            if ( _mpx3gui->getConfig()->getColourMode() ) {

                if ( _bothCounters ) {

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
                if ( _colourdata != nullptr ) {
                    for ( int i = 0 ; i < __max_colours ; i+= delvrCounters ) {
                        _mpx3gui->addLayer( _colourdata[i], i );
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

void DataConsumerThread::SeparateThresholds(int threshold_offset, uint32_t *data,
                                            int chip_offset) {

  // Layout of 110um pixel
  //  -------------   ---------------------
  //  | P3  |  P1 |   | thl 4,5 | thl 0,1 |
  //	-------------   ---------------------
  //  | P4  |  P2 |   | thl 6,7 | thl 2,3 |
  //  -------------   ---------------------
  //  Where:
  //  	    P1 --> TH0, TH1
  //		P2 --> TH2, TH3
  //		P3 --> TH4, TH5
  //		P4 --> TH6, TH7

  int pixel_index_input_data = 0, pixel_index_colour = 0, pixel_cluster_index_i = 0, pixel_cluster_index_j = 0;


  for (int j = 0; j < __matrix_size_y; j += 2) {

    pixel_cluster_index_i = 0;
    for (int i = 0; i < __matrix_size_x; i += 2) {

      // Depending on which chip are we taking care of, consider the offset.
      // 'data' bring the information of all 4 chips
      pixel_index_input_data = XYtoX(i, j, __matrix_size_x);
      pixel_index_input_data += chip_offset * __matrix_size;

      // Pixel_index_colour goes up to 128*128 = 16384
      // i.e. this is the end of the chip for a specific threshold
      pixel_index_colour = XYtoX(pixel_cluster_index_i, pixel_cluster_index_j, __matrix_size_colour_x);
      pixel_index_colour += chip_offset * __matrix_size_colour;

      _colourdata[0 + threshold_offset][pixel_index_colour] = data[pixel_index_input_data + __matrix_size_x]; // P1 // TH0 !
      _colourdata[2 + threshold_offset][pixel_index_colour] = data[pixel_index_input_data]; // P2 // TH2 !
      _colourdata[4 + threshold_offset][pixel_index_colour] = data[pixel_index_input_data + 1 + __matrix_size_x]; // P3 // TH4 !
      _colourdata[6 + threshold_offset][pixel_index_colour] = data[pixel_index_input_data + 1]; // P4 // TH6 !

      pixel_cluster_index_i++;
    }

    pixel_cluster_index_j++;
  }
}
