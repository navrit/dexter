#ifndef QCSTMDACS_H
#define QCSTMDACS_H

#include <QWidget>
#include "mpx3gui.h"

namespace Ui {
  class QCstmDacs;
}

class QCstmDacs : public QWidget
{
  Q_OBJECT

public:
  explicit QCstmDacs(QWidget *parent = 0);
  ~QCstmDacs();

private:
  Ui::QCstmDacs *ui;
  Ui::Mpx3GUI *sharedGUI;
};

#endif // QCSTMDACS_H
