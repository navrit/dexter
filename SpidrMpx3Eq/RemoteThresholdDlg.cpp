#include "RemoteThresholdDlg.h"
#include "ui_RemoteThresholdDlg.h"

RemoteThresholdDlg::RemoteThresholdDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RemoteThresholdDlg)
{
    ui->setupUi(this);
    _initializeThersholdLabels();
}

RemoteThresholdDlg::~RemoteThresholdDlg()
{
    delete ui;
}

void RemoteThresholdDlg::setThresholdInfo(int chipId, int idx, int value)
{
    if(chipId >= 0 && chipId < 4 && idx >= 0 && idx < 8){
        _thresholdLabels[chipId][idx]->setText(QString::number(value));
    }
}

void RemoteThresholdDlg::_initializeThersholdLabels()
{
    _thresholdLabels[0][0] = ui->ch0th0Label;
    _thresholdLabels[0][1] = ui->ch0th1Label;
    _thresholdLabels[0][2] = ui->ch0th2Label;
    _thresholdLabels[0][3] = ui->ch0th3Label;
    _thresholdLabels[0][4] = ui->ch0th4Label;
    _thresholdLabels[0][5] = ui->ch0th5Label;
    _thresholdLabels[0][6] = ui->ch0th6Label;
    _thresholdLabels[0][7] = ui->ch0th7Label;
    _thresholdLabels[1][0] = ui->ch1th0Label;
    _thresholdLabels[1][1] = ui->ch1th1Label;
    _thresholdLabels[1][2] = ui->ch1th2Label;
    _thresholdLabels[1][3] = ui->ch1th3Label;
    _thresholdLabels[1][4] = ui->ch1th4Label;
    _thresholdLabels[1][5] = ui->ch1th5Label;
    _thresholdLabels[1][6] = ui->ch1th6Label;
    _thresholdLabels[1][7] = ui->ch1th7Label;
    _thresholdLabels[2][0] = ui->ch2th0Label;
    _thresholdLabels[2][1] = ui->ch2th1Label;
    _thresholdLabels[2][2] = ui->ch2th2Label;
    _thresholdLabels[2][3] = ui->ch2th3Label;
    _thresholdLabels[2][4] = ui->ch2th4Label;
    _thresholdLabels[2][5] = ui->ch2th5Label;
    _thresholdLabels[2][6] = ui->ch2th6Label;
    _thresholdLabels[2][7] = ui->ch2th7Label;
    _thresholdLabels[3][0] = ui->ch3th0Label;
    _thresholdLabels[3][1] = ui->ch3th1Label;
    _thresholdLabels[3][2] = ui->ch3th2Label;
    _thresholdLabels[3][3] = ui->ch3th3Label;
    _thresholdLabels[3][4] = ui->ch3th4Label;
    _thresholdLabels[3][5] = ui->ch3th5Label;
    _thresholdLabels[3][6] = ui->ch3th6Label;
    _thresholdLabels[3][7] = ui->ch3th7Label;
}
