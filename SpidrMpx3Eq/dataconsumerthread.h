#ifndef DATACONSUMERTHREAD_H
#define DATACONSUMERTHREAD_H

#include <QObject>
#include <QSemaphore>

#include "mpx3gui.h"
#include "mpx3defs.h"

class DataConsumerThread : public QThread
{

public:

    DataConsumerThread(Mpx3GUI *, QObject * parent);

    void run() Q_DECL_OVERRIDE;

    void copydata(const void * source, size_t num);

    QSemaphore * freeFrames;
    QSemaphore * usedFrames;
    int * buffer;
    uint descriptor = 0;

private:

    const uint _nFramesBuffer = 10000;
    uint _nChips;
    bool _bothCounters;
    uint _bufferSize;
    uint _bufferSizeBytes;

};

#endif // DATACONSUMERTHREAD_H
