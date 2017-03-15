#ifndef QCSTMCT_H
#define QCSTMCT_H

#include <QWidget>
#include "mpx3gui.h"
#include "gradient.h"
#include <QElapsedTimer>
#include <QDir>
//#include <stdio.h>
//#include <phidget21.h>

namespace Ui {
class QCstmCT;
}

class QCstmCT : public QWidget
{
    Q_OBJECT

public:

    explicit QCstmCT(QWidget *parent = 0);
    ~QCstmCT();
    Ui::QCstmCT *GetUI(){ return ui; }

    void SetMpx3GUI(Mpx3GUI *p);

private:

    Ui::QCstmCT *ui;
    Mpx3GUI * _mpx3gui;
    void resetMotor();
    void setAcceleration(double acceleration);
    void setSpeed(double speed);
    void setTargetPosition(double position);
    void motor_goToTarget();
    void update_timeGUI();
    void applyCorrection(QString correctionMethod);
    QString getCorrectionFile();
    QString correctionFilename;
    int  iteration = 0;
    double targetAngle = 0;
    float angleDelta = 0;
    int numberOfProjections = 0;
    QString getMotorPositionStatus();
    void startDataTakingThread();
    void startCT(); // MAIN FUNCTION
    void stopCT();  // MAIN INTERRUPT
    bool _stop = false;
    bool isMotorMoving = false;
    void doBHCorrection(QString corrFilename);
    QString CTfolder = QDir::homePath() + "/";

    bool activeMotors = false;

signals:
    void sig_connectToMotors( bool );

public slots:
    void slot_connectedToMotors();
    void slot_motorReachedTarget();
    void resumeCT();

private slots:

    void  on_CTPushButton_clicked();
    //void rotatePushButton_clicked();

};

#endif // QCSTMCT_H
