#ifndef QCSTMCT_H
#define QCSTMCT_H

#include <QWidget>
#include "mpx3gui.h"
#include "gradient.h"
#include <QElapsedTimer>
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
  void setGradient(int index);

private:

  Ui::QCstmCT *ui;
  Mpx3GUI * _mpx3gui;
  void resetMotor();
  void setAcceleration(double acceleration);
  void setSpeed(double speed);
  void setTargetPosition(double position);
  void motor_goToTarget();
  void update_timeGUI(int i, int numberOfProjections);
  void startCT(); // MAIN FUNCTION
  void stopCT();  // MAIN INTERRUPT
  bool _stop = false;

  bool activeMotors = false;

signals:
  void sig_connectToMotors( bool );

public slots:
  void slot_connectedToMotors();

private slots:

  void  on_CTPushButton_clicked();
  //void rotatePushButton_clicked();

};

#endif // QCSTMCT_H
