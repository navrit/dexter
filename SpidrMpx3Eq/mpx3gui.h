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

namespace Ui {
	class Mpx3GUI;
}

class Mpx3GUI : public QMainWindow {
	Q_OBJECT

public:
	explicit Mpx3GUI(QApplication * coreApp, QWidget *parent = 0);
	~Mpx3GUI();

	void timerEvent( QTimerEvent * );
	void PrintFraction(int * buffer, int size, int first_last);
	int GetNPixelsActive(int * buffer, int size, verblev verbose);
	pair<int, int> XtoXY(int X, int dimX);

private:
	QStringList files;
	QMap<QString, QCPColorGradient> heatmapMap;
	QApplication * _coreApp;
	Ui::Mpx3GUI * _ui;
	SpidrController * _spidrcontrol;
	SpidrDaq * _spidrdaq;

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

	void on_heatmapCombobox_currentIndexChanged(const QString &arg1);
	void on_openfileButton_clicked();
};


#endif // MPX3GUI_H
