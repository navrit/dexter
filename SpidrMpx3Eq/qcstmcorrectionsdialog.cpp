#include "qcstmcorrectionsdialog.h"
#include "ui_qcstmcorrectionsdialog.h"
#include "qcstmglvisualization.h"

QCstmCorrectionsDialog::QCstmCorrectionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QCstmCorrectionsDialog){

    ui->setupUi(this);
    _vis = dynamic_cast<QCstmGLVisualization*>(parent);

    this->setWindowTitle( tr("Corrections") );
}

QCstmCorrectionsDialog::~QCstmCorrectionsDialog()
{
    delete ui;
}

void QCstmCorrectionsDialog::SetMpx3GUI(Mpx3GUI * p){
    _mpx3gui = p;
}

//bool QCstmCorrectionsDialog::isCorrectionsActive() {
//     return _correctionsActive;
//}

bool QCstmCorrectionsDialog::isSelectedOBCorr()
{
    return ui->obcorrCheckbox->isChecked();
}

bool QCstmCorrectionsDialog::isSelectedDeadPixelsInter()
{
    return ui->deadpixelsinterpolationCheckbox->isChecked();
}

bool QCstmCorrectionsDialog::isSelectedHighPixelsInter()
{
    return ui->highinterpolationCheckbox->isChecked();
}

double QCstmCorrectionsDialog::getNoisyPixelMeanMultiplier()
{
    return ui->noisyPixelMeanMultiplier->value();
}

void QCstmCorrectionsDialog::on_obcorrCheckbox_toggled(bool checked) {

    if ( ! checked ) {
        ui->obcorrLineEdit->setEnabled(1);
        ui->obcorrLineEdit->setText("");

    } else {
        _mpx3gui->getDataset()->removeCorrection();


        ui->obcorrLineEdit->setEnabled(1);


        QString filename = QFileDialog::getOpenFileName(this, tr("Read Data"), tr("."), tr("binary files (*.bin)"));
        if (  filename.isNull() ) { // the user clicked cancel

            ui->obcorrCheckbox->setChecked( false );

        } else {
            QFile saveFile(filename);

            if ( !saveFile.open(QIODevice::ReadOnly) ) {
                string messg = "Couldn't open: ";
                messg += filename.toStdString();
                messg += "\nNo output written!";
                QMessageBox::warning ( this, tr("Error opening data"), tr( messg.c_str() ) );
                return;
            }

            _mpx3gui->getDataset()->loadCorrection(saveFile.readAll());

            ui->obcorrLineEdit->setText(filename);
        }
    }

}

/**
 * On an existing image
 */

void QCstmCorrectionsDialog::on_applyCorr_clicked() {

    if ( ! _vis->isTakingData() ) {

        // This is done off data taking - WHAT DOES THIS EVEN MEAN
        //
        // Recover first the saved data to operate on the original
        _mpx3gui->rewindToOriginalDataset();

        // And apply corrections
        _mpx3gui->getDataset()->applyCorrections( this );

        // This applies the correction if necessary
        _vis->reload_all_layers();

    }
}

void QCstmCorrectionsDialog::receiveFilename(QString filename){
}
