/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#ifndef SPIDRMPX3EQ_H
#define SPIDRMPX3EQ_H

//#include <QImage>
#include <QMainWindow>

#include <iostream>
#include <vector>
using namespace std;

#include "mpx3eq_common.h"

#define __matrix_size_x 256
#define __matrix_size_y 256

class QCustomPlot;
class SpidrController;
class SpidrDaq;
class DACs;
class ThlScan;
class BarChart;

namespace Ui {
	class SpidrMpx3Eq;
}

class SpidrMpx3Eq : public QMainWindow {
	Q_OBJECT

public:
	explicit SpidrMpx3Eq(QWidget *parent = 0);
	~SpidrMpx3Eq();

	void timerEvent( QTimerEvent * );
	void PrintFraction(int * buffer, int size, int first_last);
	int GetNPixelsActive(int * buffer, int size, verblev verbose);
	pair<int, int> XtoXY(int X, int dimX);

private:

	Ui::SpidrMpx3Eq *ui;
	SpidrController * _spidrcontrol;
	SpidrDaq * _spidrdaq;

	// Drawing object
	//QCustomPlot * _customPlot;
	BarChart * _chart;

	// Object in charge of performing Thl scans
	ThlScan * _tscan;
	// DACs
	DACs * _dacs;

private slots:

	void StartEqualization();
	void Connect();

};


#endif // SPIDRMPX3EQ_H
