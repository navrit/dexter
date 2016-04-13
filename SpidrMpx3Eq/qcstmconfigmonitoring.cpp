#include "qcstmconfigmonitoring.h"
#include "ui_qcstmconfigmonitoring.h"
#include "mpx3config.h"

#include "SpidrController.h"
#include "StepperMotorController.h"
#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"

#include "qtableview.h"
#include "qstandarditemmodel.h"


QCstmConfigMonitoring::QCstmConfigMonitoring(QWidget *parent) :
    QWidget(parent),

    ui(new Ui::QCstmConfigMonitoring) {
    ui->setupUi(this);

    _timerId = -1;
    ui->samplingSpinner->setValue( 1.0 );

    ui->gainModeCombobox->addItem("Super High Gain Mode");
    ui->gainModeCombobox->addItem("Low Gain Mode");
    ui->gainModeCombobox->addItem("High Gain Mode");
    ui->gainModeCombobox->addItem("Super Low Gain Mode");

    ui->polarityComboBox->addItem("Positive");
    ui->polarityComboBox->addItem("Negative");

    ui->operationModeComboBox->addItem("Sequential R/W");
    ui->operationModeComboBox->addItem("Continuous R/W");

    ui->motorDial->setNotchesVisible(true);

    _stepperThread = nullptr;
    _stepper = nullptr;

    // Set inactive whatever is needed to be inactive
    activeInGUI();

    QStandardItemModel * tvmodel = new QStandardItemModel(3,2, this); // 3 Rows and 2 Columns
    tvmodel->setHorizontalHeaderItem( 0, new QStandardItem(QString("pos")) );
    tvmodel->setHorizontalHeaderItem( 1, new QStandardItem(QString("angle")) );

    ui->stepperCalibrationTableView->setModel( tvmodel );

    ui->stepperMotorCheckBox->setToolTip( "enable/disable stepper motor control" );

    QFont font1("Courier New");
    ui->omrDisplayLabel->setFont( font1 );
    ui->omrDisplayLabel->setTextFormat( Qt::RichText );

    ///////////////////////////////////////////////
    // Camera
    _cameraOn = false;
    _camera = 0x0;
    _imageCapture = 0x0;
    _viewfinder = 0x0;
    _cameraId = -1;

}

QCstmConfigMonitoring::~QCstmConfigMonitoring()
{
    if( _stepperThread ) delete _stepperThread;
    if( _imageCapture ) delete _imageCapture;
    if( _viewfinder ) delete _viewfinder;
    if( _camera ) delete _camera;

    delete ui;
}


void QCstmConfigMonitoring::on_tempReadingActivateCheckBox_toggled(bool checked) {

    if ( checked ) {
        // Get a quick read first to let the user see immediately and start the timer.
        // The timer will keep refreshing periodically.
        readTemp();
        _timerId = this->startTimer( ui->samplingSpinner->value()*1000 ); // value comes in seconds from GUI.  Convert to ms.
    } else {
        this->killTimer( _timerId );
        _timerId = -1;
        // clean the temp display
        ui->remoteTempMeasLabel->setText("");
        ui->localTempMeasLabel->setText("");
    }

}

#define __nwords_OMR 6
#define __nbits_OMR 48 // 6 words of 8 bits

void QCstmConfigMonitoring::on_readOMRPushButton_clicked() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    int  dev_nr = 2;
    unsigned char omr[6];
    spidrcontrol->getOmr( dev_nr, omr );

    // <font color=\"blue\">Hello</font> <font color=\"red\">World</font><font color=\"green">!</font>

    QString toDisplay;
    QString formatNoSpace_gui;
    QString formatWithSpace_gui;
    QString formatNoSpace_console;
    QString formatWithSpace_console;

    unsigned char endMask = 0x100; // 1 00000000 (ninth bit)
    char bits16Save[__nbits_OMR];
    int lateRunningBitCntr = 0;

    cout << "[OMR ]" << endl;
    int bitCntr = 0;
    for ( int i = 0 ; i < __nwords_OMR ; i++ ) { // this comes packed in 6 bytes
        // extract each bit
        unsigned char mask = 0x1;
        formatNoSpace_gui   = "B%00d&nbsp;";
        formatWithSpace_gui = "B%02d&nbsp;";
        formatNoSpace_console   = "B%00d ";
        formatWithSpace_console = "B%02d ";

        for( ; mask != endMask ; ) {

            // Pick the bit ON or OFF
            char bitC;
            if( (omr[i] & mask) != 0 ) {
                bitC='1';
            } else {
                bitC='0';
            }

            // Save the bit selected in this array
            bits16Save[bitCntr] = bitC;

            QString oneBitS;
            if ( bitCntr < 10 ) {
                toDisplay += oneBitS.sprintf(formatWithSpace_gui.toStdString().c_str(), bitCntr); // GUI
                printf(formatWithSpace_console.toStdString().c_str(), bitCntr, bitC); // console
            } else {
                toDisplay += oneBitS.sprintf(formatNoSpace_gui.toStdString().c_str(), bitCntr); // GUI
                printf(formatNoSpace_console.toStdString().c_str(), bitCntr, bitC); // console
            }

            mask = mask << 1; // shift a bit to the left
            bitCntr++;
        }

        if ( (bitCntr%16)==0 && bitCntr!=0  ) {

            // Printing bits now
            formatNoSpace_gui   = "&nbsp;%c&nbsp;&nbsp;";
            formatWithSpace_gui = "&nbsp;%c&nbsp;&nbsp;";
            formatNoSpace_console   = " %c  ";
            formatWithSpace_console = " %c  ";

            // A carriage return
            toDisplay += "<br />";  // GUI
            cout << endl;  // Console
            for( ; lateRunningBitCntr < bitCntr ; ) {
                // Draw the last 16 bits now
                QString oneBitS;
                // if the bit is on pain it in red
                if ( bits16Save[lateRunningBitCntr] == '1' ) {
                    toDisplay += "<font color=\"red\">";
                } else {
                    toDisplay += "<font color=\"black\">";
                }
                if ( bitCntr < 10 ) {
                    toDisplay += oneBitS.sprintf(formatWithSpace_gui.toStdString().c_str(), bits16Save[lateRunningBitCntr]); // GUI
                    printf(formatWithSpace_console.toStdString().c_str(), bits16Save[lateRunningBitCntr]); // console
                } else {
                    toDisplay += oneBitS.sprintf(formatNoSpace_gui.toStdString().c_str(), bits16Save[lateRunningBitCntr]); // GUI
                    printf(formatNoSpace_console.toStdString().c_str(), bits16Save[lateRunningBitCntr]); // console
                }
                toDisplay += "</font>";
                lateRunningBitCntr++;
            }
            // A carriage return
            toDisplay += "<br />";  // GUI
            cout << endl;  // Console
        }

    }

    ui->omrDisplayLabel->setText( toDisplay );

}

void QCstmConfigMonitoring::activeInGUI(){

    // stepper part
    if ( _stepper ) {
        if ( ! _stepper->isStepperReady() ) {
            deactivateItemsGUI();
        } else {
            activateItemsGUI();
        }
    } else {
        deactivateItemsGUI();
    }

}

void QCstmConfigMonitoring::activateItemsGUI(){
    ui->motorDial->setEnabled( true );
    ui->motorGoToTargetButton->setEnabled( true );
    ui->motorResetButton->setEnabled( true );
    ui->motorTestButton->setEnabled( true );
    ui->stepperCalibrationTableView->setEnabled( true );
    ui->stepperUseCalibCheckBox->setEnabled( true );
    ui->accelerationSpinBox->setEnabled( true );
    ui->speedSpinBox->setEnabled( true );
    ui->targetPosSpinBox->setEnabled( true );
    ui->currentISpinBox->setEnabled( true );
    ui->motorIdSpinBox->setEnabled( true );
    ui->stepperSetZeroPushButton->setEnabled( true );

}

void QCstmConfigMonitoring::deactivateItemsGUI(){
    ui->motorDial->setDisabled( true );
    ui->motorGoToTargetButton->setDisabled( true );
    ui->motorResetButton->setDisabled( true );
    ui->motorTestButton->setDisabled( true );
    ui->stepperCalibrationTableView->setDisabled( true );
    ui->stepperUseCalibCheckBox->setDisabled( true );
    ui->accelerationSpinBox->setDisabled( true );
    ui->speedSpinBox->setDisabled( true );
    ui->targetPosSpinBox->setDisabled( true );
    ui->currentISpinBox->setDisabled( true );
    ui->motorIdSpinBox->setDisabled( true );
    ui->stepperSetZeroPushButton->setDisabled( true );
}


void QCstmConfigMonitoring::SetMpx3GUI(Mpx3GUI *p) {

    _mpx3gui = p;
    Mpx3Config *config = _mpx3gui->getConfig();

    connect(ui->ColourModeCheckBox, SIGNAL(clicked(bool)), config, SLOT(setColourMode(bool)));
    connect(config, SIGNAL(colourModeChanged(bool)), ui->ColourModeCheckBox, SLOT(setChecked(bool)));

    connect(ui->readBothCountersCheckBox, SIGNAL(clicked(bool)), config, SLOT(setReadBothCounters(bool)));
    connect(config, SIGNAL(readBothCountersChanged(bool)), ui->readBothCountersCheckBox, SLOT(setChecked(bool)));

    connect(ui->csmSpmSpinner, SIGNAL(valueChanged(int)), config, SLOT(setCsmSpm(int)));
    connect(config, SIGNAL(csmSpmChanged(int)), ui->csmSpmSpinner, SLOT(setValue(int)));

    connect(ui->decodeFramesCheckbox, SIGNAL(clicked(bool)), config, SLOT(setDecodeFrames(bool)));
    connect(config, SIGNAL(decodeFramesChanged(bool)), ui->decodeFramesCheckbox, SLOT(setChecked(bool)));

    connect(ui->gainModeCombobox, SIGNAL(activated(int)), config, SLOT(setGainMode(int)));
    connect(config, SIGNAL(gainModeChanged(int)), ui->gainModeCombobox, SLOT(setCurrentIndex(int)));

    connect(ui->polarityComboBox, SIGNAL(activated(int)), config, SLOT(setPolarity(int)));
    connect(config, SIGNAL(polarityChanged(int)), ui->polarityComboBox, SLOT(setCurrentIndex(int)));

    connect(ui->maxPacketSizeSpinner, SIGNAL(valueChanged(int)), config, SLOT(setMaxPacketSize(int)));
    connect(config, SIGNAL(MaxPacketSizeChanged(int)), ui->maxPacketSizeSpinner, SLOT(setValue(int)));

    connect(ui->nTriggersSpinner, SIGNAL(valueChanged(int)), config, SLOT(setNTriggers(int)));
    connect(config, SIGNAL(nTriggersChanged(int)), ui->nTriggersSpinner, SLOT(setValue(int)));
    // connection in the viewer
    connect(ui->nTriggersSpinner, SIGNAL(valueChanged(int)),
            _mpx3gui->getVisualization()->GetUI()->nTriggersSpinBox,
            SLOT(setValue(int)));

    connect(ui->operationModeComboBox, SIGNAL(activated(int)), config, SLOT(setOperationMode(int)));
    connect(config, SIGNAL(operationModeChanged(int)), ui->operationModeComboBox, SLOT(setCurrentIndex(int)));
    // connection ni the viewer
    connect(ui->operationModeComboBox, SIGNAL(activated(int)),
            _mpx3gui->getVisualization()->GetUI()->operationModeComboBox_Vis,
            SLOT(setCurrentIndex(int)));

    connect(ui->pixelDepthComboBox, SIGNAL(activated(int)), config, SLOT(setPixelDepthByIndex(int)));
    connect(config, SIGNAL(pixelDepthChanged(int)), ui->pixelDepthComboBox, SLOT(setCurrentIndex(int)));

    connect(ui->triggerLengthSpinner, SIGNAL(valueChanged(int)), config, SLOT(setTriggerLength(int)));
    connect(config, SIGNAL(TriggerLengthChanged(int)), ui->triggerLengthSpinner, SLOT(setValue(int)));
    // connection in the viewer
    connect(ui->triggerLengthSpinner, SIGNAL(valueChanged(int)),
            _mpx3gui->getVisualization()->GetUI()->triggerLengthSpinBox,
            SLOT(setValue(int)));

    connect(ui->triggerDowntimeSpinner, SIGNAL(editingFinished()), config, SLOT(setTriggerDowntime()));
    connect(config, SIGNAL(TriggerDowntimeChanged(int)), ui->triggerDowntimeSpinner, SLOT(setValue(int)));

    connect(ui->triggerModeSpinner, SIGNAL(valueChanged(int)), config, SLOT(setTriggerMode(int)));
    connect(config, SIGNAL(TriggerModeChanged(int)), ui->triggerModeSpinner, SLOT(setValue(int)));

    connect(ui->portSpinner, SIGNAL(valueChanged(int)), config, SLOT(setPort(int)));
    connect(config, SIGNAL(portChanged(int)), ui->portSpinner, SLOT(setValue(int)));

    connect(config, SIGNAL(IpAdressChanged(QString)), ui->ipLineEdit, SLOT(setText(QString)));
    //connect(ui->ipLineEdit, SIGNAL(textEdited(QString)), config, SLOT(setIpAddress(QString)));//Can't turn of keyboard tracking for this


    // stepper
    _stepper = 0x0;
    //connect(ui->stepperUseCalibCheckBox, SIGNAL(clicked(bool)), config, SLOT(setStepperConfigUseCalib(bool)));
    //connect(config, SIGNAL(UseCalibChanged(bool)), ui->stepperUseCalibCheckBox, SLOT(setChecked(bool)));

    //connect(ui->accelerationSpinBox, SIGNAL(valueChanged(double)), config, SLOT(setStepperConfigAcceleration(double)));
    //connect(config, SIGNAL(AccelerationChanged(double)), ui->accelerationSpinBox, SLOT(setValue(double)));

    //connect(ui->speedSpinBox, SIGNAL(valueChanged(double)), config, SLOT(setStepperConfigSpeed(double)));
    //connect(config, SIGNAL(SpeedChanged(double)), ui->speedSpinBox, SLOT(setValue(double)));

    // When the slider released, talk to the hardware
    QObject::connect( ui->motorDial, SIGNAL(sliderReleased()), this, SLOT(motorDialReleased()) );
    QObject::connect( ui->motorDial, SIGNAL(sliderMoved(int)), this, SLOT(motorDialMoved(int)) );

    // Camera
    connect(ui->cameraComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeCamera(int)));


    //ui->pixelDepthComboBox->addItem( QString::number( config->getPixelDepthFromIndex( 1 ), 'f', 0 ) ) ;
    //ui->pixelDepthComboBox->addItem( QString::number( config->getPixelDepthFromIndex( 2 ), 'f', 0 ) ) ;
    //ui->pixelDepthComboBox->addItem( QString::number( config->getPixelDepthFromIndex( 3 ), 'f', 0 ) ) ;
    //ui->pixelDepthComboBox->setCurrentIndex( config->getPixelDepth12BitsIndex() );

}

void QCstmConfigMonitoring::widgetInfoPropagation()
{
    // Part of this interface reflects on others, here's when
    // The operation mode shows also in the Visualization tab
    int nItems = ui->operationModeComboBox->count();
    for ( int ii = 0 ; ii < nItems ; ii++ ) {
        _mpx3gui->getVisualization()->GetUI()->operationModeComboBox_Vis->addItem( ui->operationModeComboBox->itemText( ii ) );
    }

}


void QCstmConfigMonitoring::timerEvent(QTimerEvent *) {

    readTemp();

}

void QCstmConfigMonitoring::readTemp() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    if ( spidrcontrol ) {

        int mdegrees;
        if( spidrcontrol->getRemoteTemp( &mdegrees ) ) {
            QString qs = QString("%1.%2").arg( mdegrees/1000 ).arg( mdegrees%1000, 3, 10, QChar('0') );
            ui->remoteTempMeasLabel->setText( qs );
        } else {
            ui->remoteTempMeasLabel->setText( "--.---" );
        }

        if( spidrcontrol->getLocalTemp( &mdegrees ) ) {
            QString qs = QString("%1.%2").arg( mdegrees/1000 ).arg( mdegrees%1000, 3, 10, QChar('0') );
            ui->localTempMeasLabel->setText( qs );
        } else {
            ui->localTempMeasLabel->setText( "--.---" );
        }

    }


    /*
    int mvolt, mamp, mwatt;
    if( _spidrController->getAvddNow( &mvolt, &mamp, &mwatt ) )
    {
        _lineEditAvddMvolt->setText( QString::number( mvolt ) );
        _lineEditAvddMwatt->setText( QString::number( mwatt ) );
        QString qs = QString("%1.%2").arg( mamp/10 ).arg( mamp%10 );
        _lineEditAvddMamp->setText( qs );
    }
    else
    {
        _lineEditAvddMvolt->setText( "----" );
        _lineEditAvddMamp->setText( "----" );
        _lineEditAvddMwatt->setText( "----" );
    }
    if( _spidrController->getDvddNow( &mvolt, &mamp, &mwatt ) )
    {
        _lineEditDvddMvolt->setText( QString::number( mvolt ) );
        _lineEditDvddMwatt->setText( QString::number( mwatt ) );
        QString qs = QString("%1.%2").arg( mamp/10 ).arg( mamp%10 );
        _lineEditDvddMamp->setText( qs );
    }
    else
    {
        _lineEditDvddMvolt->setText( "----" );
        _lineEditDvddMamp->setText( "----" );
        _lineEditDvddMwatt->setText( "----" );
    }
    if( !_skipVdd )
    {
        if( _spidrController->getVddNow( &mvolt, &mamp, &mwatt ) )
        {
            _lineEditVddMvolt->setText( QString::number( mvolt ) );
            _lineEditVddMwatt->setText( QString::number( mwatt ) );
            QString qs = QString("%1.%2").arg( mamp/10 ).arg( mamp%10 );
            _lineEditVddMamp->setText( qs );
        }
        else
        {
            _skipVdd = true; // SPIDR-TPX3 does not have VDD
            _lineEditVddMvolt->setText( "----" );
            _lineEditVddMamp->setText( "----" );
            _lineEditVddMwatt->setText( "----" );
        }
    }

    _leUpdateLed->show();
    QTimer::singleShot( UPDATE_INTERVAL_MS/4, this, SLOT(updateLedOff()) );
     */

}

void QCstmConfigMonitoring::on_SaveButton_clicked()//TODO: automatically append .json
{
    QFileDialog saveDialog(this, tr("Save configuration"), tr("./config"), tr("Json files (*.json)"));
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setDefaultSuffix("json");
    //saveDialog.setDirectory("./config");
    saveDialog.exec();
    QString filename = saveDialog.selectedFiles().first();
    //QFileDialog dialog;
    //dialog.setDefaultSuffix("json");//Bugged under Linux?
    //QString filename = dialog.getSaveFileName(this, tr("Save configuration"), tr("./config"), tr("Json files (*.json)"));
    _mpx3gui->getConfig()->toJsonFile(filename, ui->IncludeDacsCheck->isChecked());
}

void QCstmConfigMonitoring::on_LoadButton_clicked() {
    QString filename = QFileDialog::getOpenFileName(this, tr("Open configuration"), tr("./config"), tr("Json files (*.json)"));
    _mpx3gui->getConfig()->fromJsonFile(filename, ui->IncludeDacsCheck->isChecked());
}

void QCstmConfigMonitoring::on_ipLineEdit_editingFinished() {
    _mpx3gui->getConfig()->setIpAddress(ui->ipLineEdit->text());
}

void QCstmConfigMonitoring::on_ColourModeCheckBox_toggled(bool checked) {
    _mpx3gui->clear_data();
    if(checked)
        _mpx3gui->resize(_mpx3gui->getDataset()->x()/2, _mpx3gui->getDataset()->y()/2);
    else
        _mpx3gui->resize(_mpx3gui->getDataset()->x()*2, _mpx3gui->getDataset()->y()*2);
}

void QCstmConfigMonitoring::on_stepperUseCalibCheckBox_toggled(bool checked) {

    // On turn on --> verify and use calib, on turn off --> stop using calibration
    if ( checked == true ) {

        QStandardItemModel * model = (QStandardItemModel *) ui->stepperCalibrationTableView->model();
        int columns = model->columnCount();
        if(columns < 2) return; // this table is expected to have pairs <pos, angle>

        int rows = model->rowCount();

        vector<pair<double, double> > vals;
        // Check how many pairs available
        for ( int row = 0 ; row < rows ; row++ ) {

            QStandardItem * itempos = model->item(row, 0);
            QStandardItem * itemangle = model->item(row, 1);
            if( !itempos || !itemangle ) continue; // meaning no data or not a full pair in this row

            QVariant datapos = itempos->data(Qt::DisplayRole);
            QVariant dataang = itemangle->data(Qt::DisplayRole);

            // See if these can be converted to QString
            if ( datapos.canConvert<QString>() && dataang.canConvert<QString>() ) {
                // Now check if the vlues are alpha numeric
                QString posS = datapos.toString();
                QString angS = dataang.toString();

                bool posok = false;
                double pos = posS.toDouble( &posok );
                bool angok = false;
                double ang  = angS.toDouble( &angok );

                // push them if conversion is ok
                if( posok && angok ) vals.push_back( make_pair( pos, ang ) );

            }

        }

        // If calibration OK
        if ( _stepper->SetStepAngleCalibration( ui->motorIdSpinBox->value(), vals ) ) {
            cout << "[STEP] Calibrated with " << vals.size() << " points." << endl;
        } else {
            // User message
            string messg = "Insufficient points to calibrate or incorrect format.";
            QMessageBox::warning ( this, tr("Can't calibrate stepper"), tr( messg.c_str() ) );
            // force uncheck
            ui->stepperUseCalibCheckBox->setChecked( false );
            // and get out
            return;
        }

        // Now prepare the interface
        angleModeGUI();

        // In the angle space things may not be matching.
        // For instance step --> 3, means an angle of 2.7 (for 0.9 deg per 1 step resolution)
        // Make it match.
        int motorId = ui->motorIdSpinBox->value();
        double targetPos = ui->targetPosSpinBox->value();
        double targetAng = _stepper->StepToAngle(motorId, targetPos);
        if ( targetPos !=  targetAng ) {
            ui->targetPosSpinBox->setValue( targetAng );
        }
        QString posS;
        posS = QString::number( targetAng , 'f', 1 );
        ui->motorCurrentPoslcdNumber->display( posS );
        // If the wasn't a match between target and current I make it match here.
        // Go green.
        ui->motorCurrentPoslcdNumber->setPalette(Qt::green);
        ui->motorCurrentPoslcdNumber->setToolTip("Target reached.");

        // Do something about the range of the dial
        // With calibration we're talking about an angle.
        // Go from 0 to 360 deg
        ui->motorDial->setMinimum( 0 );
        ui->motorDial->setMaximum( 360 );

    } else {

        // Interface back to steps mode
        stepsModeGUI();

        // In the step space things may not be matching either.
        // Same situation as immediately above
        // Make it match.
        int motorId = ui->motorIdSpinBox->value();
        // In this case we inquiry the current angle
        double currentPos = _stepper->getCurrPos(motorId);
        //double currentAng = _stepper->StepToAngle(motorId, currentPos);
        //if ( currentPos !=  currentAng ) {
        ui->targetPosSpinBox->setValue( currentPos );
        //}
        QString posS = QString::number( currentPos , 'ldd', 0 );
        ui->motorCurrentPoslcdNumber->display( posS );

        // Do something about the range of the dial
        //int motorid = ui->motorIdSpinBox->value();
        map<int, motorPars> parsMap = _stepper->getPars( );
        // The minimum is normally -1*max. I will set zero here for convenience.
        ui->motorDial->setMinimum( 0 );
        // This is just an strategy to get a maximum of steps
        //  which makes almost a full turn.
        // _stepper->getPositionMax( motorid )
        ui->motorDial->setMaximum( parsMap[motorId].maxVel / 2  );

        //cout << "[STEP] pos min = "
        //		<< _stepper->getPositionMin( motorid )
        //		<< " | pos max = "
        //		<< _stepper->getPositionMax( motorid )
        //		<< endl;

    }
    // send to config
    _mpx3gui->getConfig()->setStepperConfigUseCalib( checked );

}

void QCstmConfigMonitoring::angleModeGUI(){
    ui->stepperTargetPosLabel->setText("Target angle:");
    ui->stepperCurrentPosLabel->setText("Current angle:");

}

void QCstmConfigMonitoring::stepsModeGUI(){
    ui->stepperTargetPosLabel->setText("Target pos:");
    ui->stepperCurrentPosLabel->setText("Current pos:");

}

void QCstmConfigMonitoring::on_cameraCheckBox_toggled(bool checked) {

    // temporarily disconnect the reaction to changes in the list
    disconnect(ui->cameraComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeCamera(int)));

    if ( ! _cameraOn && checked ) {

        cameraSearch();

        //cameraResize();

        cameraSetup();

        cameraOn();


    } else if ( ! checked ) {

        cameraOff();

    }

    // temporarily disconnect the reaction to changes in the list
    connect(ui->cameraComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeCamera(int)));

}

void QCstmConfigMonitoring::changeCamera(int index) {

    // temporarily disconnect the reaction to changes in the list
    disconnect(ui->cameraComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeCamera(int)));

    // turn off any previous camera
    cameraOff();

    // Pick up this camera
    cameraSearch( index );

    // temporarily disconnect the reaction to changes in the list
    connect(ui->cameraComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeCamera(int)));

    cameraSetup();

    cameraOn();

}

void QCstmConfigMonitoring::cameraResize() {

    QRect dockGeo = ui->videoDockWidget->geometry();

    cout << "QCstmConfigMonitoring::cameraResize() " << dockGeo.x() << ", " << dockGeo.y() << endl;

    //ui->videoWidget->setGeometry( dockGeo );

}

void QCstmConfigMonitoring::cameraOff() {

    if ( _imageCapture ) {
        delete _imageCapture; _imageCapture = 0x0;
    }

    if ( _viewfinder ) {
        delete _viewfinder; _viewfinder = 0x0;
    }

    if ( _camera ) {
        delete _camera;
        _camera = 0x0;
    }

    // empty the ui->cameraComboBox
    int nItems = ui->cameraComboBox->count();
    for (int i = 0 ; i < nItems ; i++) {
        ui->cameraComboBox->removeItem( 0 ); // you are always removing the first item
    }

    _cameraOn = false;

}

void QCstmConfigMonitoring::cameraSearch (int indexRequest){

    // temporarily disconnect the reaction to changes in the list
    disconnect(ui->cameraComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeCamera(int)));

    if (QCameraInfo::availableCameras().count() > 0) {
        cout << "[CAM ] Camera found.  Number of cameras: " << QCameraInfo::availableCameras().count() << endl;
    } else {
        string messg = "No camera present";
        QMessageBox::warning ( this, tr("Connect camera"), tr( messg.c_str() ) );
    }


    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    int index = 0;

    foreach (const QCameraInfo & cameraInfo, cameras) {

        QString cameraName = cameraInfo.deviceName();
        cout << "       [" << index << "] " << cameraName.toStdString() << endl;

        ui->cameraComboBox->insertItem( index++, cameraInfo.deviceName() );

        // Pick up the last camera if the indexRequest is -1.  Else pick the one requested
        if ( indexRequest == -1 && index == QCameraInfo::availableCameras().count() ) {
            _cameraId = index - 1;
            ui->cameraComboBox->setCurrentIndex( _cameraId );
            _camera = new QCamera(cameraInfo);
        } else if ( index - 1 == indexRequest ) {
            _cameraId = index - 1;
            ui->cameraComboBox->setCurrentIndex( _cameraId );
            _camera = new QCamera(cameraInfo);
        }

    }

    if( _camera ) _cameraOn = true;

    // connect it back
    connect(ui->cameraComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeCamera(int)));

}


void QCstmConfigMonitoring::cameraSetup() {

    // In C++, your choice depends on whether you are using widgets, or QGraphicsView.
    // The QCameraViewfinder class is used in the widgets case, and QGraphicsVideoItem
    // is useful for QGraphicsView.
    if ( ! _viewfinder ) _viewfinder = new QCameraViewfinder;
    _camera->setViewfinder(_viewfinder);
    _viewfinder->show();

    //cameraResize();

    //_viewfinder->setParent( ui->videoDockWidget );

    ui->videoDockWidget->setWidget( _viewfinder );
    ui->videoDockWidget->toggleViewAction();


}

void QCstmConfigMonitoring::cameraOn() {

    if ( ! _imageCapture ) _imageCapture = new QCameraImageCapture(_camera);
    _camera->setCaptureMode(QCamera::CaptureStillImage);
    _camera->start(); // Viewfinder frames start flowing

    //on half pressed shutter button
    _camera->searchAndLock();

    //on shutter button pressed
    _imageCapture->capture();

    //on shutter button released
    _camera->unlock();


}

void QCstmConfigMonitoring::on_stepperMotorCheckBox_toggled(bool checked) {

    // if the handler hasn't been initialized
    if ( ! _stepper ) _stepper = new StepperMotorController;

    // Make the table unsensitive with respect to the config for a moment.
    // This reconnects at the end of this function.
    // This line deals with all elements in the calibration table :)
    disconnect(ui->stepperCalibrationTableView->model(), SIGNAL(itemChanged(QStandardItem *)), _mpx3gui->getConfig(), SLOT(setStepperConfigCalib(QStandardItem *)));
    disconnect(ui->accelerationSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setAcceleration(double)) );
    disconnect(ui->speedSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setSpeed(double)) );
    disconnect(ui->currentISpinBox, SIGNAL(valueChanged(double)), this, SLOT(setCurrentILimit(double)) );

    // the missing boolead (ui->stepperUseCalibCheckBox) is managed by an implicit slot (on_stepperUseCalibCheckBox_toggled)

    // On turn on --> setup, on turn off --> close
    if ( checked == true ) {

        if ( ! _stepper->arm_stepper( ) ) return; // problems attaching

        // make stuff needed active
        activeInGUI();

        // Block the maximum and minimum number of motors
        ui->motorIdSpinBox->setMinimum( 0 );
        ui->motorIdSpinBox->setMaximum( _stepper->getNumMotors() - 1 );
        QString motorIdS = "supported motors: ";
        motorIdS += QString::number( _stepper->getNumMotors() , 'd', 0 );
        ui->motorIdSpinBox->setToolTip( motorIdS );

        // Get parameters to propagate to GUI
        map<int, motorPars> parsMap = _stepper->getPars( );

        int motorid = ui->motorIdSpinBox->value();

        // Speed
        ui->speedSpinBox->setMinimum( parsMap[motorid].minVel );
        ui->speedSpinBox->setMaximum( parsMap[motorid].maxVel );
        ui->speedSpinBox->setValue( parsMap[motorid].vel );
        QString speedS = "Set velocity from ";
        speedS += QString::number( parsMap[motorid].minVel , 'f', 1 );
        speedS += " to ";
        speedS += QString::number( parsMap[motorid].maxVel , 'f', 1 );
        ui->speedSpinBox->setToolTip( speedS );

        // Acc
        ui->accelerationSpinBox->setMinimum( parsMap[motorid].minAcc );
        ui->accelerationSpinBox->setMaximum( parsMap[motorid].maxAcc );
        ui->accelerationSpinBox->setValue( parsMap[motorid].acc );
        QString accS = "Set acceleration from ";
        accS += QString::number( parsMap[motorid].minAcc , 'f', 1 );
        accS += " to ";
        accS += QString::number( parsMap[motorid].maxAcc , 'f', 1 );
        ui->accelerationSpinBox->setToolTip( accS );

        // CurrentI limit
        ui->currentISpinBox->setMinimum( 0 );
        ui->currentISpinBox->setMaximum( 1.0 ); // TODO ... is this Amps ?
        ui->currentISpinBox->setValue( parsMap[motorid].currentILimit );
        QString clS = "Set current limit from ";
        clS += QString::number( 0 , 'f', 1 );
        clS += " to ";
        clS += QString::number( 1.0 , 'f', 1 );
        ui->currentISpinBox->setToolTip( clS );

        // From config //////////////////////////////////////////////////////////////////
        // The rest of the values should have been read from the configuration at startup
        double configAcc = _mpx3gui->getConfig()->getStepperAcceleration();
        if ( configAcc >= parsMap[motorid].minAcc && configAcc <= parsMap[motorid].maxAcc ) {
            ui->accelerationSpinBox->setValue( configAcc );
            setAcceleration( configAcc );
        }
        double configSpeed = _mpx3gui->getConfig()->getStepperSpeed();
        if ( configSpeed >= parsMap[motorid].minVel && configSpeed <= parsMap[motorid].maxVel ) {
            ui->speedSpinBox->setValue( configSpeed );
            setSpeed( configSpeed );
        }

        QStandardItemModel * model = (QStandardItemModel *) ui->stepperCalibrationTableView->model();
        QModelIndex index = model->index(0,0);
        double t1 = _mpx3gui->getConfig()->getStepperCalibPos0();
        QVariant var0( t1 );
        model->setData( index, var0 );

        index = model->index(0,1);
        QVariant var1( _mpx3gui->getConfig()->getStepperCalibAngle0() );
        model->setData( index, var1 );

        index = model->index(1,0);
        QVariant var2( _mpx3gui->getConfig()->getStepperCalibPos1() );
        model->setData( index, var2 );

        index = model->index(1,1);
        QVariant var3( _mpx3gui->getConfig()->getStepperCalibAngle1() );
        model->setData( index, var3 );

        if ( _mpx3gui->getConfig()->getStepperUseCalib() ) {
            ui->stepperUseCalibCheckBox->setChecked( true );
            // and call the corresponding actions as if the user had toggled it
            on_stepperUseCalibCheckBox_toggled( true );
        }


    } else {

        _stepper->disarm_stepper();
        // make stuff needed inactive
        activeInGUI();

    }

    // This line deals with all elements in the calibration table :)
    connect(ui->stepperCalibrationTableView->model(), SIGNAL(itemChanged(QStandardItem *)), _mpx3gui->getConfig(), SLOT(setStepperConfigCalib(QStandardItem *)));
    connect(ui->accelerationSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setAcceleration(double)) );
    connect(ui->speedSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setSpeed(double)) );
    connect(ui->currentISpinBox, SIGNAL(valueChanged(double)), this, SLOT(setCurrentILimit(double)) );

    // the missing boolead (ui->stepperUseCalibCheckBox) is managed by an implicit slot (on_stepperUseCalibCheckBox_toggled)

}

void QCstmConfigMonitoring::on_motorResetButton_clicked() {

    // Disarm
    _stepper->disarm_stepper();
    delete _stepper;
    _stepper = 0x0;

    // And arm again
    on_stepperMotorCheckBox_toggled( true );

}

/*
void QCstmConfigMonitoring::ConfigCalibAngle1Changed(double val) {
    QStandardItemModel * model = (QStandardItemModel *) ui->stepperCalibrationTableView->model();
    QModelIndex index = model->index(1,1);
    QVariant var(val);
    model->setData( index, var );
}
 */

void QCstmConfigMonitoring::on_stepperSetZeroPushButton_clicked() {

    // Current position is selected as zero

    // Reset the controller
    int motorId = ui->motorIdSpinBox->value();
    _stepper->setZero( motorId );

    // Reset displays
    ui->targetPosSpinBox->setValue( 0.0 );
    QString posS;
    if ( ui->stepperUseCalibCheckBox->isChecked() ) {
        posS = QString::number( 0.0 , 'f', 1 );
        ui->motorCurrentPoslcdNumber->display( posS );
    } else {
        posS = QString::number( 0 , 'lld', 0 );
        ui->motorCurrentPoslcdNumber->display( posS );
    }

}

void QCstmConfigMonitoring::on_motorGoToTargetButton_clicked() {

    ////////////////////////////////////////////////////////////////////////////
    // Update the data structure in StepperMotorController
    // 1) Target pos
    int motorId = ui->motorIdSpinBox->value();
    // fetch target pos and enter it in the structure
    double targetPos = ui->targetPosSpinBox->value();

    // Look if we are working calibrated or uncalibrated
    if ( ui->stepperUseCalibCheckBox->isChecked() ) {
        targetPos = _stepper->AngleToStep(motorId, (double)targetPos);
    }
    _stepper->setTargetPos( motorId, targetPos );

    // If curr and target are the same there's nothing to do here
    if ( _stepper->getCurrPos( motorId ) == _stepper->getTargetPos( motorId ) ) return;

    ////////////////////////////////////////////////////////////////////////////
    // 2) If a rotation is ready to be launched, send the command to the hardware
    // If in angle mode the entry of the user will be interpreted as an angle

    // 3) and launch the thread
    if ( ! _stepperThread ) {
        _stepperThread = new ConfigStepperThread( _mpx3gui, ui, this );
        connect(_stepperThread, SIGNAL(finished()), this, SLOT(stepperGotoTargetFinished()) );
    }

    _stepperThread->start();

    _stepper->goToTarget( (long long int)targetPos, motorId );

}

void QCstmConfigMonitoring::motorDialReleased() {

    // same behaviour as clicking on motorGoToTargetButton
    on_motorGoToTargetButton_clicked();

}

void QCstmConfigMonitoring::motorDialMoved(int val) {

    // Change the target pos
    ui->targetPosSpinBox->setValue( (double) val );

}

void QCstmConfigMonitoring::setAcceleration(double acc) {

    // avoid to set if the device is not connected
    if ( !_stepper ) return;

    // Set the new acceleration
    int motorId = ui->motorIdSpinBox->value();
    _stepper->SetAcceleration( motorId, acc );

    // send to config
    _mpx3gui->getConfig()->setStepperConfigAcceleration( acc );

}

void QCstmConfigMonitoring::setSpeed(double speed) {

    // avoid to set if the device is not connected
    if ( !_stepper ) return;

    // Set the new acceleration
    int motorId = ui->motorIdSpinBox->value();
    _stepper->SetSpeed(motorId, speed);

    // send to config
    _mpx3gui->getConfig()->setStepperConfigSpeed( speed );

}

void QCstmConfigMonitoring::setCurrentILimit(double limitI) {

    // avoid to set if the device is not connected
    if ( !_stepper ) return;

    // Set the new CurrentI
    int motorId = ui->motorIdSpinBox->value();
    _stepper->SetCurrentILimit(motorId, limitI);

    // send to config
    //_mpx3gui->getConfig()->

}


ConfigStepperThread::ConfigStepperThread(Mpx3GUI * mpx3gui, Ui::QCstmConfigMonitoring * ui, QCstmConfigMonitoring * stepperController) {

    _mpx3gui = mpx3gui;
    _ui = ui;
    _stepperController = stepperController;

}

/**
 * This Thread deals with the GUI refreshing while the engine reaches from Current position to Target position
 */
void ConfigStepperThread::run() {

    bool reachedTarget = false;
    int motorId = _ui->motorIdSpinBox->value();
    long long int curr_pos = 0;

    _ui->motorCurrentPoslcdNumber->setPalette(Qt::blue);

    // Keep running until the stepper has reached the desired position
    double posDisplay;
    while ( ! reachedTarget ) {

        // get current position
        curr_pos = _stepperController->getMotorController()->getCurrPos( motorId );

        // terminating condition good for both calibrated and uncalibrated
        if ( curr_pos ==  _stepperController->getMotorController()->getTargetPos( motorId ) ) reachedTarget = true;

        posDisplay = (double)curr_pos;

        QString posS;
        if ( _ui->stepperUseCalibCheckBox->isChecked() ) {

            posDisplay = _stepperController->getMotorController()->StepToAngle(motorId, (double)curr_pos);
            // Update display
            posS = QString::number( posDisplay , 'f', 1 );
            _ui->motorCurrentPoslcdNumber->display( posS );

        } else {
            // Update display
            posS = QString::number( curr_pos , 'lld', 0 );
            _ui->motorCurrentPoslcdNumber->display( posS );
        }

        // Update dial
        _ui->motorDial->setValue( (int)posDisplay );

    }

    // At the end the following may happen
    // Say the user request and angle of 3 degrees.  With a given resolution (ex. 0.9 deg per step)
    //  the bets the stepper can do is 2.7 degrees.  In that case color the LCD
    if ( posDisplay != _ui->targetPosSpinBox->value() ) {
        _ui->motorCurrentPoslcdNumber->setPalette(Qt::yellow);
        _ui->motorCurrentPoslcdNumber->setToolTip("Requested target couldn't be reached exactly.");
    } else {
        _ui->motorCurrentPoslcdNumber->setPalette(Qt::green);
        _ui->motorCurrentPoslcdNumber->setToolTip("Target reached.");
    }

    qDebug() << "thread done";

}


void QCstmConfigMonitoring::on_motorTestButton_clicked()
{

    // Follow a sequeance of angles and back-to-zero to test.
    // Every time on_motorGoToTargetButton_clicked is called,
    //  which launches the StepperThread. Then when the tread
    //  is finished and sends its signal we can continue
    //  checkin if the sequence has been exahusted.

    // Fill in a sequance
    m_stepperTestSequence.clear();

    /*
    m_stepperTestSequence.push_back( 45.0 );
    for ( int i = 1 ; i < 9 ; i++ ) {
        m_stepperTestSequence.push_back( 0.0 );           // a return to zero
        m_stepperTestSequence.push_back( 45.0 * i );    // and some test angle
    }
    m_stepperTestSequence.push_back( 0 ); // and come back to zero
    */

    m_stepperTestSequence.push_back( -15.0 );
    m_stepperTestSequence.push_back( 15.0 );

    m_stepperTestSequence.push_back( 0 ); // and come back to zero



    m_stepperTestCurrentStep = 0;

    /////////////////////////////////////////////////////////////////////////
    int motorId = ui->motorIdSpinBox->value();
    double targetPos = m_stepperTestSequence[m_stepperTestCurrentStep++];

    // by hand set the value
    ui->targetPosSpinBox->setValue( targetPos );

    // Look if we are working calibrated or uncalibrated
    if ( ui->stepperUseCalibCheckBox->isChecked() ) {
        targetPos = _stepper->AngleToStep(motorId, (double)targetPos);
    }
    _stepper->setTargetPos( motorId, targetPos );

    // And launch the thread
    if ( ! _stepperThread ) {

        _stepperThread = new ConfigStepperThread( _mpx3gui, ui, this );
        connect(_stepperThread, SIGNAL(finished()), this, SLOT(stepperGotoTargetFinished()) );

    }

    // go !
    _stepperThread->start();
    _stepper->goToTarget( (long long int)targetPos, motorId );


}


void QCstmConfigMonitoring::stepperGotoTargetFinished()
{

    // See if this is a sequence
    if ( m_stepperTestCurrentStep != 0
         &&
         m_stepperTestCurrentStep < m_stepperTestSequence.size() ) {

        qDebug() << m_stepperTestCurrentStep << " : " << m_stepperTestSequence.size();

        /////////////////////////////////////////////////////////////////////////
        int motorId = ui->motorIdSpinBox->value();
        double targetPos = m_stepperTestSequence[m_stepperTestCurrentStep++];

        // by hand set the value
        ui->targetPosSpinBox->setValue( targetPos );

        // Look if we are working calibrated or uncalibrated
        if ( ui->stepperUseCalibCheckBox->isChecked() ) {
            targetPos = _stepper->AngleToStep(motorId, (double)targetPos);
        }
        _stepper->setTargetPos( motorId, targetPos );

        // And launch the thread
        if ( ! _stepperThread ) {

            _stepperThread = new ConfigStepperThread( _mpx3gui, ui, this );
            connect(_stepperThread, SIGNAL(finished()), this, SLOT(stepperGotoTargetFinished()) );

        }

        // go !
        _stepperThread->start();
        _stepper->goToTarget( (long long int)targetPos, motorId );


    } else {
        // ready for next test sequence if needed
        m_stepperTestCurrentStep = 0;

    }

}
