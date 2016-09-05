#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "mpx3gui.h"
#include "ui_mpx3gui.h"
#include "qcstmdqe.h"
#include <QDialogButtonBox>

optionsDialog::optionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::optionsDialog)
{
    ui->setupUi(this);

    ui->buttonBox->button(QDialogButtonBox::Apply)->setDefault(true);
}

void optionsDialog::SetMpx3GUI(Mpx3GUI * p )
{
    _mpx3gui = p;

    connect( this, SIGNAL(close_optionsDialog()), _mpx3gui->GetUI()->dqeTab, SLOT(on_close_optionsDialog()) );
    connect( this, SIGNAL(apply_options(QHash<QString, int>)), _mpx3gui->GetUI()->dqeTab, SLOT(on_apply_options(QHash<QString, int>)) );
}

optionsDialog::~optionsDialog()
{
    delete ui;
}

void optionsDialog::setCurrentSettings()
{
    //Saved in a QHash, which saves int values stored per String.
    //This way more options can be stored, without worrying about numbering

    _currentSettings["roiNumber"] = ui->roiNumberLineEdit->text().toInt();
    _currentSettings["roiSize"] = ui->roiSizeLineEdit->text().toInt();
    _currentSettings["edge"] = ui->edgeLineEdit->text().toInt();

    if(ui->edgeRadioButton->isChecked()) _currentSettings["edge"] = 1;
        else _currentSettings["edge"] = 0;

    QString arg = ui->fitComboBox->currentText();
    if(arg.contains("Error")) _currentSettings["error"] = 1 ; //_useErrorFunc = true;
        else _currentSettings["error"] = 0;

    if(ui->fitDerCheckBox->isChecked()) _currentSettings["fitder"] = 1;
        else _currentSettings["fitder"] = 0;

    if(ui->binDataCheckBox->isChecked()) _currentSettings["bindata"] = 1;
        else _currentSettings["bindata"] = 0;

    _currentSettings["binsize"] = ui->binSizeLineEdit->text().toDouble();

    //Check if the window width is not larger than the dataset.
    int maxlength = _datarange / _currentSettings.value("binsize");
    int windowW = ui->windowLineEdit->text().toInt();
    if( windowW > maxlength){
        windowW = maxlength;
        QMessageBox::warning ( this, tr("Warning"), tr( "The window width cannot be larger than the number of data points." ) );
        ui->windowLineEdit->setText( QString("%1").arg(windowW) );
    }
    _currentSettings["windowW"] = windowW;
}

void optionsDialog::resetSettings()
{
    ui->roiNumberLineEdit->setText( QString("%1").arg(_currentSettings.value("roiNumber")) );
    ui->roiSizeLineEdit->setText( QString("%1").arg(_currentSettings.value("roiSize")) );
    ui->edgeLineEdit->setText( QString("%1").arg(_currentSettings.value("edge")) );

    if(_currentSettings.value("edge") == 0) ui->edgeRadioButton->setChecked(false);
        else ui->edgeRadioButton->setChecked(true);
    ui->fitComboBox->setCurrentIndex( _currentSettings.value("error") );
    if(_currentSettings.value("fitder") == 0) ui->fitDerCheckBox->setChecked(false);
        else ui->fitDerCheckBox->setChecked(false);
    if(_currentSettings.value("bindata") == 0) ui->binDataCheckBox->setChecked(false);
        else ui->binDataCheckBox->setChecked(true);

    ui->binSizeLineEdit->setText( QString("%1").arg(_currentSettings.value("binsize")) );

    ui->binSizeLineEdit->setText( QString("%1").arg(_currentSettings.value("windowW")) );
}

void optionsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::ButtonRole role = ui->buttonBox->buttonRole(button);
    if(role == QDialogButtonBox::ApplyRole){
        //Apply changes, leave window open.
        setCurrentSettings();
        emit apply_options(_currentSettings);
    }

    if(role == QDialogButtonBox::AcceptRole){
        //Apply changes and talk back to dqe (TODO)
        setCurrentSettings();

        emit apply_options(_currentSettings);

        //close window
        emit close_optionsDialog();
    }

    if(role == QDialogButtonBox::RejectRole){
        //Set back any changes made to how they were when opening the dialog.
        //Using _currentSettings
        resetSettings();

        //close window
        emit close_optionsDialog();
    }

}


void optionsDialog::on_fitComboBox_currentIndexChanged(const QString &arg1)
{
    if(arg1.contains("Error")){
//        _useErrorFunc = true;
        ui->windowLabel->setEnabled(false);
        ui->windowLineEdit->setEnabled(false);
    }
    if(arg1.contains("Smoothing")){
//        _useErrorFunc = false;
        ui->windowLabel->setEnabled(true);
        ui->windowLineEdit->setEnabled(true);
    }
}

void optionsDialog::on_windowLineEdit_editingFinished()
{
    int width = ui->windowLineEdit->text().toInt();
    int binsize = ui->binSizeLineEdit->text().toInt();
    int maxlength = _datarange / binsize;
    if(maxlength % 2 ==0) maxlength--;

    if(width <= 2){
        width = 3;
                QMessageBox::warning ( this, tr("Warning"), tr( "The window width must be bigger than 2." ) );
        ui->windowLineEdit->setText(QString("%1").arg(width));
    }
    if(width % 2 == 0){
        width ++; //The window width must be an uneven number.
        QMessageBox::warning ( this, tr("Warning"), tr( "The window width must be an uneven number." ) );
        ui->windowLineEdit->setText(QString("%1").arg(width));
    }
    if(width > maxlength){
        width = maxlength;
        QMessageBox::warning ( this, tr("Warning"), tr( "The window width can not be larger than the number of data points." ) );
        ui->windowLineEdit->setText(QString("%1").arg(width));
    }
}
