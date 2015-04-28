#ifndef QCSTMTHRESHOLD_H
#define QCSTMTHRESHOLD_H

#include "mpx3gui.h"

#include <QWidget>


namespace Ui {
  class QCstmThreshold;
}

class QCstmThreshold : public QWidget
{
  Q_OBJECT

public:
  explicit QCstmThreshold(QWidget *parent = 0);
  ~QCstmThreshold();
 void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; }
 void SetupSignalsAndSlots();
 void GUIDefaults();

private:
  Ui::QCstmThreshold *ui;
  // Connectivity between modules
  Mpx3GUI * _mpx3gui;
  void addFrame(QPoint offset, int layer, QVector<int> data);

private slots:

void StartCalibration();

};

#endif // QCSTMTHRESHOLD_H
