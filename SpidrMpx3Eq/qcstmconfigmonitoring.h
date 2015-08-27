#ifndef QCSTMCONFIGMONITORING_H
#define QCSTMCONFIGMONITORING_H

#include <QWidget>
#include "mpx3gui.h"

#include <QCameraInfo>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QtWidgets>

class StepperMotorController;
class ConfigStepperThread; // defined in this file at the bottom

namespace Ui {
  class QCstmConfigMonitoring;
}

class QCstmConfigMonitoring : public QWidget
{
  Q_OBJECT
  Mpx3GUI * _mpx3gui;
public:
  void SetMpx3GUI(Mpx3GUI * p);
  explicit QCstmConfigMonitoring(QWidget *parent = 0);
  ~QCstmConfigMonitoring();
  void timerEvent( QTimerEvent * );

  StepperMotorController * getMotorController() { return _stepper; }
  void activeInGUI();
  void activateItemsGUI();
  void deactivateItemsGUI();

  void angleModeGUI();
  void stepsModeGUI();

  void cameraSetup();
  void cameraOn();
  void cameraOff();
  void cameraSearch(int indexRequest = -1);
  void cameraResize();


private slots:
  void on_SaveButton_clicked();

  void on_LoadButton_clicked();

  void on_ipLineEdit_editingFinished();

  void on_ColourModeCheckBox_toggled(bool checked);

  void on_readOMRPushButton_clicked();

  ////////////////////////////////////////////////////////////
  // Stepper
  void on_stepperMotorCheckBox_toggled(bool checked);
  void on_stepperUseCalibCheckBox_toggled(bool checked);
  void on_motorGoToTargetButton_clicked();
  void on_motorResetButton_clicked();
  void on_stepperSetZeroPushButton_clicked();
  //void ConfigCalibAngle1Changed(double);

  // dial
  void motorDialReleased();
  void motorDialMoved(int);
  // spins
  void setAcceleration(double);
  void setSpeed(double);

  ////////////////////////////////////////////////////////////
  // Camera
  void on_cameraCheckBox_toggled(bool checked);
  void changeCamera(int);

private:
  Ui::QCstmConfigMonitoring *ui;
  int _timerId;

  StepperMotorController * _stepper;
  ConfigStepperThread * _stepperThread;

  bool _cameraOn;
  QCamera * _camera;
  QCameraViewfinder * _viewfinder;
  QCameraImageCapture * _imageCapture;
  int _cameraId;

};

class ConfigStepperThread : public QThread {

	Q_OBJECT

public:
	explicit ConfigStepperThread(Mpx3GUI *, Ui::QCstmConfigMonitoring  *, QCstmConfigMonitoring *);
	void ConnectToHardware( );

private:

	void run();

	Mpx3GUI * _mpx3gui;
	Ui::QCstmConfigMonitoring * _ui;
	QCstmConfigMonitoring * _stepperController;
//public slots:

//signals:

};


#endif // QCSTMCONFIGMONITORING_H
