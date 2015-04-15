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
class Mpx3Config;

#include "dataset.h"
#include "gradient.h"
#include "histogram.h"
#include "mpx3eq_common.h"
#include "qcstmvoxeltab.h"
#include "mpx3config.h"

#define __matrix_size_x 256
#define __matrix_size_y 256
class Mpx3Config;
class QCustomPlot;
class SpidrController;
class SpidrDaq;
//class DACs;
class QCstmDacs;
class ThlScan;
class BarChart;
class BarChartProperties;
class Mpx3Equalization;

namespace Ui {
class Mpx3GUI;
}

class Mpx3GUI : public QMainWindow {

	Q_OBJECT

public:

	explicit Mpx3GUI(QApplication * coreApp, QWidget *parent = 0);
	~Mpx3GUI();
	void SetupSignalsAndSlots();
	Ui::Mpx3GUI * GetUI() { return _ui; }

private:
	int mode = 0;
	QApplication * _coreApp;
	Ui::Mpx3GUI * _ui;
	Mpx3Config *config;

	// Each object here deals with one tab of the
	// Equalization
	Mpx3Equalization * _equalization = nullptr;
	// DACs
	//DACs * _dacs = nullptr;

	// This helps interconnecting the different modules
	SpidrController * _spidrcontrol = nullptr;
	SpidrDaq * _spidrdaq = nullptr;

	Dataset *workingSet;
	QVector<Gradient*>  gradients;
	QVector<histogram*> hists;
public:
	Mpx3Config* getConfig();
	Dataset* getDataset(){return workingSet;}
	Mpx3Equalization* getEqualization(){return _equalization;}
	SpidrController * GetSpidrController(){ return _spidrcontrol; }
	SpidrDaq * GetSpidrDaq(){ return _spidrdaq; }
	void addFrame(int *frame);
	void addFrames(QVector<int*> frames);
	Gradient* getGradient(int index);
	int* getFrame(int index){
	  return workingSet->getFrame(index);
	}
	histogram* getHist(int index){
	  if(-1 == index)
	    index = (int)hists.size()-1;
	  return hists[index];
	}

	QPoint  getSize();
	void getSize(int *x, int *y);
	int getX();
	int getY();
	int getPixelAt(int x, int y, int layer);
	int getFrameCount();

signals:
	void dataChanged();
	void data_cleared();
	void hist_added();
	void frame_added();
	void frame_changed();
	void frames_reload();
	void active_frame_changed(int);
	void availible_gradients_changed(QStringList gradients);
	void gradient_added(QString gradient);	
	void ConnectionStatusChanged(bool); //TODO: emit false when connection is lost for whatever reason.
	void summing_set(bool);

	public slots:
	void generateFrame(); //Debugging function to generate data when not connected
	void establish_connection();
	void clear_data();
	void save_data();
	void open_data();
	void set_mode_integral();
	void set_mode_normal();
	void clear_configuration();
	void set_summing(bool);
	void start_data_taking();
public slots:
	void set_active_frame(int);
private slots:
	void LoadEqualization();
	void on_openfileButton_clicked();
};


#endif // MPX3GUI_H
