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
    freeFrames = new QSemaphore( int(_nFramesBuffer * _nChips) ); // nChips per frame
    usedFrames = new QSemaphore;

    // When working in colour mode.
    // The user might select ColourMode in the middle of an operation.
    // Allocating the data just in case.
    _colourdata = new int*[ __max_colours ]; // 8 thresholds (colours)
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
                 &QCstmGLVisualization::bufferOccupancySlot);
        connect ( this, &DataConsumerThread::doneWithOneFrame,
                  _mpx3gui->getVisualization(),
                  &QCstmGLVisualization::consumerFinishedOneFrame,
                  Qt::DirectConnection);

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
    source->copyTo32(chipIndex, counterH, buffer + descriptor);
    inc(descriptor, MPX_PIXEL_ROWS * MPX_PIXEL_COLUMNS);
}

void DataConsumerThread::run()
{
    int bothCountersMode = 1;
    uint colourIncrement = 1;

    forever {

        // When abort execution. Triggered as the destructor is called.
        if ( _abort ) return;

        // Go chasing the producer
        while ( readdescriptor != descriptor ) {

            _bothCounters = _mpx3gui->getConfig()->getReadBothCounters();

            // Check single or both counters
            if ( _bothCounters ) {
                bothCountersMode = 2;
                colourIncrement = 1; // do all of them (8)
            } else {
                bothCountersMode = 1;
                colourIncrement = 2; // do 0, 2, 4, 6
            }

            //! Colour mode - spectroscopic mode
            if ( _mpx3gui->getConfig()->getColourMode() ) {
                for (int i = 0; i < bothCountersMode; i++) {
                    for (uint chip = 0; chip < _nChips; chip++) {
                        usedFrames->acquire();
                        SeparateThresholds(i, buffer + readdescriptor, chip); // The incoming numbers are correct at this point
                    }
                    inc(readdescriptor, _bufferSizeOneFrame);
                    for (uint chip = 0; chip < _nChips; chip++) freeFrames->release();
                }
                for (int threshold = 0; threshold < __max_colours; threshold++) {
                    _mpx3gui->addLayer(_colourdata[threshold], threshold);
                }
            //! FPM - Fine Pitch Mode
            } else {
                for (int i = 0; i < bothCountersMode; i++) {
                    for (uint ci = 0; ci < _nChips; ci++) usedFrames->acquire();
                    _mpx3gui->addLayer(reinterpret_cast<int*>(buffer + readdescriptor), i);
                    inc(readdescriptor, _bufferSizeOneFrame);
                    for (uint ci = 0; ci < _nChips; ci++) freeFrames->release();
                }
            }

            emit bufferOccupancySig( int(100.*(usedFrames->available()/ double(_semaphoreSize))));
            emit doneWithOneFrame( _frameId++ );
            if ( _stop ) break;
        }

        _mutex.lock();
        if (!_restart) _condition.wait(&_mutex);
        _restart = false;
        _mutex.unlock();
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
                                            uint chip_offset) {

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

//  int th01_moreThan0 = 0;
//  int th23_moreThan0 = 0;
//  int th45_moreThan0 = 0;
//  int th67_moreThan0 = 0;

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


      // These all pass with 12 bit, integration on, colour mode, single and double counter...
//      assert(_colourdata[0 + threshold_offset][pixel_index_colour] >= 0);
//      assert(_colourdata[2 + threshold_offset][pixel_index_colour] >= 0);
//      assert(_colourdata[4 + threshold_offset][pixel_index_colour] >= 0);
//      assert(_colourdata[6 + threshold_offset][pixel_index_colour] >= 0);

//      assert(threshold_offset == 0 || threshold_offset == 1);

      // Only for 12 bit mode
//      assert(_colourdata[0 + threshold_offset][pixel_index_colour] < 4096);
//      assert(_colourdata[2 + threshold_offset][pixel_index_colour] < 4096);
//      assert(_colourdata[4 + threshold_offset][pixel_index_colour] < 4096);
//      assert(_colourdata[6 + threshold_offset][pixel_index_colour] < 4096);
      // -------------------------------------------------------------------------------------

//      if (data[pixel_index_input_data + __matrix_size_x] > 0) {
//          th01_moreThan0 += 1;
//      }
//      if (data[pixel_index_input_data] > 0) {
//          th23_moreThan0 += 1;
//      }
//      if (data[pixel_index_input_data + 1 + __matrix_size_x] > 0) {
//          th45_moreThan0 += 1;
//      }
//      if (data[pixel_index_input_data + 1] > 0) {
//          th67_moreThan0 += 1;
//      }

      pixel_cluster_index_i++;
    }

    pixel_cluster_index_j++;
  }

//  qDebug() << "Input colour data =" << th01_moreThan0 << th23_moreThan0 << th45_moreThan0 << th67_moreThan0 << " | threshold_offset =" << threshold_offset;
}
