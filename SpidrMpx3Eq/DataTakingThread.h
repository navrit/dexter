/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2015.
 */

#ifndef DATATAKINGTHREAD_H
#define DATATAKINGTHREAD_H


#include <QThread>
#include <QVector>

#include "mpx3gui.h"

#include <iostream>

using namespace std;

class QCstmGLVisualization;
class SpidrController;
class SpidrDaq;

class DataTakingThread : public QThread {

	Q_OBJECT

public:
	explicit DataTakingThread(Mpx3GUI *, QCstmGLVisualization *);
	void ConnectToHardware();
	void SeparateThresholds(int * data, int size, QVector<int> * th0, QVector<int> * th2, QVector<int> * th4, QVector<int> * th6, int sizeReduced);
	pair<int, int> XtoXY(int X, int dimX);
	int XYtoX(int x, int y, int dimX) { return y * dimX + x; }

private:

	void run();

	Mpx3GUI * _mpx3gui;
	QCstmGLVisualization * _vis;

	// IP source address (SPIDR network interface)
	int _srcAddr;

signals:
	// drawing signals calling back to QCstmGLVisualization
	void reload_all_layers();
	void reload_layer(int);

};

#endif

