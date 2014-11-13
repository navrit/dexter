/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#ifndef SPIDRMPX3EQ_H
#define SPIDRMPX3EQ_H

#include "ui_spidrmpx3eq.h"
#include <QImage>
#include <QMainWindow>

#include "SpidrController.h"
#include "SpidrDaq.h"
#include "dacsdefs.h"

#include "barchart.h"
#include "ThlScan.h"

#include "mpx3eq_common.h"

#include <vector>

#define __matrix_size_x 256
#define __matrix_size_y 256

using namespace std;

namespace Ui {
class SpidrMpx3Eq;
}

class QCustomPlot;

class SpidrMpx3Eq : public QMainWindow, Ui_SpidrMpx3Eq{
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

private slots:

void StartEqualization();
void Connect();

};

#endif // SPIDRMPX3EQ_H
