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

class ModuleConnection : public QObject {

	Q_OBJECT

public:

	ModuleConnection(){};
	~ModuleConnection(){};
	SpidrController * GetSpidrController(){ return _spidrcontrol; };
	SpidrDaq * GetSpidrDaq(){ return _spidrdaq; };

private:

	// Connectivity
	SpidrController * _spidrcontrol;
	SpidrDaq * _spidrdaq;

private slots:

	void Connection();

	signals:

void ConnectionStatusChanged();


};

class Mpx3GUI : public QMainWindow {

	Q_OBJECT

public:

	explicit Mpx3GUI(QApplication * coreApp, QWidget *parent = 0);
	~Mpx3GUI();
	void SetupSignalsAndSlots();
	Ui::Mpx3GUI * GetUI() { return _ui; };
	ModuleConnection * GetModuleConnection(){ return _moduleConn; };

	void timerEvent( QTimerEvent * );

private:

	QApplication * _coreApp;
	Ui::Mpx3GUI * _ui;

	//Define  some UI variable shared by all the modules.
	QMap<QString, QCPColorGradient> heatmapMap;
	unsigned currentFrame;

	// Each object here deals with one tab of the
	// Equalization
	Mpx3Equalization * _equalization;
	// DACs
	DACs * _dacs;
	// This helps interconnecting the different modules
	ModuleConnection * _moduleConn;

	//Data Stores
	int **data = nullptr;
	unsigned nData =0, *ny = nullptr, *nx = nullptr;
	histogram **hists = nullptr;

	private slots:

	void LoadEqualization();
	void on_openfileButton_clicked();



};


#endif // MPX3GUI_H
