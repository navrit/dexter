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

bool QCstmCorrectionsDialog::isCorrectionsActive() {
     return _correctionsActive;
}

bool QCstmCorrectionsDialog::isSelectedOBCorr() {
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

bool QCstmCorrectionsDialog::isSelectedBHCorr()
{
    return ui->bhcorrCheckbox->isChecked();
}

double QCstmCorrectionsDialog::getNoisyPixelMeanMultiplier()
{
    return ui->noisyPixelMeanMultiplier->value();
}

//!Load a BH correction
void QCstmCorrectionsDialog::on_bhcorrCheckbox_toggled(bool checked) {

    // Deal with the separate BH window
    if ( !_bhwindow && checked && _mpx3gui->getDataset()->getLayer(0)!= nullptr) {
        _bhwindow = new QCstmBHWindow(this);
        _bhwindow->SetMpx3GUI( _mpx3gui );

        _bhwindow->show(); // nonModal
        _bhwindow->raise();
        _bhwindow->activateWindow();
    }

    if(_mpx3gui->getDataset()->getLayer(0)== nullptr && checked)
    {
        QMessageBox msgBox;
        msgBox.setText("Please first load / take data.");
        msgBox.exec();
        ui->bhcorrCheckbox->setChecked(false);
    }

if(_mpx3gui->getDataset()->getLayer(0)!= nullptr)
    {
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
        _mpx3gui->getDataset()->removeCorrection();
        ui->obcorrLineEdit->setText("");
    } else {
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
void QCstmCorrectionsDialog::on_applyCorr_clicked() {
    if ( ! _vis->isTakingData() ) {

        // This is done off data taking
        // Recover first the saved data to operate on the original
        _mpx3gui->rewindToOriginalDataset();

        // And apply corrections
        _mpx3gui->getDataset()->applyCorrections( this );

        // This applies the correction if necessary
        _vis->reload_all_layers();

    }
}

void QCstmCorrectionsDialog::callBHCorrection(){
    emit applyBHCorrection();
}

void QCstmCorrectionsDialog::receiveFilename(QString filename){
    ui->bhcorrLineEdit->setText(filename);
}
