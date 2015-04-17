#ifndef QCSTMEQUALIZATION_H
#define QCSTMEQUALIZATION_H

#include <QWidget>
#include "mpx3gui.h"

namespace Ui {
  class QCstmEqualization;
}

class QCstmEqualization : public QWidget
{
  Q_OBJECT

public:
  explicit QCstmEqualization(QWidget *parent = 0);
  ~QCstmEqualization();
  void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; };

private:
  Ui::QCstmEqualization *ui;
  Ui::Mpx3GUI *sharedGUI;
  Mpx3GUI * _mpx3gui;

};

#endif // QCSTMEQUALIZATION_H
