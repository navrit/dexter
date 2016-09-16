#ifndef QCSTMSTEPPERMOTOR_H
#define QCSTMSTEPPERMOTOR_H

#include <QWidget>
#include <ui_qcstmsteppermotor.h>
#include "mpx3gui.h"

namespace Ui {
class QCstmStepperMotor;
}

class QCstmStepperMotor : public QWidget
{
    Q_OBJECT

public:
    explicit QCstmStepperMotor(QWidget *parent = 0);
    ~QCstmStepperMotor();
    Ui::QCstmStepperMotor *GetUI(){ return ui; }
    void SetMpx3GUI(Mpx3GUI *p);

private:
    Ui::QCstmStepperMotor *ui;
    Mpx3GUI * _mpx3gui;
};

#endif // QCSTMSTEPPERMOTOR_H



