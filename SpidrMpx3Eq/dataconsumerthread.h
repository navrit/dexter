#ifndef DATACONSUMERTHREAD_H
#define DATACONSUMERTHREAD_H

#include <QThread>
#include <QSemaphore>
#include <QObject>

#include "mpx3gui.h"
#include "mpx3defs.h"
#include "FrameSet.h"

class DataConsumerThread : public QThread
{
    Q_OBJECT

public:

    explicit DataConsumerThread(Mpx3GUI *, QObject * parent = nullptr);
    virtual ~DataConsumerThread() override;
    void copydata(FrameSet * source, int chipIndex, bool counterH);
    uint getSemaphoreSize(){return _semaphoreSize;}
    void dataTakingSaysIFinished();

    void consume();
    void SeparateThresholds(int threshold_offset, uint32_t *data, uint chip_offset);
    int XYtoX(int x, int y, int dimX) { return y * dimX + x; }

    QSemaphore *freeFrames = nullptr;
    QSemaphore *usedFrames = nullptr;

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

    Mpx3GUI * _mpx3gui = nullptr;

    const uint _nFramesBuffer = 32;
    uint _semaphoreSize;
    uint _nChips;
    bool _bothCounters;
    uint _bufferSize;
    uint _bufferSizeHalf;

    uint _bufferSizeOneFrame;

    void inc(uint& var, uint increase);
    uint32_t * buffer = nullptr;
    uint descriptor = 0;
    uint readdescriptor = 0;
    int ** _colourdata = {nullptr};
};

#endif // DATACONSUMERTHREAD_H
