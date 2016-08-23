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

class DataTakingThread : public QThread {

    Q_OBJECT

public:
    explicit DataTakingThread(Mpx3GUI *, QObject * parent);
    ~DataTakingThread();
    void ConnectToHardware();
    void SeparateThresholds(int id, int * data, int size, QVector<int> * th0, QVector<int> * th2, QVector<int> * th4, QVector<int> * th6, int sizeReduced);
    pair<int, int> XtoXY(int X, int dimX);
    int XYtoX(int x, int y, int dimX) { return y * dimX + x; }
    bool ThereIsAFalse(vector<bool> v);

    typedef struct {
        int missingToCompleteJob;
        int framesRequested;
        int framesReceived;
        int framesKept;
        bool tocomplete;
    } datataking_score_info;

    //datataking_score_info getScoreInfo() { return _score; }
    bool isIdling(){ return _idling; }
    void rewindScoring();
    void stop() { _stop = true; }
    bool isACompleteJob() { return _score.tocomplete; }
    int getMissingFramesToCompleteJob() { return _score.missingToCompleteJob; }
    int getFramesReceived() { return _score.framesReceived; }
    int calcScoreDifference();
    void setFramesRequested(int nf) {
        _score.framesRequested = nf;
        if ( ! _score.tocomplete ) _score.missingToCompleteJob = nf;
    }

    QVector<int> getData(int layer);

    void run2();

    void takedata();

protected:

    void run() Q_DECL_OVERRIDE;

private:

    QMutex _mutex;
    QWaitCondition _condition;
    bool _restart;
    bool _abort;
    bool _idling;

    Mpx3GUI * _mpx3gui;
    QCstmGLVisualization * _vis;
    bool _stop;
    bool _canDraw;
    datataking_score_info _score;

    // IP source address (SPIDR network interface)
    int _srcAddr;

    QVector<int> ** _th0;
    QVector<int> ** _th2;
    QVector<int> ** _th4;
    QVector<int> ** _th6;

    // Incoming data. One per chip(index)
    QQueue<QVector<int>> _incomingDataTH0;
    QQueue<QVector<int>> _incomingDataTH2;
    QQueue<QVector<int>> _incomingDataTH4;
    QQueue<QVector<int>> _incomingDataTH6;

public slots:
    void on_stop_data_taking_thread();
    void on_busy_drawing();
    void on_free_to_draw();

signals:
    // drawing signals calling back to QCstmGLVisualization
    void reload_all_layers();
    void reload_layer(int);
    void data_taking_finished(int);

    void dataReady(int layer);


    void scoring_sig(int nFramesReceived, int nFramesKept, int lost_frames,
                     int lost_packets, int frames_count, int mpx3clock_stops, bool misaligned);

};

#endif

