#ifndef DATACONSUMERTHREAD_H
#define DATACONSUMERTHREAD_H

#include <QThread>
#include <QSemaphore>
#include <QObject>

#include "mpx3gui.h"
#include "mpx3defs.h"

class DataConsumerThread : public QThread
{

    Q_OBJECT

public:

    explicit DataConsumerThread(Mpx3GUI *, QObject * parent = nullptr);
    virtual ~DataConsumerThread();
    void copydata(int * source, size_t num);
    void rewindcopydata(int nChipsRewind, size_t num);
    void rewindcopydata(size_t num);
    void freeResources();
    uint getSemaphoreSize(){return _semaphoreSize;}
    void dataTakingSaysIFinished();

    void consume();
    void SeparateThresholds(int th,
                       int * data,
                       int chipOffset
                       );
    int XYtoX(int x, int y, int dimX) { return y * dimX + x; }

    QSemaphore * freeFrames;
    QSemaphore * usedFrames;
    alignas (64) int * buffer = nullptr;
    uint descriptor = 0;
    uint readdescriptor = 0;

protected:

    void run() Q_DECL_OVERRIDE;

signals:

    void bufferOccupancySig(int);
    void doneWithOneFrame(int);
    void bufferFull(int);

private:

    QMutex _mutex;
    QWaitCondition _condition;
    bool _restart;
    bool _abort;
    bool _stop;
    int _frameId;

    Mpx3GUI * _mpx3gui;

    const uint _nFramesBuffer = 2048;
    uint _semaphoreSize;
    uint _nChips;
    bool _bothCounters;
    uint _bufferSize;
    uint _bufferSizeHalf;

    uint _bufferSizeOneFrame;

    int ** _colordata = nullptr;

};

#endif // DATACONSUMERTHREAD_H
