/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#ifndef MPX3EQUALIZATION_H
#define MPX3EQUALIZATION_H

#include "ui_mpx3gui.h"
#include "mpx3gui.h"
#include "mpx3defs.h"

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
class ModuleConnection;

class Mpx3Equalization : public QWidget {
	Q_OBJECT

public:
	explicit Mpx3Equalization();
	explicit Mpx3Equalization(QApplication * coreApp, Ui::Mpx3GUI * );
	~Mpx3Equalization();

	void timerEvent( QTimerEvent * );
	void PrintFraction(int * buffer, int size, int first_last);
	int GetNPixelsActive(int * buffer, int size, verblev verbose);
	pair<int, int> XtoXY(int X, int dimX);
	void SetupSignalsAndSlots();
	void SetLimits();
	void SetModuleConnection(ModuleConnection * p) { _moduleConn = p; };

private:

	// Connectivity between modules
	ModuleConnection * _moduleConn;

	QStringList files;
	QMap<QString, QCPColorGradient> heatmapMap;
	SpidrController * _spidrcontrol;
	SpidrDaq * _spidrdaq;

	QApplication * _coreApp;
	Ui::Mpx3GUI * _ui;

	int _deviceIndex;
	int _nTriggers;
	int _spacing;

	int **data = 0;
	unsigned *nx =0, *ny =0, nData =0;
	histogram **hists = 0;

	// Drawing object
	//QCustomPlot * _customPlot;
	//BarChart * _chart;

	// Object in charge of performing Thl scans
	ThlScan * _tscan;
	// DACs
	DACs * _dacs;

private slots:

	void StartEqualization();
	void Connect();
	void ChangeNTriggers(int);
	void ChangeDeviceIndex(int);
	void ChangeSpacing(int);

	void on_heatmapCombobox_currentIndexChanged(const QString &arg1);
	void on_openfileButton_clicked();
};


#endif // MPX3EQUALIZATION_H
