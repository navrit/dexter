#include "dataconsumerthread.h"

DataConsumerThread::DataConsumerThread(Mpx3GUI * mpx3gui, QObject * parent)
: QThread( parent )
{

    _nChips = mpx3gui->getConfig()->getNActiveDevices();
    _bothCounters = mpx3gui->getConfig()->getReadBothCounters();

    _bufferSize = _nChips * MPX_PIXELS; // nChips * 256*256
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

}

void DataConsumerThread::copydata(const void * source, size_t num )
{
    if ( descriptor < _bufferSize ) {
        memcpy( buffer+descriptor, source, num );
        descriptor += num/4;            // 4 bytes per integer
    }
}

void DataConsumerThread::run()
{

}

