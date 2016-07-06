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

private slots:
    void on_takeDataPushButton_clicked();

private:
    Ui::QCstmDQE *ui;
    Mpx3GUI * _mpx3gui;


signals:
    void start_takingData();

};

#endif // QCSTMDQE_H
