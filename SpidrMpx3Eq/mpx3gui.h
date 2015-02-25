/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#ifndef MPX3GUI_H
#define MPX3GUI_H

//#include <QImage>
#include <QMainWindow>

#include <iostream>
#include <vector>

#include <qcustomplot.h>

using namespace std;

#include "histogram.h"
#include "mpx3eq_common.h"

#define __matrix_size_x 256
#define __matrix_size_y 256

class QCustomPlot;
class SpidrController;
class SpidrDaq;
class DACs;
class ThlScan;
class BarChart;
class BarChartProperties;
class Mpx3Equalization;

namespace Ui {
class Mpx3GUI;
}

class ModuleConnection {

public:
	ModuleConnection(){};
	~ModuleConnection(){};
	//
	void SetDACs(DACs * p) { _dacs = p ;};
	void SetEqualization(Mpx3Equalization * p) { _equalization = p; };
	// A pointer to the other tabs
	DACs * GetDACs() { return _dacs; };
	Mpx3Equalization * GetEqualization() { return _equalization; };

private:

	// Equalization
	Mpx3Equalization * _equalization;
	// DACs
	DACs * _dacs;
};

class Mpx3GUI : public QMainWindow {

	Q_OBJECT

public:

	explicit Mpx3GUI(QApplication * coreApp, QWidget *parent = 0);
	~Mpx3GUI();

	void timerEvent( QTimerEvent * );

private:

	QApplication * _coreApp;
	Ui::Mpx3GUI * _ui;

	// Each object here deals with one tab of the
	// Equalization
	Mpx3Equalization * _equalization;
	// DACs
	DACs * _dacs;
	// This helps interconnecting the different modules
	ModuleConnection * _moduleConn;

};


#endif // MPX3GUI_H
