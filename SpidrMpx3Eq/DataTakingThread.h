/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2015.
 */

#ifndef DATATAKINGTHREAD_H
#define DATATAKINGTHREAD_H


#include <QThread>
#include <QVector>

#include <iostream>

#include "mpx3gui.h"
#include "mpx3defs.h"

using namespace std;

class QCstmGLVisualization;
class SpidrController;
class SpidrDaq;
class DataConsumerThread;

class DataTakingThread : public QThread {

    Q_OBJECT

public:
    explicit DataTakingThread(Mpx3GUI *, DataConsumerThread *, QObject * parent);
    virtual ~DataTakingThread() override;
    void ConnectToHardware();

    typedef struct {
        int missingToCompleteJob;
        int framesRequested;
        int framesReceived;
        int framesKept;
        bool tocomplete;
    } datataking_score_info;

    bool isIdling(){ return _idling; }
    void rewindScoring();
    void stop() { _stop = true; }
    int calcScoreDifference();
    void setFramesRequested(int nf) {
        _score.framesRequested = nf;
        if ( ! _score.tocomplete ) _score.missingToCompleteJob = nf;
    }

    void takedata();

    void setExternalTrigger(bool external);

protected:

    void run() Q_DECL_OVERRIDE;

private:

    QMutex _mutex;
    QWaitCondition _condition;
    DataConsumerThread * _consumer;
    bool _restart;
    bool _abort;
    bool _idling;
    bool _stop;

    Mpx3GUI * _mpx3gui = nullptr;
    QCstmGLVisualization * _vis = nullptr;
    datataking_score_info _score;

    // IP source address (SPIDR network interface)
    int _srcAddr;

    bool _isExternalTrigger = false;

public slots:
    void on_stop_data_taking_thread();

signals:
    // drawing signals calling back to QCstmGLVisualization
    void data_taking_finished();
    void bufferFull(int);

    void scoring_sig(int nFramesReceived, int nFramesKept, int lost_frames,
                     int lost_packets, int frames_count, int mpx3clock_stops, bool misaligned);

    void sendingShutter();
};

#endif

