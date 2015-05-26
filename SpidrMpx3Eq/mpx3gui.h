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
class QCstmEqualization;
class QCstmGLVisualization;

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
	vector<int> _MPX3RX_ORIENTATION = {{Dataset::orientationTtBRtL, Dataset::orientationBtTLtR, Dataset::orientationBtTLtR, Dataset::orientationTtBRtL}};
	vector<QPoint> _MPX3RX_LAYOUT = {{QPoint(0,1), QPoint(1,1),QPoint(1,0), QPoint(0,0)}};
	int mode = 0;
	QApplication * _coreApp;
	Ui::Mpx3GUI * _ui;

	Mpx3Config *config;
	//Dataset *workingSet;
	QVector<Dataset> workingSet;

	SpidrDaq * _spidrdaq = nullptr;

	QVector<Gradient*>  gradients;
	//QVector<istogram*> hists;
	void updateHistogram(int layer);
public:
	Mpx3Config* getConfig();
	Dataset* getDataset(){return &workingSet.last();}
	Dataset* getDataset(int index){return &workingSet[index];}
	QCstmEqualization * getEqualization();
	QCstmGLVisualization * getVisualization();

	SpidrController * GetSpidrController();
	SpidrDaq * GetSpidrDaq(){ return _spidrdaq; }
	void addFrame(int *frame, int index, int layer);
	Gradient* getGradient(int index);
	//histogram* getHist(int index){return hists[index];}

	void addDataset(Dataset &newSet){workingSet.append(newSet);}

	QPoint  getSize();
	void getSize(int *x, int *y);
	int getX();
	int getY();
	int getPixelAt(int x, int y, int layer);
	int getFrameCount();
	void addSlice();

signals:
	void dataChanged();
	void data_cleared();
	void hist_added(int);
	void slice_added(int);
	void hist_changed(int);
	/*void reload_layer(int layer);
	void frame_added(int layer);
	void frame_changed(int layer);
	void frames_reload();*/
	void active_frame_changed(int);
	void availible_gradients_changed(QStringList gradients);
	void gradient_added(QString gradient);	
	void ConnectionStatusChanged(bool); //TODO: emit false when connection is lost for whatever reason.
	void summing_set(bool);
	void reload_layer(int layer);
	void reload_all_layers();

	public slots:
	void addLayer(int* data);
	void addLayer(int* data, int layer);
	void generateFrame(); //Debugging function to generate data when not connected
	void establish_connection();
	void clear_data();
	void save_data();
	void open_data();
	void set_mode_integral();
	void set_mode_normal();
	void clear_configuration();
	void set_summing(bool);
	void save_config();
	void load_config();
private slots:
	void LoadEqualization();
	void on_openfileButton_clicked();
};


#endif // MPX3GUI_H
