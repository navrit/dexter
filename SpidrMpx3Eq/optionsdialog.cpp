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
    //Saved in a QHash, which (in this case) saves int values stored by Qstring (in a random order).
    //This way more options can be added at any point, without worrying about numbering.

    //MTF
    _currentSettings["edge"] = ui->edgeRadioButton->isChecked();

    QString arg = ui->fitComboBox->currentText();
    _currentSettings["error"] = arg.contains("Error");

    _currentSettings["fitder"]  = ui->fitDerCheckBox->isChecked();
    _currentSettings["bindata"] = ui->binDataCheckBox->isChecked();

    _currentSettings["binsize"] = ui->binSizeSpinBox->value();

    //Check if the window width is not larger than the dataset.
    int maxlength = _datarange / _currentSettings.value("binsize");
    int windowW = ui->windowSpinBox->value();
    if( windowW > maxlength){
        windowW = maxlength;
        QMessageBox::warning ( this, tr("Warning"), tr( "The window width cannot be larger than the number of data points." ) );
        ui->windowSpinBox->setValue( windowW );
    }
    _currentSettings["windowW"] = windowW;

    //NPS
    _currentSettings["fullimage"]   = ui->fullImageRadioButton->isChecked();
    _currentSettings["selectedroi"] = ui->selectedRoIRadioButton->isChecked();
    _currentSettings["manualroi"]   = ui->manualRadioButton->isChecked();

    _currentSettings["roinumber"]   = ui->roiNumberSpinBox->value();
    _currentSettings["roixsize"]    = ui->roiXsizeSpinBox->value();
    _currentSettings["roiysize"]    = ui->roiYsizeSpinBox->value();
    _currentSettings["npixedge"]    = ui->edgeSpinBox->value();
    _currentSettings["nlines"]      = ui->nLinesSpinBox->value();

    _currentSettings["fitplane"]    = ui->fitPlaneCheckBox->isChecked();
    _currentSettings["zerofreq"]    = ui->zeroFreqCheckBox->isChecked();
    _currentSettings["showft"]      = ui->showFTcheckBox->isChecked();
    _currentSettings["normmaxnps"]  = ui->normMaxNpsCheckBox->isChecked();

    _currentSettings["pixelsize"]   = ui->pixelSizeLineEdit->text().toInt();
    for(int i = 0; i < ui->unitsNpsComboBox->count(); i++)
        _currentSettings[ ui->unitsNpsComboBox->itemText(i) ] = 0;
    _currentSettings[ ui->unitsNpsComboBox->currentText() ] = 1;
}

void optionsDialog::resetSettings()
{
    //MTF options
    ui->edgeRadioButton->setChecked(    _currentSettings.value("edge")      );
    ui->slitRadioButton->setChecked(    _currentSettings.value("slit")      );

    if(_currentSettings.value("error") )
         ui->fitComboBox->setCurrentIndex( 0 ); //Error function fitting is the first option.
    else ui->fitComboBox->setCurrentIndex( 1 ); //CHANGE when adding more fitting types.

    ui->fitDerCheckBox->setChecked(     _currentSettings.value("fitder")    );
    ui->binDataCheckBox->setChecked(    _currentSettings.value("bindata")   );

    ui->binSizeSpinBox->setValue(       _currentSettings.value("binsize")   );
    ui->windowSpinBox->setValue(        _currentSettings.value("windowW")   );

    //NPS options
    if( _currentSettings.value("fullimage") )
         ui->fullImageRadioButton->setChecked(true);
    else if( _currentSettings.value("selectedroi") )
         ui->selectedRoIRadioButton->setChecked(true);
    else ui->manualRadioButton->setChecked(true);

    ui->roiNumberSpinBox->setValue( _currentSettings.value("roinumber"));
    ui->roiXsizeSpinBox->setValue(  _currentSettings.value("roixsize") );
    ui->roiYsizeSpinBox->setValue(  _currentSettings.value("roiysize") );
    ui->edgeSpinBox->setValue(      _currentSettings.value("npixedge") );
    ui->nLinesSpinBox->setValue(    _currentSettings.value("nlines")   );

    ui->fitPlaneCheckBox->setChecked(   _currentSettings.value("fitplane") );
    ui->zeroFreqCheckBox->setChecked(   _currentSettings.value("zerofreq") );
    ui->showFTcheckBox->setChecked(     _currentSettings.value("showft") );
    ui->normMaxNpsCheckBox->setChecked( _currentSettings.value("normmaxnps") );

    ui->pixelSizeLineEdit->setText( QString("%1").arg(_currentSettings.value("pixelsize")) );

    if(_currentSettings.value("1/pix"))   ui->unitsNpsComboBox->setCurrentText("1/pix");
    if(_currentSettings.value("1/mm"))    ui->unitsNpsComboBox->setCurrentText("1/mm" );
    if(_currentSettings.value("lp/mm"))   ui->unitsNpsComboBox->setCurrentText("lp/mm");

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
        ui->windowSpinBox->setEnabled(false);
    }
    if(arg1.contains("Smoothing")){
//        _useErrorFunc = false;
        ui->windowLabel->setEnabled(true);
        ui->windowSpinBox->setEnabled(true);
    }
}

void optionsDialog::on_windowSpinBox_editingFinished()
{
    int width = ui->windowSpinBox->value();
    int binsize = ui->binSizeSpinBox->value();
    int maxlength = _datarange / binsize;
    if(maxlength % 2 ==0) maxlength--;

    if(width <= 2){
        width = 3;
                QMessageBox::warning ( this, tr("Warning"), tr( "The window width must be bigger than 2." ) );
        ui->windowSpinBox->setValue(width);
    }
    if(width % 2 == 0){
        width ++; //The window width must be an uneven number.
        QMessageBox::warning ( this, tr("Warning"), tr( "The window width must be an uneven number." ) );
        ui->windowSpinBox->setValue(width);
    }
    if(width > maxlength){
        width = maxlength;
        QMessageBox::warning ( this, tr("Warning"), tr( "The window width can not be larger than the number of data points." ) );
        ui->windowSpinBox->setValue(width);
    }
}

void optionsDialog::on_manualRadioButton_toggled(bool checked)
{
    ui->roiXsizeSpinBox->setEnabled(checked);
    ui->roiYsizeSpinBox->setEnabled(checked);
    ui->roiNumberSpinBox->setEnabled(checked);

    if(!checked){
//        ui->roiNumberSpinBox->setMaximum( 1 );
        ui->roiNumberSpinBox->setValue(   1 );
    }
}

void optionsDialog::on_roiXsizeSpinBox_editingFinished()
{
    int Nmax = _mpx3gui->getDataset()->calcMaxNroi( ui->roiXsizeSpinBox->value(), ui->roiYsizeSpinBox->value());
    ui->roiNumberSpinBox->setMaximum( Nmax );
    ui->roiNumberSpinBox->setValue( Nmax );
}

void optionsDialog::on_roiYsizeSpinBox_editingFinished()
{
    int x = ui->roiXsizeSpinBox->value();
    int y = ui->roiYsizeSpinBox->value();
    int Nmax = _mpx3gui->getDataset()->calcMaxNroi( ui->roiXsizeSpinBox->value(), ui->roiYsizeSpinBox->value() );
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
    ui->edgeSpinBox->setEnabled(checked);
}

void optionsDialog::on_zeroFreqCheckBox_toggled(bool checked)
{
    //Prevents the case that neither the zero axis as any off-axis lines are used. (No data)
    if(checked) ui->nLinesSpinBox->setMinimum(0);
    else ui->nLinesSpinBox->setMinimum(1);
}

void optionsDialog::on_binSizeSpinBox_editingFinished()
{
    //TODO: Check if the binsize is smaller than the dataset. No errors, but no bindata.. Warn user.
}
