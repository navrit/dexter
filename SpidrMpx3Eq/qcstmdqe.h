#ifndef QCSTMDQE_H
#define QCSTMDQE_H

#include <QWidget>
#include "mpx3gui.h"

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
    void SetMpx3GUI(Mpx3GUI *p);
    void setLayer(int threshold){ _currentlayer = threshold;}
    void setRegion(QPoint pixel_begin, QPoint pixel_end);
    void plotMTF();
    void plotESF();
    void plotFitESF();
    void plotEdge(QPoint ab);

private slots:
    void on_takeDataPushButton_clicked();

    void on_comboBox_currentIndexChanged(const QString &arg1);

    void on_fitPushButton_clicked();

private:
    Ui::QCstmDQE *ui;
    Mpx3GUI * _mpx3gui;
    int _currentlayer;
    QPoint _beginpix, _endpix;
    QVector<QVector<double> > _data; //Contains a vector for the distances and one for the pixel values of the esf data.

signals:
    void start_takingData();

};

#endif // QCSTMDQE_H
