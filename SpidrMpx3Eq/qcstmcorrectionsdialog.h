#ifndef QCSTMCORRECTIONSDIALOG_H
#define QCSTMCORRECTIONSDIALOG_H

#include <QDialog>
#include "mpx3gui.h"

namespace Ui {
class QCstmCorrectionsDialog;
}

class QCstmGLVisualization;

class QCstmCorrectionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QCstmCorrectionsDialog(QWidget *parent = 0);
    ~QCstmCorrectionsDialog();
    void SetMpx3GUI(Mpx3GUI *p);

//    bool isCorrectionsActive();
    bool isSelectedOBCorr();
    bool isSelectedDeadPixelsInter();
    bool isSelectedHighPixelsInter();

//    void setCorrectionsActive(bool s){ _correctionsActive = s; } //Not used!
    double getNoisyPixelMeanMultiplier();

public slots:
    void receiveFilename(QString filename);

private slots:
    void on_obcorrCheckbox_toggled(bool checked);
    //!Apply corrections manually
    void on_applyCorr_clicked();

private:
    Ui::QCstmCorrectionsDialog *ui;
    QCstmGLVisualization * _vis;
    Mpx3GUI * _mpx3gui;
//    bool _correctionsActive = false;

};

#endif // QCSTMCORRECTIONSDIALOG_H
