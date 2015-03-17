/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#ifndef MPX3GUI_H
#define MPX3GUI_H

//#include <QImage>
#include <QMainWindow>

#include <QVector>
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
/*
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
*/
class Mpx3GUI : public QMainWindow {

	Q_OBJECT

public:

	explicit Mpx3GUI(QApplication * coreApp, QWidget *parent = 0);
	~Mpx3GUI();
	void SetupSignalsAndSlots();
	Ui::Mpx3GUI * GetUI() { return _ui; };
	//ModuleConnection * GetModuleConnection(){ return _moduleConn; }

	void timerEvent( QTimerEvent * );

private:
	int mode = 0;
	QApplication * _coreApp;
	Ui::Mpx3GUI * _ui;

	//Define  some UI variable shared by all the modules.
	QMap<QString, QCPColorGradient> gradients;
	unsigned currentFrame;

	// Each object here deals with one tab of the
	// Equalization
	Mpx3Equalization * _equalization = nullptr;
	// DACs
	DACs * _dacs = nullptr;
	// This helps interconnecting the different modules
	//ModuleConnection * _moduleConn;
	SpidrController * _spidrcontrol = nullptr;
	SpidrDaq * _spidrdaq = nullptr;

	//Data Stores
	vector<int*> data;
	/*QVector<int> dataSize;*/
	//unsigned nData =0;
	int ny = 256;
	int nx = 256;
	vector<histogram*> hists;
public:
	Mpx3Equalization* getEqualization(){return _equalization;}
	SpidrController * GetSpidrController(){ return _spidrcontrol; }
	SpidrDaq * GetSpidrDaq(){ return _spidrdaq; }
	void generateFrame(); //Debugging function to generate data when not connected
	void addFrame(int *frame);
	int* getFrame(int index){
	  if(-1 == index)
	    index = (int)data.size() - 1;//data.count()-1;
	  return data[index];
	}
	histogram* getHist(int index){
	  if(-1 == index)
	    index = (int)hists.size()-1;
	  return hists[index];
	}

	void getSize(QPoint *size);
	void getSize(int *x, int *y);
	int getX();
	int getY();
	int getFrameCount();

	QCPColorGradient getGradient(QString index);

signals:
	void dataChanged();
	void frame_added();
	void frame_changed();
	void availible_gradients_changed(QStringList gradients);
	void gradient_added(QString gradient);	
	void ConnectionStatusChanged(bool); //TODO: emit false when connection is lost for whatever reason.
public slots:
	void establish_connection();
	void save_data();
	void open_data();
	void set_mode_integral();
	void set_mode_normal();
private slots:
	void LoadEqualization();
	void on_openfileButton_clicked();
};


#endif // MPX3GUI_H
