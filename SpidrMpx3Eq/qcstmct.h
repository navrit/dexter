#ifndef QCSTMCT_H
#define QCSTMCT_H

#include <QWidget>
#include "mpx3gui.h"
#include "gradient.h"

namespace Ui {
  class QCstmCT;
}

class QCstmCT : public QWidget
{
  Q_OBJECT

public:

  explicit QCstmCT(QWidget *parent = 0);
  ~QCstmCT();
  Ui::QCstmCT * GetUI(){ return ui; };

  void SetMpx3GUI(Mpx3GUI *p);
  void setGradient(int index);

private:

  Ui::QCstmCT *ui;
  Mpx3GUI * _mpx3gui;

};

#endif // QCSTMCT_H
