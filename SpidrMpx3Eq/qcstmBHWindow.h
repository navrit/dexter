#ifndef QCSTMBHWINDOW_H
#define QCSTMBHWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QMap>
#include "mpx3gui.h"
#include "gradient.h"
#include "qcstmBHdialog.h"
#include "qcustomplot.h"


namespace Ui {
  class QCstmBHWindow;
}

class QCstmCorrectionsDialog;

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

  void openData(bool);

  void openData2(bool, bool, QString);

  void loadData(bool, QString);

  void reload();

  void loadSignal();

  void applyCorrection();

  void updateProgressBar(int value);

private slots:
  void on_addButton_clicked();

  void on_dataButton_clicked();

  void on_clearButton_clicked();

  void on_loadButton_clicked();

  void on_loadData(bool requestPath = false, QString path = "");

  void on_saveButton_clicked();

  void on_optionsButton_clicked();

  void on_list_itemClicked(QListWidgetItem *item);

  void on_talkToForm(double thickness, QString material);

  void on_startButton_clicked();

  void on_open_data_failed();

  void on_list_doubleClicked(const QModelIndex &index);

  void on_progressBar_valueChanged(int value);

  void on_applyBHCorrection();

  void on_okButton_clicked();

  void on_loadJsonButton_clicked();

  void on_saveJsonButton_clicked();

private:

  Ui::QCstmBHWindow *ui;
  QCstmCorrectionsDialog * _corr;
  Mpx3GUI * _mpx3gui; 
  int selectedItemNo = 0;
  int emptyCorrectionCounter = 0;
  QMap<double,Dataset> correctionMap;
  QVector<double> thicknessvctr;
  bool dataOpened;
  QCustomPlot* customPlot;
  tk::spline* m_spline = nullptr; //!< spline interpolation
  struct sortStruct {
    bool operator() (int a,int b) { return (a>b);}
  } cstmSortStruct;

};

#endif // QCSTMBHWINDOW_H
