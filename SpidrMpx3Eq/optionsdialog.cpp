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
    if(ui->edgeRadioButton->isChecked())
         _currentSettings["edge"] = 1;
    else _currentSettings["edge"] = 0;

    QString arg = ui->fitComboBox->currentText();
    if(arg.contains("Error"))
         _currentSettings["error"] = 1;
    else _currentSettings["error"] = 0;

    if(ui->fitDerCheckBox->isChecked())
         _currentSettings["fitder"] = 1;
    else _currentSettings["fitder"] = 0;

    if(ui->binDataCheckBox->isChecked())
         _currentSettings["bindata"] = 1;
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

    //NPS
    if(ui->fullImageRadioButton->isChecked())
         _currentSettings["fullimage"] = 1;
    else _currentSettings["fullimage"] = 0;
    if(ui->selectedRoIRadioButton->isChecked())
         _currentSettings["selectedroi"] = 1;
    else _currentSettings["selectedroi"] = 0;


    _currentSettings["roinumber"]   = ui->roiNumberSpinBox->value();
    _currentSettings["roixsize"]    = ui->roiXsizeLineEdit->text().toInt();
    _currentSettings["roiysize"]    = ui->roiYsizeLineEdit->text().toInt();
    _currentSettings["npixedge"]    = ui->edgeLineEdit->text().toInt();
    _currentSettings["nlines"] = ui->nLinesLineEdit->text().toInt();

    if(ui->fitPlaneCheckBox->isChecked())
         _currentSettings["fitplane"] = 1;
    else _currentSettings["fitplane"] = 0;

    if(ui->zeroFreqCheckBox->isChecked())
         _currentSettings["zerofreq"] = 1;
    else _currentSettings["zerofreq"] = 0;

    if(ui->showFTcheckBox->isChecked())
         _currentSettings["showft"] = 1;
    else _currentSettings["showft"] = 0;
}

void optionsDialog::resetSettings()
{
    //MTF options
    if(_currentSettings.value("edge") == 0)
         ui->edgeRadioButton->setChecked( false );
    else ui->edgeRadioButton->setChecked( true  );

    ui->fitComboBox->setCurrentIndex( _currentSettings.value("error") );

    if(_currentSettings.value("fitder") == 0)
         ui->fitDerCheckBox->setChecked( false );
    else ui->fitDerCheckBox->setChecked( true  );

    if(_currentSettings.value("bindata") == 0)
         ui->binDataCheckBox->setChecked( false );
    else ui->binDataCheckBox->setChecked( true  );

    ui->binSizeLineEdit->setText( QString("%1").arg( _currentSettings.value("binsize")) );
    ui->binSizeLineEdit->setText( QString("%1").arg( _currentSettings.value("windowW")) );

    //NPS options
    ui->roiNumberSpinBox->setValue( _currentSettings.value("roinumber") );
    ui->roiXsizeLineEdit->setText(  QString("%1").arg( _currentSettings.value("roixsize")) );
    ui->roiYsizeLineEdit->setText(  QString("%1").arg( _currentSettings.value("roiysize")) );
    ui->edgeLineEdit->setText(      QString("%1").arg( _currentSettings.value("npixedge")) );
    ui->nLinesLineEdit->setText(    QString("%1").arg( _currentSettings.value("nlines"))   );

    if(_currentSettings.value("fitplane") == 0)
         ui->fitPlaneCheckBox->setChecked( false );
    else ui->fitPlaneCheckBox->setChecked( true  );

    if(_currentSettings.value("zerofreq") == 0)
         ui->zeroFreqCheckBox->setChecked( false );
    else ui->zeroFreqCheckBox->setChecked( true  );

    if(_currentSettings.value("showft") == 0)
         ui->showFTcheckBox->setChecked( false );
    else ui->showFTcheckBox->setChecked( true  );


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

            QMessageBox::warning ( this, tr("No region selected"), tr( "Please select a region of interest." ) );
            _mpx3gui->GetUI()->stackedWidget->setCurrentIndex( __visualization_page_Id );
        }
    }
}
