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
class QCstmConfigMonitoring;



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
    vector<int> _MPX3RX_ORIENTATION = vector< int > {Dataset::orientationTtBRtL, Dataset::orientationBtTLtR, Dataset::orientationBtTLtR, Dataset::orientationTtBRtL};
    vector<QPoint> _MPX3RX_LAYOUT = vector<QPoint> {QPoint(0, 1), QPoint(1, 1), QPoint(1, 0), QPoint(0, 0)};
    int mode = 0;
    QApplication * _coreApp;
    Ui::Mpx3GUI * _ui;

    Mpx3Config * config;
    Dataset * workingSet;
    Dataset * originalSet;

    SpidrDaq * _spidrdaq = nullptr;
    bool _armedOk = true; // it won't let the application go into the event loop if set to false

    QVector<Gradient*>  gradients;
    //QVector<istogram*> hists;
    void updateHistogram(int layer);

public:

    Mpx3Config* getConfig();
    Dataset* getDataset(){return workingSet;}
    Dataset* getOriginalDataset(){return originalSet;}
    bool isArmedOk(){return _armedOk;}

    void saveOriginalDataset();
    void rewindToOriginalDataset();
    void setWindowWidgetsStatus(win_status s = win_status::startup);

    QCstmEqualization * getEqualization();
    QCstmGLVisualization * getVisualization();
    QCstmDacs * getDACs();
    QCstmConfigMonitoring * getConfigMonitoring();

    SpidrController * GetSpidrController();
    SpidrDaq * GetSpidrDaq(){ return _spidrdaq; }
    void addFrame(int *frame, int index, int layer);
    Gradient* getGradient(int index);
    void resize(int x, int y);
    //histogram* getHist(int index){return hists[index];}

    vector<int> getOrientation() { return _MPX3RX_ORIENTATION; }
    vector<QPoint> getLayout() { return _MPX3RX_LAYOUT; }

    QPoint  getSize();
    void getSize(int *x, int *y);
    int getMode(){return mode;}
    int getX();
    int getY();
    int getPixelAt(int x, int y, int layer);
    int getFrameCount();

    int XYtoX(int x, int y, int dimX) { return y * dimX + x; };

    void establish_connection();

signals:
    void dataChanged();
    void data_cleared();
    void hist_added(int);
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
    void sizeChanged(int, int);
    void open_data_failed();

public slots:
    void addLayer(int* data);
    void addLayer(int* data, int layer);
    void generateFrame(); //Debugging function to generate data when not connected
    void clear_data();
    void save_data();
    void open_data(bool saveOriginal = true);
    void set_mode_integral();
    void set_mode_normal();
    void clear_configuration();
    void set_summing(bool);
    void save_config();
    void load_config();
    void onConnectionStatusChanged(bool);

private slots:
    void LoadEqualization();
    void on_actionExit_triggered();
    void on_actionConnect_triggered();
};


#endif // MPX3GUI_H
