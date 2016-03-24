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
	explicit DataTakingThread(Mpx3GUI *, QCstmGLVisualization *);
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
    } datataking_score_info;

    datataking_score_info getScoreInfo() { return _score; }
    void rewindScoring();
    void setFramesRequested(int nf){ _score.framesRequested = nf; }
    void setMissingToCompleteJob(int nf) { _score.missingToCompleteJob = nf; }

private:

	void run();

	Mpx3GUI * _mpx3gui;
	QCstmGLVisualization * _vis;
	bool _stop;
	bool _canDraw;
    datataking_score_info _score;

	// IP source address (SPIDR network interface)
	int _srcAddr;

public slots:
	void on_stop_data_taking_thread();
	void on_busy_drawing();
	void on_free_to_draw();

signals:
	// drawing signals calling back to QCstmGLVisualization
	void reload_all_layers();
	void reload_layer(int);
	void data_taking_finished(int);
	void progress(int);
	void lost_packets(int);
	void fps_update(int);
    void overflow_update(int);

};

#endif

