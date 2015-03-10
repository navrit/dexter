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

private:
  Ui::QCstmEqualization *ui;
  Ui::Mpx3GUI *sharedGUI;
};

#endif // QCSTMEQUALIZATION_H
