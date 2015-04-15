#ifndef QCSTMCONFIGMONITORING_H
#define QCSTMCONFIGMONITORING_H

#include <QWidget>
#include "mpx3gui.h"

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

private:
  Ui::QCstmConfigMonitoring *ui;

};

#endif // QCSTMCONFIGMONITORING_H
