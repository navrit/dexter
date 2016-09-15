#ifndef QCSTMSTEPPERMOTOR_H
#define QCSTMSTEPPERMOTOR_H

#include <QWidget>

namespace Ui {
class qcstmStepperMotor;
}

class qcstmStepperMotor : public QWidget
{
    Q_OBJECT

public:
    explicit qcstmStepperMotor(QWidget *parent = 0);
    ~qcstmStepperMotor();

private:
    Ui::qcstmStepperMotor *ui;
};

#endif // QCSTMSTEPPERMOTOR_H
