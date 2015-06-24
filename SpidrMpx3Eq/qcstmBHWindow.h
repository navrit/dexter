#ifndef QCSTMBHWINDOW_H
#define QCSTMBHWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include "mpx3gui.h"
#include "gradient.h"

namespace Ui {
  class QCstmBHWindow;
}

class QCstmBHWindow : public QMainWindow
{
  Q_OBJECT

public:

  explicit QCstmBHWindow(QWidget *parent = 0);
  ~QCstmBHWindow();
  Ui::QCstmBHWindow * GetUI(){ return ui; };

  void SetMpx3GUI(Mpx3GUI *p);

private:

  Ui::QCstmBHWindow *ui;
  Mpx3GUI * _mpx3gui;

};

#endif // QCSTMBHWINDOW_H
