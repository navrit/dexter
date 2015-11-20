#ifndef QCSTMBHWINDOW_H
#define QCSTMBHWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include "mpx3gui.h"
#include "gradient.h"
#include "qstmBHdialog.h"

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

  qstmBHdialog * _bhdialog;

private slots:
  void on_addButton_clicked();

  void on_dataButton_clicked();

  void on_clearButton_clicked();

  void on_loadButton_clicked();

  void on_saveButton_clicked();

  void on_optionsButton_clicked();

  void on_list_itemClicked(QListWidgetItem *item);

private:

  Ui::QCstmBHWindow *ui;
  Mpx3GUI * _mpx3gui;

};

#endif // QCSTMBHWINDOW_H
