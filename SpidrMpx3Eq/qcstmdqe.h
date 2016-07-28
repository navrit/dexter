#ifndef QCSTMDQE_H
#define QCSTMDQE_H

#include <QWidget>
#include "mpx3gui.h"
#include "ui_qcstmdqe.h"
#include "qcstmglvisualization.h"

namespace Ui {
class QCstmDQE;
}

class QCstmDQE : public QWidget
{
    Q_OBJECT

public:
    explicit QCstmDQE(QWidget *parent = 0);
    ~QCstmDQE();
    //Ui::QCstmDQE * GetUI(){ return ui; };
    void SetMpx3GUI(Mpx3GUI *p){ _mpx3gui = p;}
    void setSelectedThreshold(int threshold){ ui->comboBox->setCurrentText(QString("Threshold %1").arg(threshold)); }
    //void updateSelectedThreshold(){ui->comboBox->setCurrentText( QString("Threshold %1").arg(_mpx3gui->getVisualization()->getActiveThreshold()) ); }
    void setRegion(QPoint pixel_begin, QPoint pixel_end);
    void plotESF();
    void setParams(parameter_vector params){_params = params;}
    void setxStart(double start){_xstart = start;}
    void setPlotLength(int length){_plotrange = length;}


private slots:
    void on_takeDataPushButton_clicked();

    void on_comboBox_currentIndexChanged(const QString &arg1);

    void on_fitPushButton_clicked();

    void on_plotPSFpushButton_clicked();

    void on_loadDataPushButton_clicked();

private:
    Ui::QCstmDQE *ui;
    Mpx3GUI * _mpx3gui;
    //int _currentThreshold;
    QPoint _beginpix, _endpix;
    QVector<QVector<double> > _data; //Contains a vector for the distances and one for the pixel values of the esf data.
    parameter_vector _params;
    double _xstart;
    double _plotrange;
    double _stepsize = 0.2; //Specify the distance between datapoints of the plot in pixels.

    //functions:
    QVector<QVector<double> > calcESFfitData();
    QVector<QVector<double> > calcPSFdata();
    void plotMTF();

    void plotFitESF();
    void plotPSF();
    void plotEdge(QPoint ab);

signals:
    void start_takingData();
    void open_data(bool, bool, QString);

};

#endif // QCSTMDQE_H
