#ifndef QCSTMCONFIGMONITORING_H
#define QCSTMCONFIGMONITORING_H

#include <QWidget>
#include "mpx3gui.h"

#include <QCameraInfo>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QtWidgets>

//class StepperMotorController;
//class ConfigStepperThread; // defined in this file at the bottom

namespace Ui {
class QCstmConfigMonitoring;
}

class QCstmConfigMonitoring : public QWidget
{

    Q_OBJECT

    Mpx3GUI * _mpx3gui;

public:
    void SetMpx3GUI(Mpx3GUI * p);
    void widgetInfoPropagation();
    explicit QCstmConfigMonitoring(QWidget *parent = 0);
    ~QCstmConfigMonitoring();
    Ui::QCstmConfigMonitoring * getUI() { return ui; }

    void timerEvent( QTimerEvent * );

    void setWindowWidgetsStatus(win_status s = win_status::startup);


//    StepperMotorController * getMotorController() { return _stepper; }
//    void activeInGUI();
//    void activateItemsGUI();
//    void deactivateItemsGUI();

//    void angleModeGUI();
//    void stepsModeGUI();

    void cameraSetup();
    void cameraOn();
    void cameraOff();
    bool cameraSearch(int indexRequest = -1);
    void cameraResize();

    void readMonitoringInfo();

    unsigned int getPixelDepthFromIndex(int indx);
    unsigned int getPixelDepth12BitsIndex() { return __pixelDepth12BitsIndex; }

public slots:

    void OperationModeSwitched(int indx);
    void on_taking_data_gui();
    void on_idling_gui();

private slots:
    void ConnectionStatusChanged(bool);

    void setPixelDepthByIndex(int newValIndx);
    void pixelDepthChangedByValue(int val);

    void setTriggerModeByIndex(int newValIndx);
    void triggerModeChangedByValue(int val);

    void setCsmSpmByIndex(int newValIndx);
    void csmSpmChangedByValue(int val);

    void IpAddressEditFinished();

    void nTriggersEdited();
    void ContRWFreqEdited();
    void TriggerLengthEdited();
    void TriggerDowntimeEdited();

    void on_SaveButton_clicked();

    void on_LoadButton_clicked();

    void on_ColourModeCheckBox_toggled(bool checked);

    void on_tempReadingActivateCheckBox_toggled(bool checked);

    void on_readOMRPushButton_clicked();


//    ////////////////////////////////////////////////////////////
//    // Stepper
//    void on_stepperMotorCheckBox_toggled(bool checked);
//    void on_stepperUseCalibCheckBox_toggled(bool checked);
//    void on_motorGoToTargetButton_clicked();
//    void on_motorResetButton_clicked();
//    void on_stepperSetZeroPushButton_clicked();
//    //void ConfigCalibAngle1Changed(double);

//    // dial
//    void motorDialReleased();
//    void motorDialMoved(int);
//    // spins
//    void setAcceleration(double);
//    void setSpeed(double);
//    void setCurrentILimit(double);

    ////////////////////////////////////////////////////////////
    // Camera
    void on_cameraCheckBox_toggled(bool checked);
    void changeCamera(int);

//    void on_motorTestButton_clicked();
//    void stepperGotoTargetFinished();
    void biasVoltageChanged();


    void on_checkBox_sendDataToIP_toggled(bool checked);
    void on_sendDataToIPlineEdit_editingFinished();

private:
    Ui::QCstmConfigMonitoring *ui;
    int _timerId;

//    StepperMotorController * _stepper;
//    ConfigStepperThread * _stepperThread;

    bool _cameraOn;
    QCamera * _camera;
    QCameraViewfinder * _viewfinder;
    QCameraImageCapture * _imageCapture;
    int _cameraId;
//    QVector<double> m_stepperTestSequence;
//    int m_stepperTestCurrentStep = 0;

    // Some constants in the configuration (MPX3 manual pag. 18)
    vector<unsigned int> __pixelDepthMap; // = { 1 , 6 , 12 , 24 };
    const unsigned int __pixelDepth12BitsIndex = 2;

    vector<unsigned int> __triggerModeMap;

    vector<unsigned int> __csmSpmMap;

};

//class ConfigStepperThread : public QThread {

//    Q_OBJECT

//public:
//    explicit ConfigStepperThread(Mpx3GUI *, Ui::QCstmConfigMonitoring  *, QCstmConfigMonitoring *);
//    void ConnectToHardware( );

//private:

//    void run();

//    Mpx3GUI * _mpx3gui;
//    Ui::QCstmConfigMonitoring * _ui;
//    QCstmConfigMonitoring * _stepperController;
//    //public slots:

//    //signals:

//};


#endif // QCSTMCONFIGMONITORING_H
