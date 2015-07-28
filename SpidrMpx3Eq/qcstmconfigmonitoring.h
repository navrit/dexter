#ifndef QCSTMCONFIGMONITORING_H
#define QCSTMCONFIGMONITORING_H

#include <QWidget>
#include "mpx3gui.h"

class StepperMotorController;

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

private slots:
  void on_SaveButton_clicked();

  void on_LoadButton_clicked();

  void on_ipLineEdit_editingFinished();

  void on_ColourModeCheckBox_toggled(bool checked);

  // Stepper
  void on_stepperMotorCheckBox_toggled(bool checked);
  void on_motorGoToTargetButton_clicked();

private:
  Ui::QCstmConfigMonitoring *ui;
  int _timerId;

  StepperMotorController * _stepper;

};

#endif // QCSTMCONFIGMONITORING_H
