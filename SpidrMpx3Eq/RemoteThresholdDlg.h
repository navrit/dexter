#ifndef REMOTETHRESHOLDDLG_H
#define REMOTETHRESHOLDDLG_H

#include <QDialog>
#include <QLabel>

namespace Ui {
class RemoteThresholdDlg;
}

class RemoteThresholdDlg : public QDialog
{
    Q_OBJECT

public:
    explicit RemoteThresholdDlg(QWidget *parent = 0);
    ~RemoteThresholdDlg();
    void setThresholdInfo(int chipId,int idx,int value);

private:
    Ui::RemoteThresholdDlg *ui;
    QLabel *_thresholdLabels[4][8];
    void _initializeThersholdLabels(void);
};

#endif // REMOTETHRESHOLDDLG_H
