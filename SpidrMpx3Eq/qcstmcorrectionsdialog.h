#ifndef QCSTMCORRECTIONSDIALOG_H
#define QCSTMCORRECTIONSDIALOG_H

#include <QDialog>
#include "mpx3gui.h"

namespace Ui {
    class QCstmCorrectionsDialog;
}

class QCstmBHWindow;
class QCstmGLVisualization;

class QCstmCorrectionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QCstmCorrectionsDialog(QWidget *parent = nullptr);
    ~QCstmCorrectionsDialog();
    void SetMpx3GUI(Mpx3GUI *p);

    bool isSelectedOBCorr();
    bool isSelectedDeadPixelsInter();
    bool isSelectedHighPixelsInter();
    double getNoisyPixelMeanMultiplier();

private slots:
    void on_obcorrCheckbox_toggled(bool checked);

private:
    Ui::QCstmCorrectionsDialog *ui = nullptr;
    QCstmGLVisualization *_vis = nullptr;
    Mpx3GUI *_mpx3gui = nullptr;

signals:
    void applyBHCorrection();
};

#endif // QCSTMCORRECTIONSDIALOG_H
