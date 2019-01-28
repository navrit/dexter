#include "qcstmconfigmonitoring.h"
#include "ui_qcstmconfigmonitoring.h"
#include "mpx3config.h"

#include "SpidrController.h"
#include "StepperMotorController.h"
#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"

#include "qtableview.h"
#include "qstandarditemmodel.h"
#include "mpx3gui.h"
#include "SpidrDaq.h"
#include "MerlinInterface.h"

static QCstmConfigMonitoring* qCstmConfigMonitoring;

QCstmConfigMonitoring::QCstmConfigMonitoring(QWidget *parent) :
    QWidget(parent),

    ui(new Ui::QCstmConfigMonitoring) {
    ui->setupUi(this);

    _timerId = -1;
    //ui->samplingSpinner->setValue( 1.0 );

    // Gain
    // LSB FIRST for NO REASON !!!
    // 00: SHGM  0
    // 10: HGM   1
    // 01: LGM   2
    // 11: SLGM  3
    ui->gainModeCombobox->addItem("Super High - 7fF");
    ui->gainModeCombobox->addItem("High - 14fF");
    ui->gainModeCombobox->addItem("Low - 21fF");
    ui->gainModeCombobox->addItem("Super Low - 28fF");

    ui->polarityComboBox->addItem("Positive (holes)");
    ui->polarityComboBox->addItem("Negative (electrons)");

    ui->operationModeComboBox->addItem("Sequential R/W");
    ui->operationModeComboBox->addItem("Continuous R/W");

    // Pixel depth
    __pixelDepthMap.push_back( 1 );
    __pixelDepthMap.push_back( 6 );
    __pixelDepthMap.push_back( 12 );
    __pixelDepthMap.push_back( 24 );
    ui->pixelDepthCombo->addItem( QString("%1 bit").arg( __pixelDepthMap[0] ) );
    ui->pixelDepthCombo->addItem( QString("%1 bits").arg( __pixelDepthMap[1] ) );
    ui->pixelDepthCombo->addItem( QString("%1 bits").arg( __pixelDepthMap[2] ) );
    ui->pixelDepthCombo->addItem( QString("%1 bits").arg( __pixelDepthMap[3] ) );
    ui->pixelDepthCombo->setCurrentIndex( 2 );

    // Trigger mode
    __triggerModeMap.push_back( SHUTTERMODE_AUTO );
    __triggerModeMap.push_back( SHUTTERMODE_POS_EXT );
    __triggerModeMap.push_back( SHUTTERMODE_NEG_EXT );
    __triggerModeMap.push_back( SHUTTERMODE_POS_EXT_TIMER );
    __triggerModeMap.push_back( SHUTTERMODE_NEG_EXT_TIMER );
    __triggerModeMap.push_back( SHUTTERTRIG_POS_EXT_CNTR );

    ui->triggerModeCombo->addItem( "Auto" );
    ui->triggerModeCombo->addItem( "Positive Ext" );
    ui->triggerModeCombo->addItem( "Negative Ext" );
    ui->triggerModeCombo->addItem( "Positive Ext Timer" );
    ui->triggerModeCombo->addItem( "Negative Ext Timer" );
    ui->triggerModeCombo->addItem( "Positive Ext Counter" );

    ui->triggerModeCombo->setCurrentIndex( 0 );

    // Configurable clock
    ui->mpx3ClockComboBox->setEnabled( false );
    //ui->mpx3ClockComboBox->addItem("64");
    //ui->mpx3ClockComboBox->addItem("100");
    //ui->mpx3ClockComboBox->addItem("128");
    //ui->mpx3ClockComboBox->addItem("200");

    // CSM_SPM
    __csmSpmMap.push_back( 0 );
    __csmSpmMap.push_back( 1 );
    ui->csmSpmCombo->addItem( "OFF" );
    ui->csmSpmCombo->addItem( "ON" );
    ui->csmSpmCombo->setCurrentIndex( 0 );

    // Time units
    ui->radioButtonS->setEnabled( false );
    ui->radioButtonMS->setEnabled( false );

    QFont font1("Courier New");
    ui->omrDisplayLabel->setFont( font1 );
    ui->omrDisplayLabel->setTextFormat( Qt::RichText );


    //! Disable a bunch of 'advanced' buttons
    developerMode(false);

    qCstmConfigMonitoring = this;
}

unsigned int QCstmConfigMonitoring::getPixelDepthFromIndex(int indx) {

    int sizev = int(__pixelDepthMap.size());
    if ( indx >= sizev ) return __pixelDepthMap[ __pixelDepth12BitsIndex ]; // 12 bits

    return __pixelDepthMap[std::size_t(indx)];
}

void QCstmConfigMonitoring::setReadoutFrequency(int frequency)
{
    ui->contRWFreq->setValue(frequency);
}

void QCstmConfigMonitoring::protectTriggerMode(SpidrController *spidrController)
{
//    _currentTriggerMode = ui->triggerModeCombo->currentIndex();
//    ui->triggerModeCombo->setCurrentIndex(0); //set it to auto
    spidrController->stopAutoTrigger();
    spidrController->getShutterTriggerConfig(&shutterInfo.trigger_mode,&shutterInfo.trigger_width_us,&shutterInfo.trigger_freq_mhz,&shutterInfo.nr_of_triggers,&shutterInfo.trigger_pulse_count);
    spidrController->setShutterTriggerConfig(SHUTTERMODE_AUTO,shutterInfo.trigger_width_us,shutterInfo.trigger_freq_mhz,shutterInfo.nr_of_triggers,shutterInfo.trigger_pulse_count);
    usleep(100000);
    qDebug() << "[Info] Trigger is set to : " << SHUTTERMODE_AUTO;
}

void QCstmConfigMonitoring::returnLastTriggerMode(SpidrController *spidrController)
{
    //ui->triggerModeCombo->setCurrentIndex(_currentTriggerMode);
    spidrController->setShutterTriggerConfig(shutterInfo.trigger_mode,shutterInfo.trigger_width_us,shutterInfo.trigger_freq_mhz,shutterInfo.nr_of_triggers,shutterInfo.trigger_pulse_count);
    qDebug() << "[Info] Trigger is set back to : " << shutterInfo.trigger_mode;
}

void QCstmConfigMonitoring::returnLastTriggerMode2(SpidrController *spidrController)
{
    spidrController->getShutterTriggerConfig(&shutterInfo.trigger_mode,&shutterInfo.trigger_width_us,&shutterInfo.trigger_freq_mhz,&shutterInfo.nr_of_triggers,&shutterInfo.trigger_pulse_count);
    spidrController->setShutterTriggerConfig(_mpx3gui->getConfig()->getTriggerMode(),shutterInfo.trigger_width_us,shutterInfo.trigger_freq_mhz,shutterInfo.nr_of_triggers,shutterInfo.trigger_pulse_count);
    qDebug() << "[Info] Trigger mode is set to : " << _mpx3gui->getConfig()->getTriggerMode();
}



QCstmConfigMonitoring::~QCstmConfigMonitoring()
{
    delete ui;
}

QCstmConfigMonitoring *QCstmConfigMonitoring::getInstance()
{
    return qCstmConfigMonitoring;
}


void QCstmConfigMonitoring::on_tempReadingActivateCheckBox_toggled(bool checked) {

    if ( checked ) {
        // Get a quick read first to let the user see immediately and start the timer.
        // The timer will keep refreshing periodically.
        readMonitoringInfo();
        _timerId = this->startTimer(int(ui->samplingSpinner->value()*1000)); // seconds from GUI. Convert to ms.

        ui->remoteTempMeasLineEdit->setEnabled( true );
        ui->localTempMeasLineEdit->setEnabled( true );
        ui->FpgaTempMeasLineEdit->setEnabled( true );
        ui->biasVoltageMeasLineEdit->setEnabled( true );
        ui->systemClockLineEdit->setEnabled( true );
        ui->humidityMeasureLineEdit->setEnabled( true );
        ui->pressureMeasureLineEdit->setEnabled( true );

        ui->avddmamp->setEnabled( true );
        ui->avddmvolt->setEnabled( true );
        ui->avddmwatt->setEnabled( true );

        ui->vddmamp->setEnabled( true );
        ui->vddmvolt->setEnabled( true );
        ui->vddmwatt->setEnabled( true );

        ui->dvddmamp->setEnabled( true );
        ui->dvddmvolt->setEnabled( true );
        ui->dvddmwatt->setEnabled( true );

    } else {

        this->killTimer( _timerId );
        _timerId = -1;

        // clean the temp display
        ui->remoteTempMeasLineEdit->setText("");
        ui->localTempMeasLineEdit->setText("");
        ui->FpgaTempMeasLineEdit->setText("");
        ui->biasVoltageMeasLineEdit->setText("");
        ui->systemClockLineEdit->setText("");
        ui->humidityMeasureLineEdit->setText("");
        ui->pressureMeasureLineEdit->setText("");

        ui->avddmamp->setText("");
        ui->avddmvolt->setText("");
        ui->avddmwatt->setText("");

        ui->vddmamp->setText("");
        ui->vddmvolt->setText("");
        ui->vddmwatt->setText("");

        ui->dvddmamp->setText("");
        ui->dvddmvolt->setText("");
        ui->dvddmwatt->setText("");

        ui->remoteTempMeasLineEdit->setEnabled( false );
        ui->localTempMeasLineEdit->setEnabled( false );
        ui->FpgaTempMeasLineEdit->setEnabled( false );
        ui->biasVoltageMeasLineEdit->setEnabled( false );
        ui->systemClockLineEdit->setEnabled( false );
        ui->humidityMeasureLineEdit->setEnabled( false );
        ui->pressureMeasureLineEdit->setEnabled( false );

        ui->avddmamp->setEnabled( false );
        ui->avddmvolt->setEnabled( false );
        ui->avddmwatt->setEnabled( false );

        ui->vddmamp->setEnabled( false );
        ui->vddmvolt->setEnabled( false );
        ui->vddmwatt->setEnabled( false );

        ui->dvddmamp->setEnabled( false );
        ui->dvddmvolt->setEnabled( false );
        ui->dvddmwatt->setEnabled( false );
    }

}

void QCstmConfigMonitoring::on_readOMRPushButton_clicked() {

    QMessageBox::StandardButton ans =
            QMessageBox::warning(this,
                   tr("Warning"),
                   tr("This will set the device to a mode unsuitable for imaging, you will have to power cycle to image again. Continue?"));
    if ( ans == QMessageBox::No ) {
            return;
    }

    int  dev_nr = 2;
    uchar omr[6];

    uchar endMask = uchar(0x100); // 1 00000000 (ninth bit)
    char bits16Save[__nbits_OMR];
    int lateRunningBitCntr = 0;
    int bitCntr = 0;

    QString toDisplay;
    QString formatNoSpace_gui;
    QString formatWithSpace_gui;
    QString formatNoSpace_console;
    QString formatWithSpace_console;

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    if (!spidrcontrol->isConnected()) {
        return;
    }
    // This will crash if not connected
    spidrcontrol->getOmr( dev_nr, omr );

    cout << "[OMR ]" << endl;

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

            mask = uchar(mask << 1); // shift a bit to the left
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

    return;
}

void QCstmConfigMonitoring::when_taking_data_gui()
{
    ui->groupBoxConfiguration->setEnabled( false );
}

void QCstmConfigMonitoring::when_idling_gui()
{
    ui->groupBoxConfiguration->setEnabled( true );
}

void QCstmConfigMonitoring::developerMode(bool enabled)
{
    if (enabled) {
        _isDeveloperMode = true;

        //! Enable a bunch of 'advanced' buttons
        ui->readOMRPushButton->show();
        ui->logLevelSpinner->show();
        ui->label_logLevel->show();
        ui->label_timeUnits->show();
        ui->radioButtonS->show();
        ui->radioButtonMS->show();
        ui->radioButtonUS->show();
        ui->mpx3ClockComboBox->show();
        ui->label_clockMHz->show();
        ui->IP_ZMQ_PUB_lineEdit->show();
        ui->IP_ZMQ_SUB_lineEdit->show();
        ui->label_zmq_pub->show();
        ui->label_zmq_sub->show();
        ui->merlinInterfaceLineEdit->show();
        ui->merlinInterfaceTestButton->show();
    } else {
        _isDeveloperMode = false;

        //! Disable a bunch of 'advanced' buttons
        ui->readOMRPushButton->hide();
        ui->logLevelSpinner->hide();
        ui->label_logLevel->hide();
        ui->label_timeUnits->hide();
        ui->radioButtonS->hide();
        ui->radioButtonMS->hide();
        ui->mpx3ClockComboBox->hide();
        ui->label_clockMHz->hide();
        ui->radioButtonUS->hide();
        ui->IP_ZMQ_PUB_lineEdit->hide();
        ui->IP_ZMQ_SUB_lineEdit->hide();
        ui->label_zmq_pub->hide();
        ui->label_zmq_sub->hide();
        ui->merlinInterfaceLineEdit->hide();
        ui->merlinInterfaceTestButton->hide();
    }
}


void QCstmConfigMonitoring::hideLabels()
{
    if ( _mpx3gui->getConfig()->getOperationMode() == Mpx3Config::__operationMode_SequentialRW ) {
        ui->label_23->hide();
        ui->label_8->show();
        ui->label_17->show();
    } else {
        ui->label_23->show();
        ui->label_8->hide();
        ui->label_17->hide();
    }
}

void QCstmConfigMonitoring::shortcutGainModeSLGM()
{
    // 11: SLGM  3
    ui->gainModeCombobox->setCurrentIndex(3);
}

void QCstmConfigMonitoring::shortcutGainModeLGM()
{
    // 01: LGM   2
    ui->gainModeCombobox->setCurrentIndex(2);
}

void QCstmConfigMonitoring::shortcutGainModeHGM()
{
    // 10: HGM   1
    ui->gainModeCombobox->setCurrentIndex(1);
}

void QCstmConfigMonitoring::shortcutGainModeSHGM()
{
    // 00: SHGM  0
    ui->gainModeCombobox->setCurrentIndex(0);
}

void QCstmConfigMonitoring::shortcutCSMOff()
{
    int index = ui->csmSpmCombo->findText("OFF");
    ui->csmSpmCombo->setCurrentIndex(index);
}

void QCstmConfigMonitoring::shortcutCSMOn()
{
    int index = ui->csmSpmCombo->findText("ON");
    ui->csmSpmCombo->setCurrentIndex(index);
}

void QCstmConfigMonitoring::ConnectionStatusChanged(bool conn) {

    if ( conn ) {
        setWindowWidgetsStatus( win_status::connected );
        ui->readOMRPushButton->setEnabled(true);

    } else {
        setWindowWidgetsStatus( win_status::disconnected );
        ui->readOMRPushButton->setEnabled(false);
    }
}

void QCstmConfigMonitoring::diable24BitMode(int idx)
{
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(ui->pixelDepthCombo->model());
    Q_ASSERT(model != nullptr);
    QStandardItem *item = model->item(3);
    qDebug() << "index is :"<<idx;
    if(idx == 1){
        if(ui->pixelDepthCombo->currentIndex() == 3)
            ui->pixelDepthCombo->setCurrentIndex(2);
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }
    else
        item->setFlags(item->flags() |  Qt::ItemIsEnabled);
}

void QCstmConfigMonitoring::SetMpx3GUI(Mpx3GUI *p) {

    _mpx3gui = p;
    Mpx3Config *config = _mpx3gui->getConfig();

    // nTriggers
    connect(ui->nTriggersSpinner, SIGNAL( editingFinished() ), this, SLOT( nTriggersEdited() ) );
    connect(config, SIGNAL(nTriggersChanged(int)), ui->nTriggersSpinner, SLOT(setValue(int)));
    // connection in the viewer
    connect(ui->nTriggersSpinner, SIGNAL(valueChanged(int)),
            _mpx3gui->getVisualization()->GetUI()->nTriggersSpinBox,
            SLOT(setValue(int)));

    // contRW
    connect(config, SIGNAL(ContRWFreqChanged(int)), this, SLOT(setContRWFreqFromConfig(int)));
    connect(config, SIGNAL(ContRWFreqChanged(int)), ui->contRWFreq, SLOT(setValue(int))); /* Links to ui->contRW valueChanged(int) */
    connect(ui->contRWFreq, SIGNAL(valueChanged(int)), this, SLOT(ContRWFreqEdited()));

    // connection in the viewer
    connect(ui->triggerLengthSpinner, SIGNAL(valueChanged(int)),
            _mpx3gui->getVisualization()->GetUI()->triggerLengthSpinBox,
            SLOT(setValue(int)));

    // Shutter Length
    connect(ui->triggerLengthSpinner, SIGNAL(editingFinished()), this, SLOT(TriggerLengthEdited()) );
    connect(config, SIGNAL(TriggerLengthChanged(int)), ui->triggerLengthSpinner, SLOT(setValue(int)));

    // Trigger Down
    connect(ui->triggerDowntimeSpinner, SIGNAL(editingFinished()), this, SLOT(TriggerDowntimeEdited()) );
    connect(config, SIGNAL(TriggerDowntimeChanged(int)), ui->triggerDowntimeSpinner, SLOT(setValue(int)));

    // Operation Mode
    connect(ui->operationModeComboBox, SIGNAL(currentIndexChanged(int)), config, SLOT(setOperationMode(int)));
    connect(ui->operationModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(diable24BitMode(int)));
    connect(config, SIGNAL(operationModeChanged(int)), ui->operationModeComboBox, SLOT(setCurrentIndex(int)));
    connect(config, SIGNAL(operationModeChanged(int)), this, SLOT(hideLabels()));
    connect(config, SIGNAL(operationModeChanged(int)), this, SLOT(setMaximumFPSinVisualiation()));

    // connection in the viewer
    connect(ui->operationModeComboBox, SIGNAL(currentIndexChanged(int)),
            _mpx3gui->getVisualization()->GetUI()->operationModeComboBox_Vis,
            SLOT(setCurrentIndex(int)));
    // extra actions on OperationMode change
    connect(ui->operationModeComboBox, SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(OperationModeSwitched(int)));
    connect(ui->operationModeComboBox, SIGNAL(currentIndexChanged(int)),
            _mpx3gui->getVisualization(),
            SLOT(OperationModeSwitched(int)));

    // Polarity
    connect(ui->polarityComboBox, SIGNAL(currentIndexChanged(int)), config, SLOT(setPolarity(int)));
    connect(config, SIGNAL(polarityChanged(int)), ui->polarityComboBox, SLOT(setCurrentIndex(int)));

    // Gain Mode
    connect(ui->gainModeCombobox, SIGNAL(currentIndexChanged(int)), config, SLOT(setGainMode(int)));
    connect(config, SIGNAL(gainModeChanged(int)), ui->gainModeCombobox, SLOT(setCurrentIndex(int)));

    // Pixel Depth
    connect(ui->pixelDepthCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setPixelDepthByIndex(int)) );
    connect(config, SIGNAL(pixelDepthChanged(int)), this, SLOT(pixelDepthChangedByValue(int)) );

    // Trigger mode
    connect(ui->triggerModeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setTriggerModeByIndex(int)) );
    connect(config, SIGNAL(TriggerModeChanged(int)), this, SLOT( triggerModeChangedByValue(int) ) );

    // Bias Voltage
    connect(ui->biasVoltageSpinner, SIGNAL(editingFinished()), this, SLOT(biasVoltageChanged()) );
    connect(config, SIGNAL(BiasVoltageChanged(double)), ui->biasVoltageSpinner, SLOT(setValue(double)));

    // CSM SPM
    connect(ui->csmSpmCombo, SIGNAL(currentIndexChanged(int)), this, SLOT( setCsmSpmByIndex(int) ) );
    connect(config, SIGNAL(csmSpmChanged(int)), this, SLOT( csmSpmChangedByValue(int)) );



    // BothCounters
    connect(ui->readBothCountersCheckBox, SIGNAL(toggled(bool)), config, SLOT(setReadBothCounters(bool)));
    connect(config, SIGNAL(readBothCountersChanged(bool)), ui->readBothCountersCheckBox, SLOT(setChecked(bool)));

    // ColourMode
    connect(ui->ColourModeCheckBox, SIGNAL(toggled(bool)), config, SLOT(setColourMode(bool)));
    connect(config, SIGNAL(colourModeChanged(bool)), ui->ColourModeCheckBox, SLOT(setChecked(bool)));

    //inhibit_shutter enable
    connect(ui->inhibitShutterCheckBox, SIGNAL(toggled(bool)), config, SLOT(setInhibitShutter(bool)));
    connect(config,SIGNAL(inhibitShutterchanged(bool)),ui->inhibitShutterCheckBox,SLOT(setChecked(bool)));

    // DecodeFrames
    /* Always hide this, don't delete it in case we want to change it easily later. */
    connect(ui->decodeFramesCheckbox, SIGNAL(toggled(bool)), config, SLOT(setDecodeFrames(bool)));
    connect(config, SIGNAL(decodeFramesChanged(bool)), ui->decodeFramesCheckbox, SLOT(setChecked(bool)));
    ui->decodeFramesCheckbox->setVisible(false);

    // IP and Port
    connect(ui->ipLineEdit, SIGNAL( editingFinished() ), this, SLOT( IpAddressEditFinished() ) );// config, SLOT(setIpAddress(QString)) );
    connect(config, SIGNAL(IpAdressChanged(QString)), ui->ipLineEdit, SLOT(setText(QString)) );

    // ZMQ IP and Port (x2)
    connect(ui->IP_ZMQ_PUB_lineEdit, SIGNAL( editingFinished() ), this, SLOT( IpZmqPubAddressEditFinished() ) );
    connect(ui->IP_ZMQ_SUB_lineEdit, SIGNAL( editingFinished() ), this, SLOT( IpZmqSubAddressEditFinished() ) );

    connect(config, SIGNAL(IpZmqPubAddressChanged(QString)), ui->IP_ZMQ_PUB_lineEdit, SLOT(setText(QString)) );
    connect(config, SIGNAL(IpZmqSubAddressChanged(QString)), ui->IP_ZMQ_SUB_lineEdit, SLOT(setText(QString)) );
    connect(config, SIGNAL(IpZmqPubAddressChangedFailed(QString)), ui->IP_ZMQ_PUB_lineEdit, SLOT(setText(QString)) );
    connect(config, SIGNAL(IpZmqSubAddressChangedFailed(QString)), ui->IP_ZMQ_SUB_lineEdit, SLOT(setText(QString)) );

    // Log level
    connect(ui->logLevelSpinner, SIGNAL( editingFinished() ), this, SLOT( setLogLevel() ) );
    connect(config, SIGNAL(logLevelChanged(int)), ui->logLevelSpinner, SLOT(setValue(int)) );

}

void QCstmConfigMonitoring::nTriggersEdited() {

    _mpx3gui->getConfig()->setNTriggers(
                ui->nTriggersSpinner->value()
                );
}

void QCstmConfigMonitoring::ContRWFreqEdited()
{
    int idx = ui->pixelDepthCombo->currentIndex();
    int idx_1_bit = ui->pixelDepthCombo->findText("1 bit");
    int idx_6_bit = ui->pixelDepthCombo->findText("6 bits");
    int idx_12_bit = ui->pixelDepthCombo->findText("12 bits");
    int idx_24_bit = ui->pixelDepthCombo->findText("24 bits");

    if (idx == idx_1_bit) {
        ui->contRWFreq->setMaximum(__maximumFPS_1_bit);
        _mpx3gui->getVisualization()->setMaximumContRW_FPS(__maximumFPS_1_bit);

    } else if (idx == idx_6_bit) {
        ui->contRWFreq->setMaximum(__maximumFPS_6_bit);
        _mpx3gui->getVisualization()->setMaximumContRW_FPS(__maximumFPS_6_bit);

    } else if (idx == idx_12_bit) {
        ui->contRWFreq->setMaximum(__maximumFPS_12_bit);
        _mpx3gui->getVisualization()->setMaximumContRW_FPS(__maximumFPS_12_bit);

    } else if (idx == idx_24_bit) {
        ui->contRWFreq->setMaximum(__maximumFPS_24_bit);
        _mpx3gui->getVisualization()->setMaximumContRW_FPS(__maximumFPS_24_bit);
    }

    _mpx3gui->getConfig()->setContRWFreq( ui->contRWFreq->value() );
}

void QCstmConfigMonitoring::TriggerLengthEdited()
{
    _mpx3gui->getConfig()->setTriggerLength(
                ui->triggerLengthSpinner->value()
                );
}

void QCstmConfigMonitoring::TriggerDowntimeEdited()
{
    _mpx3gui->getConfig()->setTriggerDowntime(
                ui->triggerDowntimeSpinner->value()
                );
}

void QCstmConfigMonitoring::biasVoltageChanged(){

    _mpx3gui->getConfig()->setBiasVoltage(
                ui->biasVoltageSpinner->value()
                );

}

void QCstmConfigMonitoring::setLogLevel(){
    _mpx3gui->getConfig()->setLogLevel(
                ui->logLevelSpinner->value()
                );
}

void QCstmConfigMonitoring::OperationModeSwitched(int indx)
{
    if ( indx == Mpx3Config::__operationMode_SequentialRW ) {
        ui->triggerLengthSpinner->setEnabled( true );
        ui->triggerLengthSpinner->show();
        ui->triggerDowntimeSpinner->setEnabled( true );
        ui->triggerDowntimeSpinner->show();

        ui->label_23->hide();
        ui->label_8->show();
        ui->label_17->show();

        ui->contRWFreq->setEnabled( false );
        ui->contRWFreq->hide();

        ui->readBothCountersCheckBox->show();
        ui->readBothCountersCheckBox->setEnabled( true );
    } else if ( indx == Mpx3Config::__operationMode_ContinuousRW ) {
        ui->triggerLengthSpinner->setEnabled( false );
        ui->triggerLengthSpinner->hide();
        ui->triggerDowntimeSpinner->setEnabled( false );
        ui->triggerDowntimeSpinner->hide();

        ui->label_8->hide();
        ui->label_17->hide();
        ui->label_23->show();

        ui->contRWFreq->setEnabled( true );
        ui->contRWFreq->show();

        ui->readBothCountersCheckBox->setChecked( false );
        ui->readBothCountersCheckBox->hide();
    }
}

void QCstmConfigMonitoring::setPixelDepthByIndex(int newValIndx) {
    _mpx3gui->getConfig()->setPixelDepth(int(__pixelDepthMap[std::size_t(newValIndx)]));
    setMaximumFPSFromPixelDepth(newValIndx, -1);
}

void QCstmConfigMonitoring::setTriggerModeByIndex(int newValIndx) {
   // _mpx3gui->getConfig()->setTriggerMode(int(__triggerModeMap[std::size_t(newValIndx)]));
    switch (newValIndx) {
    case 0:
        _mpx3gui->getConfig()->setTriggerMode(SHUTTERMODE_AUTO);
        break;
    case 1:
        _mpx3gui->getConfig()->setTriggerMode(SHUTTERMODE_POS_EXT);
        break;
    case 2:
        _mpx3gui->getConfig()->setTriggerMode(SHUTTERMODE_NEG_EXT);
        break;
    case 3:
        _mpx3gui->getConfig()->setTriggerMode(SHUTTERMODE_POS_EXT_TIMER);
        break;
    case 4:
        _mpx3gui->getConfig()->setTriggerMode(SHUTTERMODE_NEG_EXT_TIMER);
        break;
    case 5:
        _mpx3gui->getConfig()->setTriggerMode(SHUTTERTRIG_POS_EXT_CNTR);
        break;
    default:
        break;
    }
}

int QCstmConfigMonitoring::findTriggerModeIndex(int val) {
    std::size_t sizea = __triggerModeMap.size();
    for (std::size_t i = 0 ; i < sizea ; i++) {
        if ( __triggerModeMap[i] == std::size_t(val)) {
            return int(i);
        }
    }
    return -1;
}

void QCstmConfigMonitoring::setMaximumFPSFromPixelDepth(int idx, int val)
{
    if (idx == -1 && val == -1) {
        /* Get the index from the GUI */

        idx = ui->pixelDepthCombo->currentIndex();

    } else if (idx == -1 && val != -1) {
        /* Get the value directly */

        if (val == 1) {
            ui->contRWFreq->setMaximum(__maximumFPS_1_bit);
            _mpx3gui->getVisualization()->setMaximumContRW_FPS(__maximumFPS_1_bit);
        } else if (val == 6) {
            ui->contRWFreq->setMaximum(__maximumFPS_6_bit);
            _mpx3gui->getVisualization()->setMaximumContRW_FPS(__maximumFPS_6_bit);
        } else if (val == 12) {
            ui->contRWFreq->setMaximum(__maximumFPS_12_bit);
            _mpx3gui->getVisualization()->setMaximumContRW_FPS(__maximumFPS_12_bit);
        } else if (val == 24) {
            ui->contRWFreq->setMaximum(__maximumFPS_24_bit);
            _mpx3gui->getVisualization()->setMaximumContRW_FPS(__maximumFPS_24_bit);
        }

    } else if (val == -1 && idx != -1) {
        /* Get the value by finding the item in the list */

        int idx_1_bit = ui->pixelDepthCombo->findText("1 bit");
        int idx_6_bit = ui->pixelDepthCombo->findText("6 bits");
        int idx_12_bit = ui->pixelDepthCombo->findText("12 bits");
        int idx_24_bit = ui->pixelDepthCombo->findText("24 bits");

        if (idx == idx_1_bit) {
            ui->contRWFreq->setMaximum(__maximumFPS_1_bit);
            _mpx3gui->getVisualization()->setMaximumContRW_FPS(__maximumFPS_1_bit);

        } else if (idx == idx_6_bit) {
            ui->contRWFreq->setMaximum(__maximumFPS_6_bit);
            _mpx3gui->getVisualization()->setMaximumContRW_FPS(__maximumFPS_6_bit);

        } else if (idx == idx_12_bit) {
            ui->contRWFreq->setMaximum(__maximumFPS_12_bit);
            _mpx3gui->getVisualization()->setMaximumContRW_FPS(__maximumFPS_12_bit);

        } else if (idx == idx_24_bit) {
            ui->contRWFreq->setMaximum(__maximumFPS_24_bit);
            _mpx3gui->getVisualization()->setMaximumContRW_FPS(__maximumFPS_24_bit);
        }
    }

    return;
}

int QCstmConfigMonitoring::getTriggerModeIndex() {
    return findTriggerModeIndex(_mpx3gui->getConfig()->getTriggerMode());
}

void QCstmConfigMonitoring::setCsmSpmByIndex(int newValIndx)
{
    _mpx3gui->getConfig()->setCsmSpm(int(__csmSpmMap[std::size_t(newValIndx)]));

    int idx_off = ui->csmSpmCombo->findText("OFF");
    int idx_on = ui->csmSpmCombo->findText("ON");

    if (_isDeveloperMode) {
        /* FREEDOM!!! */
        /* Make sure it's visible and enabled. */
        /* The user gets to choose if they want to turn it off now */
        /* Do not change the checked state */

        ui->readBothCountersCheckBox->show();
        ui->readBothCountersCheckBox->setEnabled(true);
    } else {
        /* Make sure it's visible and enabled. */
        /* The user is assumed not to know everything */
        /* So, double counter readout is automatically checked based on if it should be on or off */

        ui->readBothCountersCheckBox->show();
        ui->readBothCountersCheckBox->setEnabled(true);
        if (newValIndx == idx_off) {
            /* CSM = OFF */
            ui->readBothCountersCheckBox->setChecked(false);
        } else if (newValIndx == idx_on) {
            /* CSM = ON */
            ui->readBothCountersCheckBox->setChecked(true);
        } else {
            qDebug() << "[ERROR]\tWTF happened here, OFF or ON isn't in the csmSpmCombo box???";
        }
    }

    return;
}

void QCstmConfigMonitoring::csmSpmChangedByValue(int val)
{
    int sizea = int(__csmSpmMap.size());
    for ( int i = 0; i < sizea; i++ ) {
        if ( __csmSpmMap[i] == val) {
            ui->csmSpmCombo->setCurrentIndex( i );
            return;
        }
    }
    ui->csmSpmCombo->setCurrentIndex( 0 );
}

void QCstmConfigMonitoring::IpAddressEditFinished()
{
    // The string should be of the form
    //  192.168.1.10:50000
    QString ipLine = ui->ipLineEdit->text();
    _mpx3gui->getConfig()->setIpAddress( ipLine );

}

void QCstmConfigMonitoring::IpZmqPubAddressEditFinished()
{
    QString ip_and_port = ui->IP_ZMQ_PUB_lineEdit->text();
    _mpx3gui->getConfig()->setIpZmqPubAddress( ip_and_port );
}

void QCstmConfigMonitoring::IpZmqSubAddressEditFinished()
{
    QString ip_and_port = ui->IP_ZMQ_SUB_lineEdit->text();
    _mpx3gui->getConfig()->setIpZmqSubAddress( ip_and_port );
}

void QCstmConfigMonitoring::pixelDepthChangedByValue(int val)
{
    setMaximumFPSFromPixelDepth(-1, val);

    int sizea = int(__pixelDepthMap.size());
    for ( int i = 0; i < sizea; i++ ) {
        if ( __pixelDepthMap[i] == val ) {
            ui->pixelDepthCombo->setCurrentIndex( i );
            return;
        }
    }
    ui->pixelDepthCombo->setCurrentIndex( 0 );
}

void QCstmConfigMonitoring::triggerModeChangedByValue(int val) {
    int i = findTriggerModeIndex(val);
    ui->triggerModeCombo->setCurrentIndex(i >= 0 ? i : 0);
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

    readMonitoringInfo();

}

void QCstmConfigMonitoring::setWindowWidgetsStatus(win_status s){
    switch (s) {

    case win_status::startup:
        this->setEnabled( false );
        break;

    case win_status::connected:
        this->setEnabled( true );
        break;

    default:
        break;

    }
}

void QCstmConfigMonitoring::readMonitoringInfo() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    if ( spidrcontrol ) {

        int mdegrees, clockMHz;
        if( spidrcontrol->getRemoteTemp( &mdegrees ) ) {
            QString qs = QString("%1.%2 °C").arg( mdegrees/1000 ).arg( mdegrees%1000, 3, 10, QChar('0') );
            ui->remoteTempMeasLineEdit->setText( qs );
        } else {
            ui->remoteTempMeasLineEdit->setText( "--.--" );
        }

        if( spidrcontrol->getLocalTemp( &mdegrees ) ) {
            QString qs = QString("%1.%2 °C").arg( mdegrees/1000 ).arg( mdegrees%1000, 3, 10, QChar('0') );
            ui->localTempMeasLineEdit->setText( qs );
        } else {
            ui->localTempMeasLineEdit->setText( "--.--" );
        }
        if( spidrcontrol->getFpgaTemp( &mdegrees ) ) {
            QString qs = QString("%1.%2 °C").arg( mdegrees/1000 ).arg( mdegrees%1000, 3, 10, QChar('0') );
            ui->FpgaTempMeasLineEdit->setText( qs );
        } else {
            ui->FpgaTempMeasLineEdit->setText( "--.--" );
        }

        if ( spidrcontrol->getMpx3Clock( &clockMHz ) ) {
            QString qs = QString("%1 MHz").arg( clockMHz );
            ui->systemClockLineEdit->setText( qs );
        } else {
            ui->systemClockLineEdit->setText( "---" );
        }

        int biasVolts;
        if ( spidrcontrol->getBiasVoltage(&biasVolts) ) {
            QString qs;
            if (biasVolts < 12) {
                qs = QString("Ext. (%1)").arg( biasVolts );
            } else {
                qs = QString("%1 V").arg( biasVolts );
            }
            ui->biasVoltageMeasLineEdit->setText( qs );
        } else {
            ui->biasVoltageMeasLineEdit->setText( "--.-" );
        }

        int humidityPercentage = -1;
        if ( spidrcontrol->getHumidity(&humidityPercentage)) {
            QString qs = "---";
            if (humidityPercentage >=0 && humidityPercentage <= 100) {
                qs = QString("%1%").arg(humidityPercentage);
            }
            ui->humidityMeasureLineEdit->setText(qs);
        }

        int pressure_mbar = -1;
        if ( spidrcontrol->getPressure(&pressure_mbar)) {
            QString qs = "---";

            //! TODO Add an upper acceptable limit?

            if (pressure_mbar >=0) {
                qs = QString("%1 mbar").arg(pressure_mbar);
            }
            ui->pressureMeasureLineEdit->setText(qs);
        }

        int mvolt, mamp, mwatt;
        if( spidrcontrol->getAvddNow( &mvolt, &mamp, &mwatt ) ) {
            ui->avddmvolt->setText( QString::number( mvolt ) );
            ui->avddmwatt->setText( QString::number( mwatt ) );
            QString qs = QString("%1.%2").arg( mamp/10 ).arg( mamp%10 );
            ui->avddmamp->setText( qs );
        } else {
            ui->avddmvolt->setText( "----" ) ;
            ui->avddmwatt->setText( "----" );
            ui->avddmamp->setText( "----" );
        }

        if( spidrcontrol->getVddNow( &mvolt, &mamp, &mwatt ) ) {
            ui->vddmvolt->setText( QString::number( mvolt ) );
            ui->vddmwatt->setText( QString::number( mwatt ) );
            QString qs = QString("%1.%2").arg( mamp/10 ).arg( mamp%10 );
            ui->vddmamp->setText( qs );
        } else {
            ui->vddmvolt->setText( "----" );
            ui->vddmwatt->setText( "----" );
            ui->vddmamp->setText( "----" );
        }

        if( spidrcontrol->getDvddNow( &mvolt, &mamp, &mwatt ) ) {
            ui->dvddmvolt->setText( QString::number( mvolt ) );
            ui->dvddmwatt->setText( QString::number( mwatt ) );
            QString qs = QString("%1.%2").arg( mamp/10 ).arg( mamp%10 );
            ui->dvddmamp->setText( qs );
        } else {
            ui->dvddmvolt->setText( "----" );
            ui->dvddmwatt->setText( "----" );
            ui->dvddmamp->setText( "----" );
        }
    }

}

void QCstmConfigMonitoring::on_SaveButton_clicked()
{
    if(!_saveConfigFileRemotely)
    {

        QFileDialog saveDialog(this, tr("Save configuration"), tr("./config"), tr("Json files (*.json)"));
        saveDialog.setAcceptMode(QFileDialog::AcceptSave);
        saveDialog.setDefaultSuffix("json");
        //saveDialog.setDirectory("./config");
        saveDialog.exec();
        if( saveDialog.selectedFiles().empty() ) return; // the user hit Cancel
        _conigFileDestination = saveDialog.selectedFiles().first();
    }
    _saveConfigFileRemotely = false; //reset the flag
    //QFileDialog dialog;
    //dialog.setDefaultSuffix("json");//Bugged under Linux?
    //QString filename = dialog.getSaveFileName(this, tr("Save configuration"), tr("./config"), tr("Json files (*.json)"));
    _mpx3gui->getConfig()->toJsonFile(_conigFileDestination, ui->IncludeDacsCheck->isChecked());
}

void QCstmConfigMonitoring::saveConfigFileRemotely(QString path){
    _saveConfigFileRemotely = true;
    _conigFileDestination = path;
    on_SaveButton_clicked();
}

void QCstmConfigMonitoring::on_LoadButton_clicked() {
    QString filename = QFileDialog::getOpenFileName(this, tr("Open configuration"), tr("./config"), tr("Json files (*.json)"));
    _mpx3gui->getConfig()->fromJsonFile(filename, ui->IncludeDacsCheck->isChecked());
}

void QCstmConfigMonitoring::on_ColourModeCheckBox_toggled(bool checked) {
    bool conn = _mpx3gui->getConfig()->isConnected();
    if (conn) {
        _mpx3gui->clear_data();
    }
    if(checked)
        _mpx3gui->resize(_mpx3gui->getDataset()->x()/2, _mpx3gui->getDataset()->y()/2);
    else
        _mpx3gui->resize(_mpx3gui->getDataset()->x()*2, _mpx3gui->getDataset()->y()*2);
}

void QCstmConfigMonitoring::on_merlinInterfaceTestButton_clicked()
{
    MerlinInterface mi;
    QString ba = ui->merlinInterfaceTestButton->text();
    MerlinCommand mc(ba, mi);
    QString res = mc.parseResult;
    qDebug() << res;
    qDebug() << "Response : " << mc.makeSetCmdResponse();
    QString pad ;
    for (int var = 0; var < 99; ++var) {
        pad.append(' ');
    }
    pad.append('e');
    qDebug()<<" Pad len = " << pad.length();
    qDebug()<<" Pad len byte= " << pad.toLatin1().length();
    qDebug()<<" Pad data = " << pad;
}

void QCstmConfigMonitoring::setContRWFreqFromConfig(int val)
{
    if ( _mpx3gui->getConfig()->getOperationMode() == Mpx3Config::__operationMode_ContinuousRW ) {
        _mpx3gui->getVisualization()->GetUI()->triggerLengthSpinBox->setValue(val);

        setMaximumFPSFromPixelDepth();
    } /* else {
        Do nothing because we don't want to set the trigger length to the continuous readout frequency...
    } */
}

void QCstmConfigMonitoring::setMaximumFPSinVisualiation()
{
    _mpx3gui->getVisualization()->setMaximumContRW_FPS(-1);
}
