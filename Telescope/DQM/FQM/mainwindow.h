#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qcustomplot.h"
#include <QTimer>
#include <QDateTime>
#include <vector>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    int m_extension;
    explicit MainWindow(int);
    ~MainWindow();
    int m_sampleNum;
    double m_timeWidth;
    double m_samplePeriod;
    QCustomPlot * _Qg;
    int m_nPlanes;
    int m_runNumber;
    std::vector <std::vector< std::string > > m_fileNames;

private:
    Ui::MainWindow *ui;
    double getFileSize(int);

private slots:
    void timer_update();
    void on_b_reScan_clicked();
    void mousePress(QMouseEvent*);
    void mouseWheel();

};

#endif // MAINWINDOW_H
