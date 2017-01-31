#ifndef QCSTMBHWINDOW_H
#define QCSTMBHWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QMap>
#include <QString>
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
    void SetNLayersCurrentImage(int);

    //#44 Another corrections enhancement
    bool getFileSaved() const;
    void setFileSaved(bool value);
    //-----------------------------------

private:

    qcstmBHdialog * _bhdialog;

    void saveJSON(QString);
    bool fileSaved;

    //TO DO: Add descriptions...
    Ui::QCstmBHWindow *ui;
    QCstmCorrectionsDialog * _corr;
    Mpx3GUI * _mpx3gui;

    int selectedItemNo = 0;
    int emptyCorrectionCounter = 0;

    QMap<double,Dataset> correctionMap;
    QMap<double,QString> correctionPaths;
    QMap<double,QString> correctionMaterial;

    QVector<double> thicknessvctr;
    int _nLayersInCorrectionData;
    int _nLayersInCurrentImage;

    QString fileName;
    QString correctionPath;

    bool dataOpened;
    bool usePath = false;

    QCustomPlot* customPlot;
    tk::spline* m_spline = nullptr; //!< spline interpolation

    struct sortStruct {
        bool operator() (int a,int b) { return (a>b);}
    } cstmSortStruct;


signals:

    void takeData();
    void openData(bool);
    void openData2(bool, bool, QString);
    void loadData();
    void reload();
    void loadSignal();
    void applyCorrection();
    void sendFilename(QString filename);
    void sendChecked_BHCorrCheckbox(bool);

private slots:
    void on_addButton_clicked();
    void on_clearButton_clicked();
    void on_clearAllButton_clicked();
    void on_loadButton_clicked();
    void on_list_itemClicked(QListWidgetItem *item);
    void on_talkToForm(double thickness, QString material);
    //void on_startButton_clicked(); Disabled start button
    void on_open_data_failed();
    void on_list_doubleClicked(const QModelIndex &index);
    void on_applyBHCorrection();
    void on_okButton_clicked();
    void on_loadJsonButton_clicked();
    void on_saveJsonButton_clicked();
    void on_plot();

};

#endif // QCSTMBHWINDOW_H
