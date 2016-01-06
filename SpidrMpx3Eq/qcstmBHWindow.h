#ifndef QCSTMBHWINDOW_H
#define QCSTMBHWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QMap>
#include "mpx3gui.h"
#include "gradient.h"
#include "qcstmBHdialog.h"


namespace Ui {
  class QCstmBHWindow;
}

class QCstmBHWindow : public QDialog
{
  Q_OBJECT

public:

  explicit QCstmBHWindow(QWidget *parent = 0);
  ~QCstmBHWindow();
  Ui::QCstmBHWindow * GetUI(){ return ui; }

  void SetMpx3GUI(Mpx3GUI *p);

private:

  qcstmBHdialog * _bhdialog;

signals:

  void takeData();

  void openData();

  void reload();

private slots:
  void on_addButton_clicked();

  void on_dataButton_clicked();

  void on_clearButton_clicked();

  void on_loadButton_clicked();

  void on_saveButton_clicked();

  void on_optionsButton_clicked();

  void on_list_itemClicked(QListWidgetItem *item);

  void on_talkToForm(double thickness);

  void on_startButton_clicked();

  void on_open_data_failed();

private:

  Ui::QCstmBHWindow *ui;
  Mpx3GUI * _mpx3gui; 
  int selectedItemNo;
  QMap<double,Dataset> correctionMap;
  vector<double> thicknessvctr;
  int emptyCorrectionCounter = 0;
  bool dataOpened;


};

#endif // QCSTMBHWINDOW_H
