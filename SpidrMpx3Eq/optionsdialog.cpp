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
    //Saved in a QHash, which (in this case) saves int values stored by Qstring.
    //This way more options can be added at any point, without worrying about numbering

    //MTF
    _currentSettings["edge"] = ui->edgeRadioButton->isChecked();

    QString arg = ui->fitComboBox->currentText();
    _currentSettings["error"] = arg.contains("Error");

    _currentSettings["fitder"]  = ui->fitDerCheckBox->isChecked();
    _currentSettings["bindata"] = ui->binDataCheckBox->isChecked();

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

    //NPS
    _currentSettings["fullimage"]   = ui->fullImageRadioButton->isChecked();
    _currentSettings["selectedroi"] = ui->selectedRoIRadioButton->isChecked();
    _currentSettings["manualroi"]   = ui->manualRadioButton->isChecked();

    _currentSettings["roinumber"]   = ui->roiNumberSpinBox->value();
    _currentSettings["roixsize"]    = ui->roiXsizeLineEdit->text().toInt();
    _currentSettings["roiysize"]    = ui->roiYsizeLineEdit->text().toInt();
    _currentSettings["npixedge"]    = ui->edgeLineEdit->text().toInt();
    _currentSettings["nlines"]      = ui->nLinesLineEdit->text().toInt();

    _currentSettings["fitplane"]    = ui->fitPlaneCheckBox->isChecked();
    _currentSettings["zerofreq"]    = ui->zeroFreqCheckBox->isChecked();
    _currentSettings["showft"]      = ui->showFTcheckBox->isChecked();
    _currentSettings["normmaxnps"]  = ui->normMaxNpsCheckBox->isChecked();
}

void optionsDialog::resetSettings()
{
    //MTF options
    ui->edgeRadioButton->setChecked(    _currentSettings.value("edge")  );
    ui->fitComboBox->setCurrentIndex(   _currentSettings.value("error") );
    ui->fitDerCheckBox->setChecked(     _currentSettings.value("fitder") );
    ui->binDataCheckBox->setChecked(    _currentSettings.value("bindata") );

    ui->binSizeLineEdit->setText( QString("%1").arg( _currentSettings.value("binsize")) );
    ui->binSizeLineEdit->setText( QString("%1").arg( _currentSettings.value("windowW")) );

    //NPS options
    if( _currentSettings.value("fullimage") )
         ui->fullImageRadioButton->setChecked(true);
    else if( _currentSettings.value("selectedroi") )
         ui->selectedRoIRadioButton->setChecked(true);
    else ui->manualRadioButton->setChecked(true);

    ui->roiNumberSpinBox->setValue( _currentSettings.value("roinumber") );
    ui->roiXsizeLineEdit->setText(  QString("%1").arg( _currentSettings.value("roixsize")) );
    ui->roiYsizeLineEdit->setText(  QString("%1").arg( _currentSettings.value("roiysize")) );
    ui->edgeLineEdit->setText(      QString("%1").arg( _currentSettings.value("npixedge")) );
    ui->nLinesLineEdit->setText(    QString("%1").arg( _currentSettings.value("nlines"))   );

    ui->fitPlaneCheckBox->setChecked(   _currentSettings.value("fitplane") );
    ui->zeroFreqCheckBox->setChecked(   _currentSettings.value("zerofreq") );
    ui->showFTcheckBox->setChecked(     _currentSettings.value("showft") );
    ui->normMaxNpsCheckBox->setChecked( _currentSettings.value("normmaxnps") );
}

void optionsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::ButtonRole role = ui->buttonBox->buttonRole(button);
    if(role == QDialogButtonBox::ApplyRole){
        //Apply changes
        setCurrentSettings();
        emit apply_options(_currentSettings);
        //leave window open.
    }

    if(role == QDialogButtonBox::AcceptRole){
        //Apply changes
        setCurrentSettings();
        emit apply_options(_currentSettings);

        //close window
        emit close_optionsDialog();
    }

    if(role == QDialogButtonBox::RejectRole){
        //Set back any changes made to how they were when opening the dialog.
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

void optionsDialog::on_manualRadioButton_toggled(bool checked)
{
    ui->roiXsizeLineEdit->setEnabled(checked);
    ui->roiYsizeLineEdit->setEnabled(checked);

    if(!checked){
        ui->roiNumberSpinBox->setMaximum( 1 );
        ui->roiNumberSpinBox->setValue(   1 );
    }
}

void optionsDialog::on_roiXsizeLineEdit_editingFinished()
{
//    int x = ui->roiXsizeLineEdit->text().toInt(); //debugging
//    int y = ui->roiYsizeLineEdit->text().toInt();
    int Nmax = _mpx3gui->getDataset()->calcMaxNroi( ui->roiXsizeLineEdit->text().toInt(), ui->roiYsizeLineEdit->text().toInt() );
    ui->roiNumberSpinBox->setMaximum( Nmax );
    ui->roiNumberSpinBox->setValue( Nmax );
}

void optionsDialog::on_roiYsizeLineEdit_editingFinished()
{
    int Nmax = _mpx3gui->getDataset()->calcMaxNroi( ui->roiXsizeLineEdit->text().toInt(), ui->roiYsizeLineEdit->text().toInt() );
    ui->roiNumberSpinBox->setMaximum( Nmax );
    ui->roiNumberSpinBox->setValue( Nmax );
}

void optionsDialog::on_selectedRoIRadioButton_toggled(bool checked)
{
    if(checked){
        //Check if a region of interest is selected.
        if(!_mpx3gui->GetUI()->dqeTab->isValidRegionSelected()){
            ui->selectedRoIRadioButton->setChecked( false );
            ui->fullImageRadioButton->setChecked(   true  );

            QMessageBox::warning ( this, tr("No valid region selected"), tr( "Please select a region of interest that lies within the image." ) );
            _mpx3gui->GetUI()->stackedWidget->setCurrentIndex( __visualization_page_Id );
        }
    }
}
