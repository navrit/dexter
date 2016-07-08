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
    Ui::QCstmDQE * GetUI(){ return ui; };
    void SetMpx3GUI(Mpx3GUI *p);
    void setThreshold(int threshold){ _currentThreshold = threshold;}
    void setPixels(QPoint pixel_begin, QPoint pixel_end) {_pixel_begin = pixel_begin; _pixel_end = pixel_end;}
    void CalcMTF();

private slots:
    void on_takeDataPushButton_clicked();

private:
    Ui::QCstmDQE *ui;
    Mpx3GUI * _mpx3gui;
    int _currentThreshold;
    QPoint _pixel_begin, _pixel_end;

signals:
    void start_takingData();

};

#endif // QCSTMDQE_H
