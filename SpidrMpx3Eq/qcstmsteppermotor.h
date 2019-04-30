#ifndef QCSTMSTEPPERMOTOR_H
#define QCSTMSTEPPERMOTOR_H

#include <QWidget>
#include <QThread>
#include <ui_qcstmsteppermotor.h>
#include "mpx3eq_common.h"

class Mpx3GUI;
class StepperMotorController;
class ConfigStepperThread; // defined in this file at the bottom

namespace Ui {
    class QCstmStepperMotor;
}

class QCstmStepperMotor : public QWidget
{
    Q_OBJECT

public:

    explicit QCstmStepperMotor(QWidget *parent = nullptr);
    ~QCstmStepperMotor();
    Ui::QCstmStepperMotor *GetUI(){ return ui; }
    void SetMpx3GUI(Mpx3GUI *p);

    void setWindowWidgetsStatus(win_status s = win_status::startup );

    StepperMotorController * getMotorController() { return _stepper; }
    void activeInGUI();
    void activateItemsGUI();
    void deactivateItemsGUI();

    void angleModeGUI();
    void stepsModeGUI();

private:

    Ui::QCstmStepperMotor *ui = nullptr;
    Mpx3GUI * _mpx3gui = nullptr;

    StepperMotorController * _stepper = nullptr;
    ConfigStepperThread * _stepperThread = nullptr;

    QVector<double> m_stepperTestSequence;
    int m_stepperTestCurrentStep = 0;

signals:
    //! Status bar signal function
    void sig_statusBarAppend(QString mess, QString colorString);

    void sig_motorsConnected();

public slots:
    void ConnectionStatusChanged(bool);

    //! Stepper slots
    void on_stepperUseCalibCheckBox_toggled(bool checked);
    void on_motorResetButton_clicked();
    void on_stepperSetZeroPushButton_clicked();
    void on_stepperMotorCheckBox_toggled(bool checked);
    void on_motorGoToTargetButton_clicked();

    // Dial
    void motorDialReleased();
    void motorDialMoved(int);

    // Spins
    void setAcceleration(double);
    void setSpeed(double);
    void setCurrentILimit(double);

    void on_motorTestButton_clicked();
    void stepperGotoTargetFinished();
};

class ConfigStepperThread : public QThread {

    Q_OBJECT

public:
    explicit ConfigStepperThread(Mpx3GUI *, Ui::QCstmStepperMotor *, QCstmStepperMotor *);
    void ConnectToHardware( );

    bool reachedTarget = false;

private:

    void run();

    Mpx3GUI * _mpx3gui = nullptr;
    Ui::QCstmStepperMotor * _ui = nullptr;
    QCstmStepperMotor * _stepperController = nullptr;
};

#endif // QCSTMSTEPPERMOTOR_H
