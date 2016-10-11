#include "qcstmcorrectionsdialog.h"
#include "ui_qcstmcorrectionsdialog.h"
#include "qcstmBHWindow.h"
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

bool QCstmCorrectionsDialog::isSelectedBHCorr()
{
    return ui->bhcorrCheckbox->isChecked();
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


//!Load a BH correction
void QCstmCorrectionsDialog::on_bhcorrCheckbox_toggled(bool checked) {

    if (!checked){
        ui->obcorrCheckbox->setEnabled(1);
        ui->obcorrLineEdit->setEnabled(1);
        ui->bhcorrLineEdit->setEnabled(1);
        ui->bhcorrLineEdit->setText("");
    } else {
        ui->obcorrCheckbox->setEnabled(0);
        ui->obcorrCheckbox->setChecked(0);

        ui->obcorrLineEdit->setEnabled(0);
        ui->obcorrLineEdit->setText("");

        ui->bhcorrLineEdit->setEnabled(1);
    }


    // Deal with the separate BH window
    if ( !_bhwindow && checked && _mpx3gui->getDataset()->getLayer(0)!= nullptr) {
        _bhwindow = new QCstmBHWindow(this);
        _bhwindow->SetMpx3GUI( _mpx3gui );

        _bhwindow->show(); // nonModal
        _bhwindow->raise();
        _bhwindow->activateWindow();
    }

    if(_mpx3gui->getDataset()->getLayer(0) == nullptr && checked) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error");
        msgBox.setText("Please first load / take data.");
        msgBox.exec();
        ui->bhcorrCheckbox->setChecked(false);
    }

    if(_mpx3gui->getDataset()->getLayer(0)!= nullptr) {
        if ( ! checked ) {
            _bhwindow->close();
        } else {
            _bhwindow->show();
            _bhwindow->raise();
            _bhwindow->activateWindow();
        }
    }


}

void QCstmCorrectionsDialog::on_obcorrCheckbox_toggled(bool checked) {

    if(!checked) {
        ui->bhcorrCheckbox->setEnabled(1);
        ui->bhcorrLineEdit->setEnabled(1);
        ui->obcorrLineEdit->setEnabled(1);
        ui->obcorrLineEdit->setText("");

    } else {
        _mpx3gui->getDataset()->removeCorrection();

        ui->bhcorrCheckbox->setEnabled(0);
        ui->bhcorrCheckbox->setChecked(0);

        ui->bhcorrLineEdit->setEnabled(0);
        ui->bhcorrLineEdit->setText("");

        ui->obcorrLineEdit->setEnabled(1);


        QString filename = QFileDialog::getOpenFileName(this, tr("Read Data"), tr("."), tr("binary files (*.bin)"));
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

/**
 * On an existing image
 */
// DISABLED #BH_warning_dialog
// bool firstBHCorr = true;

void QCstmCorrectionsDialog::on_applyCorr_clicked() {

    //if ( ui->bhcorrLineEdit->text().isEmpty() && ui->obcorrLineEdit->text().isEmpty()) {
    //    return;
    //}

    if ( ! _vis->isTakingData() ) {

        /* DISABLED #BH_warning_dialog - implement this properly if the desire arises

        // Warn user if they want to apply another BH correction

        if (!firstBHCorr){
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, "Warning", "Do you really want to apply another beam hardening correction?",
                                          QMessageBox::Yes|QMessageBox::No);
            if (reply == QMessageBox::No) {
                return;
            }
        }
        */

        // This is done off data taking - WHAT DOES THIS EVEN MEAN
        //
        // Recover first the saved data to operate on the original
        _mpx3gui->rewindToOriginalDataset();

        // And apply corrections
        _mpx3gui->getDataset()->applyCorrections( this );

        // This applies the correction if necessary
        _vis->reload_all_layers();

        // DISABLED #BH_warning_dialog
        // firstBHCorr = false;
    }
}

void QCstmCorrectionsDialog::callBHCorrection(){
    emit applyBHCorrection();
}

void QCstmCorrectionsDialog::receiveFilename(QString filename){
    ui->bhcorrLineEdit->setText(filename);
}

void QCstmCorrectionsDialog::setChecked_BHCorrCheckbox(bool b)
{
    ui->bhcorrCheckbox->setChecked(b);
}
