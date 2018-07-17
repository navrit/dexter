/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#include <QFont>
#include <QIntValidator>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>
#include <QPen>
#include <QSignalMapper>

#include "qcstmdacs.h"
#include "ui_qcstmdacs.h"
#include "mpx3eq_common.h"
#include "mpx3dacsdescr.h"
#include "mpx3gui.h"
#include "qcstmequalization.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "qcustomplot.h"
#include "ui_thresholdscan.h"

QCstmDacs *qCstmDacsInst = nullptr;

QCstmDacs::QCstmDacs(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QCstmDacs)
{
    ui->setupUi(this);

    //_ui = ui;
    _senseThread = nullptr;
    _scanThread = nullptr;
    _updateDACsThread = nullptr;
    _signalMapperSliderSpinBoxConn = nullptr;
    _signalMapperSlider = nullptr;
    _signalMapperSpinBox = nullptr;

    // Number of plots added to the Scan
    _plotIdxCntr = 0;

    // Defaults
    _scanStep = 16;
    ui->scanStepSpinBox->setMaximum( 256 );
    ui->scanStepSpinBox->setMinimum( 1 );
    ui->scanStepSpinBox->setValue( _scanStep );
    _deviceIndex = 2;
    ui->deviceIdSpinBox->setMaximum(3);
    ui->deviceIdSpinBox->setMinimum(0);
    ui->deviceIdSpinBox->setValue( _deviceIndex );
    _nSamples = 1;
    ui->samplesSpinBox->setMaximum(100);
    ui->samplesSpinBox->setMinimum(0);
    ui->samplesSpinBox->setValue( _nSamples );

    ui->progressBar->setValue( 0 );

    _dacsSimultaneous = true;
    ui->allDACSimultaneousCheckBox->setChecked( true );
    ui->allDACSimultaneousCheckBox->setToolTip("Apply changes to all chips simultaneously");

    // Order widgets in vectors
    FillWidgetVectors();

    // Set limits in widgets
    SetLimits();

    // Setup connection between Sliders and SpinBoxes
    SetupSignalsAndSlots();

    // Leave a few things unnactivated
    ui->startScanButton->setDisabled( true );
    ui->senseDACsPushButton->setDisabled( true );



    // Prepare plot
    // Prepare the plot
    //ui->plotScan = new QCustomPlot();
    ui->plotScan->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );
    // The legend
    ui->plotScan->legend->setVisible( false ); // Nothing there yet...
    QFont f = ui->plotScan->font();
    f.setPointSize( 7 ); // and make a bit smaller for legend
    ui->plotScan->legend->setFont( f );
    ui->plotScan->legend->setBrush( QBrush(QColor(255,255,255,230)) );
    // The axes
    ui->plotScan->xAxis->setRange( 0, __max_DAC_range ); // Maximum DAC range
    ui->plotScan->yAxis->setRange( 0, __voltage_DACS_MAX );
    f = ui->plotScan->font();  // Start out with Dialog's font..
    f.setBold( true );
    ui->plotScan->xAxis->setLabelFont( f );
    ui->plotScan->yAxis->setLabelFont( f );
    // The labels:
    ui->plotScan->xAxis->setLabel("DAC setting");
    ui->plotScan->yAxis->setLabel("DAC out [V]");
    qCstmDacsInst = this;
}

QCstmDacs::~QCstmDacs() {
    delete ui;
}

void QCstmDacs::on_allDACSimultaneousCheckBox_toggled(bool checked) {

    _dacsSimultaneous = checked;

    if ( _dacsSimultaneous ) ui->deviceIdSpinBox->setDisabled( true );
    else  ui->deviceIdSpinBox->setEnabled( true );

}

int QCstmDacs::GetDACValueFromConfig(int chip, int dacIndex) {
    return _mpx3gui->getConfig()->getDACValue(chip, dacIndex);
}

void QCstmDacs::SetDACValueLocalConfig(int chip, int dacIndex, int val) {
    _mpx3gui->getConfig()->setDACValue(chip, dacIndex, val);
}

void QCstmDacs::StartDACScan() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    if ( ! spidrcontrol ) {
        QMessageBox::information(this, tr("MPX3"), tr("Connect to hardware first.") );
        return;
    }

    // Replot to start
    ui->plotScan->clearGraphs();
    ui->plotScan->legend->setVisible( false );
    ui->plotScan->replot();

    // Threads
    if ( _scanThread ) {
        if ( _scanThread->isRunning() ) {
            return;
        }
        //disconnect(_senseThread, SIGNAL( progress(int) ), ui->progressBar, SLOT( setValue(int)) );
        delete _scanThread;
        _scanThread = 0x0;
    }

    // Create the thread
    _scanThread = new ScanDACsThread(_deviceIndex, this, spidrcontrol);
    // Connect to the progress bar
    connect( _scanThread, SIGNAL( progress(int) ), ui->progressBar, SLOT( setValue(int)) );

    _scanThread->start();

}

void QCstmDacs::changeDAC(int threshold, int value)
{
    slideAndSpin(threshold, value);
}

UpdateDACsThread * QCstmDacs::FillDACValues( int devId, bool updateInTheChip ) {


    // Switch to this dev id
    //ChangeDeviceIndex( devId );

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    // Threads
    if ( _updateDACsThread ) {
        if ( _updateDACsThread->isRunning() ) {
            return 0;
        }
        //disconnect(_senseThread, SIGNAL( progress(int) ), ui->progressBar, SLOT( setValue(int)) );
        delete _updateDACsThread;
        _updateDACsThread = 0x0;
    }

    // Create the thread
    _updateDACsThread = new UpdateDACsThread(devId, _mpx3gui->getConfig()->getDacCount(), this, spidrcontrol);
    _updateDACsThread->SetUpdateInTheChip( updateInTheChip );

    // Run !
    _updateDACsThread->start();

    // return it to run it later
    return _updateDACsThread;
}

void QCstmDacs::PopulateDACValues() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    // Here we set the default values hardcoded in MPX3RX_DAC_TABLE (mid range)
    //   OR if the DACs file is present we read the values from it.  The file has
    //   higher priority.
    string defaultDACsFn = __default_DACs_filename;

    qDebug() << "[INFO] setting dacs from default DACs file.";

    int lastIndexToSet = 0;
    int nChipsToSet = 0;
    if ( GetDACsFromConfiguration() ) {
        QVector<QPoint> devices = _mpx3gui->getConfig()->getDevicePresenceLayout();
        for(int i = 0; i < devices.length();i++) {
            // The DAC setting threads
            if(devices[i] != QPoint(0,0)) {
                nChipsToSet++;
                lastIndexToSet = i;
            } else {
            }
        }
    } else { // Setting DACs at mid-range

        for (int i = 0 ; i < 1; i++) {

            spidrcontrol->setDac( _deviceIndex, MPX3RX_DAC_TABLE[i].code, MPX3RX_DAC_TABLE[i].dflt );
            _dacSpinBoxes[i]->setValue( MPX3RX_DAC_TABLE[i].dflt );
            _dacSliders[i]->setValue( MPX3RX_DAC_TABLE[i].dflt );

        }

    }

    // do multiple or only one
    if(nChipsToSet > 1) FillDACValues(-1);
    else FillDACValues(lastIndexToSet);

}

void QCstmDacs::ConnectionStatusChanged(bool conn) {

    // Widgets status
    if ( conn ) {
        setWindowWidgetsStatus( win_status::connected );
    } else {
        setWindowWidgetsStatus( win_status::disconnected );
    }

    // Fill the DACs
    if ( conn ) PopulateDACValues();

}

void QCstmDacs::SenseDACs() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    if ( !spidrcontrol ) {
        QMessageBox::information(this, tr("MPX3"), tr("Connect to hardware first.") );
        return;
    }

    // Threads
    if ( _senseThread ) {
        if ( _senseThread->isRunning() ) {
            return;
        }
        //disconnect(_senseThread, SIGNAL( progress(int) ), ui->progressBar, SLOT( setValue(int)) );
        delete _senseThread;
        _senseThread = 0x0;
    }

    // Create the thread
    _senseThread = new SenseDACsThread(_deviceIndex, this, spidrcontrol);
    // Connect to the progress bar
    connect( _senseThread, SIGNAL( progress(int) ), ui->progressBar, SLOT( setValue(int)) );

    _senseThread->start();

}


void QCstmDacs::FillWidgetVectors() {

    // Spin boxes for the DAC values
    _dacSpinBoxes[0] = ui->dac0SpinBox;
    _dacSpinBoxes[1] = ui->dac1SpinBox;
    _dacSpinBoxes[2] = ui->dac2SpinBox;
    _dacSpinBoxes[3] = ui->dac3SpinBox;
    _dacSpinBoxes[4] = ui->dac4SpinBox;
    _dacSpinBoxes[5] = ui->dac5SpinBox;
    _dacSpinBoxes[6] = ui->dac6SpinBox;
    _dacSpinBoxes[7] = ui->dac7SpinBox;
    _dacSpinBoxes[8] = ui->dac8SpinBox;
    _dacSpinBoxes[9] = ui->dac9SpinBox;
    _dacSpinBoxes[10] = ui->dac10SpinBox;
    _dacSpinBoxes[11] = ui->dac11SpinBox;
    _dacSpinBoxes[12] = ui->dac12SpinBox;
    _dacSpinBoxes[13] = ui->dac13SpinBox;
    _dacSpinBoxes[14] = ui->dac14SpinBox;
    _dacSpinBoxes[15] = ui->dac15SpinBox;
    _dacSpinBoxes[16] = ui->dac16SpinBox;
    _dacSpinBoxes[17] = ui->dac17SpinBox;
    _dacSpinBoxes[18] = ui->dac18SpinBox;
    _dacSpinBoxes[19] = ui->dac19SpinBox;
    _dacSpinBoxes[20] = ui->dac20SpinBox;
    _dacSpinBoxes[21] = ui->dac21SpinBox;
    _dacSpinBoxes[22] = ui->dac22SpinBox;
    _dacSpinBoxes[23] = ui->dac23SpinBox;
    _dacSpinBoxes[24] = ui->dac24SpinBox;
    _dacSpinBoxes[25] = ui->dac25SpinBox;
    _dacSpinBoxes[26] = ui->dac26SpinBox;

    // Sliders
    _dacSliders[0] = ui->dac0hSlider;
    _dacSliders[1] = ui->dac1hSlider;
    _dacSliders[2] = ui->dac2hSlider;
    _dacSliders[3] = ui->dac3hSlider;
    _dacSliders[4] = ui->dac4hSlider;
    _dacSliders[5] = ui->dac5hSlider;
    _dacSliders[6] = ui->dac6hSlider;
    _dacSliders[7] = ui->dac7hSlider;
    _dacSliders[8] = ui->dac8hSlider;
    _dacSliders[9] = ui->dac9hSlider;
    _dacSliders[10] = ui->dac10hSlider;
    _dacSliders[11] = ui->dac11hSlider;
    _dacSliders[12] = ui->dac12hSlider;
    _dacSliders[13] = ui->dac13hSlider;
    _dacSliders[14] = ui->dac14hSlider;
    _dacSliders[15] = ui->dac15hSlider;
    _dacSliders[16] = ui->dac16hSlider;
    _dacSliders[17] = ui->dac17hSlider;
    _dacSliders[18] = ui->dac18hSlider;
    _dacSliders[19] = ui->dac19hSlider;
    _dacSliders[20] = ui->dac20hSlider;
    _dacSliders[21] = ui->dac21hSlider;
    _dacSliders[22] = ui->dac22hSlider;
    _dacSliders[23] = ui->dac23hSlider;
    _dacSliders[24] = ui->dac24hSlider;
    _dacSliders[25] = ui->dac25hSlider;
    _dacSliders[26] = ui->dac26hSlider;

    // DAC labels
    _dacLabels[0] = ui->dac0Label;
    _dacLabels[1] = ui->dac1Label;
    _dacLabels[2] = ui->dac2Label;
    _dacLabels[3] = ui->dac3Label;
    _dacLabels[4] = ui->dac4Label;
    _dacLabels[5] = ui->dac5Label;
    _dacLabels[6] = ui->dac6Label;
    _dacLabels[7] = ui->dac7Label;
    _dacLabels[8] = ui->dac8Label;
    _dacLabels[9] = ui->dac9Label;
    _dacLabels[10] = ui->dac10Label;
    _dacLabels[11] = ui->dac11Label;
    _dacLabels[12] = ui->dac12Label;
    _dacLabels[13] = ui->dac13Label;
    _dacLabels[14] = ui->dac14Label;
    _dacLabels[15] = ui->dac15Label;
    _dacLabels[16] = ui->dac16Label;
    _dacLabels[17] = ui->dac17Label;
    _dacLabels[18] = ui->dac18Label;
    _dacLabels[19] = ui->dac19Label;
    _dacLabels[20] = ui->dac20Label;
    _dacLabels[21] = ui->dac21Label;
    _dacLabels[22] = ui->dac22Label;
    _dacLabels[23] = ui->dac23Label;
    _dacLabels[24] = ui->dac24Label;
    _dacLabels[25] = ui->dac25Label;
    _dacLabels[26] = ui->dac26Label;

    // DAC sense labels. Where voltage is shown.
    _dacVLabels[0] = ui->dac0VLabel;
    _dacVLabels[1] = ui->dac1VLabel;
    _dacVLabels[2] = ui->dac2VLabel;
    _dacVLabels[3] = ui->dac3VLabel;
    _dacVLabels[4] = ui->dac4VLabel;
    _dacVLabels[5] = ui->dac5VLabel;
    _dacVLabels[6] = ui->dac6VLabel;
    _dacVLabels[7] = ui->dac7VLabel;
    _dacVLabels[8] = ui->dac8VLabel;
    _dacVLabels[9] = ui->dac9VLabel;
    _dacVLabels[10] = ui->dac10VLabel;
    _dacVLabels[11] = ui->dac11VLabel;
    _dacVLabels[12] = ui->dac12VLabel;
    _dacVLabels[13] = ui->dac13VLabel;
    _dacVLabels[14] = ui->dac14VLabel;
    _dacVLabels[15] = ui->dac15VLabel;
    _dacVLabels[16] = ui->dac16VLabel;
    _dacVLabels[17] = ui->dac17VLabel;
    _dacVLabels[18] = ui->dac18VLabel;
    _dacVLabels[19] = ui->dac19VLabel;
    _dacVLabels[20] = ui->dac20VLabel;
    _dacVLabels[21] = ui->dac21VLabel;
    _dacVLabels[22] = ui->dac22VLabel;
    _dacVLabels[23] = ui->dac23VLabel;
    _dacVLabels[24] = ui->dac24VLabel;
    _dacVLabels[25] = ui->dac25VLabel;
    _dacVLabels[26] = ui->dac26VLabel;

    // Check boxes
    _dacCheckBoxes[0] = ui->dac0CheckBox;
    _dacCheckBoxes[1] = ui->dac1CheckBox;
    _dacCheckBoxes[2] = ui->dac2CheckBox;
    _dacCheckBoxes[3] = ui->dac3CheckBox;
    _dacCheckBoxes[4] = ui->dac4CheckBox;
    _dacCheckBoxes[5] = ui->dac5CheckBox;
    _dacCheckBoxes[6] = ui->dac6CheckBox;
    _dacCheckBoxes[7] = ui->dac7CheckBox;
    _dacCheckBoxes[8] = ui->dac8CheckBox;
    _dacCheckBoxes[9] = ui->dac9CheckBox;
    _dacCheckBoxes[10] = ui->dac10CheckBox;
    _dacCheckBoxes[11] = ui->dac11CheckBox;
    _dacCheckBoxes[12] = ui->dac12CheckBox;
    _dacCheckBoxes[13] = ui->dac13CheckBox;
    _dacCheckBoxes[14] = ui->dac14CheckBox;
    _dacCheckBoxes[15] = ui->dac15CheckBox;
    _dacCheckBoxes[16] = ui->dac16CheckBox;
    _dacCheckBoxes[17] = ui->dac17CheckBox;
    _dacCheckBoxes[18] = ui->dac18CheckBox;
    _dacCheckBoxes[19] = ui->dac19CheckBox;
    _dacCheckBoxes[20] = ui->dac20CheckBox;
    _dacCheckBoxes[21] = ui->dac21CheckBox;
    _dacCheckBoxes[22] = ui->dac22CheckBox;
    _dacCheckBoxes[23] = ui->dac23CheckBox;
    _dacCheckBoxes[24] = ui->dac24CheckBox;
    _dacCheckBoxes[25] = ui->dac25CheckBox;
    _dacCheckBoxes[26] = ui->dac26CheckBox;

    // Tooltips
    for (int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {

        QString tooltip = "Integer value between 0 and ";
        tooltip += QString::number( (MPX3RX_DAC_TABLE[i].dflt * 2) - 1 );
        _dacSpinBoxes[i]->setToolTip( tooltip );
        _dacSliders[i]->setToolTip( tooltip );

        //
        QString tooltipLabel = "[";
        tooltipLabel += MPX3RX_DAC_TABLE[i].name;
        tooltipLabel += "] index:";
        tooltipLabel += QString::number( i );
        tooltipLabel += ",code:";
        tooltipLabel += QString::number( MPX3RX_DAC_TABLE[i].code );
        _dacLabels[i]->setToolTip( tooltipLabel );
        _dacCheckBoxes[i]->setToolTip( tooltipLabel );

        QString tooltipLabelV = "16bits AD conversion (V)";
        _dacVLabels[i]->setToolTip( tooltipLabelV );

    }

    // Text in the DAC labels
    for (int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {
        QString DACname = MPX3RX_DAC_TABLE[i].name;
        _dacLabels[i]->setText( DACname );
    }

}

void QCstmDacs::SetLimits() {

    for (int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {

        // Min and max for spin boxes
        _dacSpinBoxes[i]->setMinimum( 0 );
        _dacSpinBoxes[i]->setMaximum( (MPX3RX_DAC_TABLE[i].dflt * 2) - 1 );
        // Min and max for sliders
        _dacSliders[i]->setMinimum( 0 );
        _dacSliders[i]->setMaximum( (MPX3RX_DAC_TABLE[i].dflt * 2) - 1 );
        // Check all for scan
        _dacCheckBoxes[i]->setChecked(true);

    }

    // Leave by default the THL2 o THL7 uncheked by default
    for (int i = 2 ; i <= 7 ; i++ ) {
        _dacCheckBoxes[i]->setChecked( false );
    }

}

void QCstmDacs::shortcutTH0()
{
    qDebug() << "[INFO] Focus on DAC 0: TH0";
    ui->dac0SpinBox->setFocus();
}

void QCstmDacs::shortcutIkrum()
{
    qDebug() << "[INFO] Focus on DAC 9: Ikrum";
    ui->dac9SpinBox->setFocus();
}

void QCstmDacs::UncheckAllDACs() {

    for (int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {
        // Deselect all
        _dacCheckBoxes[i]->setChecked( false );
    }

}

void QCstmDacs::CheckAllDACs() {

    for (int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {
        // Check all for scan
        _dacCheckBoxes[i]->setChecked(true);
    }

}

void QCstmDacs::SetupSignalsAndSlots() {

    // Buttons
    connect( ui->clearAllPushButton, SIGNAL(clicked()), this, SLOT( UncheckAllDACs() ) );
    connect( ui->selectAllPushButton, SIGNAL(clicked()), this, SLOT( CheckAllDACs() ) );
    connect( ui->startScanButton, SIGNAL(clicked()), this, SLOT( StartDACScan() ) );
    connect( ui->senseDACsPushButton, SIGNAL(clicked()), this, SLOT( SenseDACs() ) );
    connect( ui->deviceIdSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeDeviceIndex(int) ) );
    connect( ui->samplesSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeNSamples(int) ) );
    connect( ui->scanStepSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeScanStep(int) ) );

    // Sliders and SpinBoxes

    // I need the SignalMapper in order to handle custom slots with parameters
    if(!_signalMapperSliderSpinBoxConn) _signalMapperSliderSpinBoxConn = new QSignalMapper (this);
    if(!_signalMapperSlider) _signalMapperSlider = new QSignalMapper (this);
    if(!_signalMapperSpinBox) _signalMapperSpinBox = new QSignalMapper (this);

    for ( int i = 0 ; i < MPX3RX_DAC_COUNT; i++ ) {

        // Connect slides to Spin Boxes back and forth
        // When the sliders move the SpinBoxes actualizes
        //QObject::connect( _dacSliders[i], SIGNAL(sliderMoved(int)),
        //_dacSpinBoxes[i], SLOT(setValueDAC(int)) );

        QObject::connect( _dacSliders[i], SIGNAL(sliderMoved(int)),
                          _signalMapperSliderSpinBoxConn, SLOT(map()) );

        // When the slider released, talk to the hardware
        QObject::connect( _dacSliders[i], SIGNAL(sliderReleased()),
                          _signalMapperSlider, SLOT(map()) );

        // When a value is changed in the SpinBox the slider needs to move
        //  and talk to the hardware
        QObject::connect( _dacSpinBoxes[i], SIGNAL(valueChanged(int)), // SIGNAL(valueChanged(int)), // SIGNAL(editingFinished()),
                          _signalMapperSpinBox, SLOT(map()) );

        // map the index
        _signalMapperSliderSpinBoxConn->setMapping( _dacSliders[i], i ); // SLOT(setValueDAC(int))

        // map the index
        _signalMapperSlider->setMapping( _dacSliders[i], i );

        // map the index
        _signalMapperSpinBox->setMapping( _dacSpinBoxes[i], i );

    }

    QObject::connect( _signalMapperSliderSpinBoxConn,  SIGNAL(mapped(int)), this, SLOT(setValueDAC(int)) );

    QObject::connect( _signalMapperSlider, SIGNAL(mapped(int)), this, SLOT( FromSliderUpdateSpinBox(int) ) );

    QObject::connect( _signalMapperSpinBox, SIGNAL(mapped(int)), this, SLOT( FromSpinBoxUpdateSlider(int) ) ); // SLOT( UpdateSliders(int) ) );

}

/**
 * While the slider moves I want to see the spinBox running but
 *  only want to talk to the hardware when it is released.
 */
void QCstmDacs::setValueDAC(int i) {

    // Temporarily disconnect the signal that triggers the message to the hardware
    QObject::disconnect( _signalMapperSpinBox, SIGNAL(mapped(int)), this, SLOT( FromSpinBoxUpdateSlider(int) ) );

    // Set the value in the spinBox so the user sees it running
    int val = _dacSliders[ i ]->value();
    // Change the value in the spinBox
    _dacSpinBoxes[i]->setValue( val );

    // Connect it back
    QObject::connect( _signalMapperSpinBox, SIGNAL(mapped(int)), this, SLOT( FromSpinBoxUpdateSlider(int) ) );

}

void QCstmDacs::FromSpinBoxUpdateSlider(int i) {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    // Set the value
    int val = _dacSpinBoxes[i]->value();
    // Set the slider according to the new value in the Spin Box
    _dacSliders[i]->setValue( val );


    //////////////////////////////////////
    // Set DAC
    // If the setting is global apply it
    if ( _dacsSimultaneous ) {
        for( int chip = 0 ; chip < _mpx3gui->getConfig()->getNDevicesSupported() ; chip++) {
            // Check if the device is alive
            if ( ! _mpx3gui->getConfig()->detectorResponds( chip ) ) {
                qDebug() << "[ERR ] Device " << chip << " not responding.";
                continue;
            }
            spidrcontrol->setDac( chip, MPX3RX_DAC_TABLE[ i ].code, val );
            // Now I need to chage it in the local data base
            SetDACValueLocalConfig( chip, i, val);
        }
    } else {
        spidrcontrol->setDac( _deviceIndex, MPX3RX_DAC_TABLE[ i ].code, val );
        // Now I need to chage it in the local data base
        SetDACValueLocalConfig( _deviceIndex, i, val);
    }

    // Clean up the corresponding labelV.  The dacOut won't be read right away.
    // Only under user request
    //++i
    //_dacVals[i] = val;
    GetLabelsList()[i]->setText("");

}

void QCstmDacs::FromSliderUpdateSpinBox(int i) {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    //
    int val = _dacSliders[ i ]->value();
    // Set the spin box according to the new value in the Slider
    //_dacSpinBoxes[i]->setValue( val );

    //////////////////////////////////////
    // Set DAC
    // If the setting is global apply it
    if ( _dacsSimultaneous ) {
        for( int chip = 0 ; chip < _mpx3gui->getConfig()->getNDevicesSupported() ; chip++) {
            // Check if the device is alive
            if ( ! _mpx3gui->getConfig()->detectorResponds( chip ) ) {
                qDebug() << "[ERR ] Device " << chip << " not responding.";
                continue;
            }
            spidrcontrol->setDac( chip, MPX3RX_DAC_TABLE[ i ].code, val );
            // Now I need to chage it in the local data base
            SetDACValueLocalConfig( chip, i, val);
        }
    } else {
        spidrcontrol->setDac( _deviceIndex, MPX3RX_DAC_TABLE[ i ].code, val );
        // Now I need to chage it in the local data base
        SetDACValueLocalConfig( _deviceIndex, i, val);
    }

    // Set DAC
    //spidrcontrol->setDac( _deviceIndex, MPX3RX_DAC_TABLE[ i ].code, val );


    //++i
    //_dacVals[i] = val;
    // Clean up the corresponding labelV.  The dacOut won't be read right away.
    // Only under user request
    GetLabelsList()[i]->setText("");

}

//void QCstmDacs::SetDAC(QObject * info) {
//	// Verify first if the value changed at all
//	int val = _dacSpinBoxes[ ((SignalSlotMapping *)info)->index ]->value();
//	_spidrcontrol->setDac( _deviceIndex, MPX3RX_DAC_TABLE[ ((SignalSlotMapping *)info)->index ].code, val );
//}


bool QCstmDacs::GetDACsFromConfiguration() {

    // See if there DAC information to work with
    if ( _mpx3gui->getConfig()->getDacCount() != 0 ) {
        return true;
    }

    return false;
}

bool QCstmDacs::WriteDACsFile(string fn){
    if(fn.empty()) { // default
        fn = __default_DACs_filename;
    }
    QFile saveFile(QString(fn.c_str()));
    if (!saveFile.open(QIODevice::WriteOnly)) {
        string messg = "Couldn't open: ";
        messg += fn;
        messg += "\nNo output written!";
        QMessageBox::warning ( this, tr("MPX3 - default DACs"), tr( messg.c_str() ) );
        return false;
    }
    QFileInfo fInfo(saveFile.fileName());
    qDebug() << "[INFO] Opened " <<fInfo.absoluteFilePath().toStdString().c_str();

    return true;
}

QCstmDacs *QCstmDacs::getInstance()
{
    return qCstmDacsInst;
}

void QCstmDacs::ChangeDeviceIndex( int index )
{
    if( index < 0 ) return; // can't really happen cause the SpinBox has been limited
    _deviceIndex = index;

    // Now change the entire view.  Actualize all values
    // If the detector is busy with something else set the
    //   actualization flag to false so the GUI refreshes
    //   but the values are not sent to the chip
    bool busy = _mpx3gui->getEqualization()->isBusy();
    // busy |= visualization->isBusy();

    // if busy, don't update in the chip
    FillDACValues( index, !busy );

}

void QCstmDacs::ChangeNSamples( int index )
{
    if( index < 0 ) return; // can't really happen cause the SpinBox has been limited
    _nSamples = index;
}

void QCstmDacs::ChangeScanStep( int index )
{
    if( index < 0 ) return; // can't really happen cause the SpinBox has been limited
    _scanStep = index;
}

QCPGraph * QCstmDacs::GetGraph(int idx) {
    return ui->plotScan->graph( idx );
}


void QCstmDacs::setWindowWidgetsStatus(win_status s)
{

    switch (s) {

    case win_status::startup:

        this->setEnabled( false );
        ui->startScanButton->setDisabled( true );
        ui->senseDACsPushButton->setDisabled( true );

        break;

    case win_status::connected:
        this->setEnabled( true );
        ui->startScanButton->setDisabled( false );
        ui->senseDACsPushButton->setDisabled( false );

        break;

    default:

        break;

    }

}


void QCstmDacs::scanFinished() {

    _plotIdxCntr = 0;
    _plotIdxMap.clear();

    // clear plots
    //ui->plotScan->clearGraphs();
    //ui->plotScan->legend->setVisible( false );

}

void QCstmDacs::updateFinished() {

    // What needs to be done when an update has been requested ?

}


void QCstmDacs::addData(int dacIdx, int dacVal, double adcVal ) {

    _graph = ui->plotScan->graph( _plotIdxMap[dacIdx] ); // the right vector index
    _graph->addData( dacVal, adcVal );
    ui->plotScan->replot();

    // Actualize Slider, SpinBoxes, and Labels
    //_dacSpinBoxes[dacIdx]->setValue( dacVal );
    //_dacSliders[dacIdx]->setValue( dacVal );
    // Labels are updated already through a SIGNAL/SLOT from the thread.

}

void QCstmDacs::setTextWithIdx(QString s, int i) {

    // Get the color from the Color table
    // Create the style sheet string
    QPalette palette = GetLabelsList()[i]->palette();
    //palette.setColor(GetLabelsList()[i]->backgroundRole(), COLOR_TABLE[i]);
    palette.setColor(GetLabelsList()[i]->foregroundRole(), COLOR_TABLE[i]);

    GetLabelsList()[i]->setPalette(palette);

    // The StyleSheet is the crossplatform solution but I am not using it here
    //GetLabelsList()[i]->setStyleSheet("QLabel { background-color : Qt::red; color : Qt::black; }");

    // Finally set the text
    GetLabelsList()[i]->setText(s);

}

/**
 * In case I have the dac code and I want to know to which slider/spinBox it corresponds
 */
int QCstmDacs::GetDACIndex(int dac_code) {

    for(int i = 0 ; i < MPX3RX_DAC_COUNT ; i++) {
        if ( MPX3RX_DAC_TABLE[i].code == dac_code ) return i;
    }
    return -1;
}

void QCstmDacs::slideAndSpin(int i, int val) {

    // // Temporarily disconnect the signal that triggers the message to the hardware
    QObject::disconnect( _signalMapperSpinBox, SIGNAL(mapped(int)), this, SLOT( FromSpinBoxUpdateSlider(int) ) );

    // Slide n' Spin
    GetSpinBoxList()[i]->setValue( val );
    GetSliderList()[i]->setValue( val );

    // Also change the value in the local database


    // Connect it back
    QObject::connect( _signalMapperSpinBox, SIGNAL(mapped(int)), this, SLOT( FromSpinBoxUpdateSlider(int) ) );

}

void QCstmDacs::addData(int dacIdx) {

    _plotIdxMap[dacIdx] = _plotIdxCntr;
    _plotIdxCntr++;

    // If starting a plot, create it first
    ui->plotScan->addGraph();
    _graph = ui->plotScan->graph( _plotIdxMap[dacIdx] ); // the right vector index

    QPen pen( COLOR_TABLE[ dacIdx ] );
    pen.setWidth( 0.1 );
    _graph->setPen( pen );
    _graph->setName( QString(MPX3RX_DAC_TABLE[dacIdx].name) ); // the dac index

    // Do not use legend.  I will color the text boxes instead.
    //_graph->addToLegend();
    //ui->plotScan->legend->setVisible( true );

}

SenseDACsThread::SenseDACsThread (int devIdx, QCstmDacs * dacs, SpidrController * sc) {

    _dacs = dacs;
    _spidrcontrol = sc;
    _deviceIndex = devIdx;
    // I need to do this here and not when already running the thread
    // Get the IP source address (SPIDR network interface) from the already connected SPIDR module.
    if( _spidrcontrol ) { _spidrcontrol->getIpAddrSrc( 0, &_srcAddr ); }
    else { _srcAddr = 0; }

}

ScanDACsThread::ScanDACsThread (int devIdx, QCstmDacs * dacs, SpidrController * sc) {

    _dacs = dacs;
    _spidrcontrol = sc;
    _deviceIndex = devIdx;
    // I need to do this here and not when already running the thread
    // Get the IP source address (SPIDR network interface) from the already connected SPIDR module.
    if( _spidrcontrol ) { _spidrcontrol->getIpAddrSrc( 0, &_srcAddr ); }
    else { _srcAddr = 0; }

}

void SenseDACsThread::run() {

    // Check if the device is alive
    if ( ! _dacs->GetMpx3GUI()->getConfig()->detectorResponds( _dacs->GetDeviceIndex() ) ) {
        qDebug() << "[ERR ] Device " << _dacs->GetDeviceIndex() << " not responding.";
        return;
    }

    // Open a new temporary connection to the spider to avoid collisions to the main one
    // Extract the ip address
    int ipaddr[4] = { 1, 1, 168, 192 };
    if( _srcAddr != 0 ) {
        ipaddr[3] = (_srcAddr >> 24) & 0xFF;
        ipaddr[2] = (_srcAddr >> 16) & 0xFF;
        ipaddr[1] = (_srcAddr >>  8) & 0xFF;
        ipaddr[0] = (_srcAddr >>  0) & 0xFF;
    }
    SpidrController * spidrcontrol = new SpidrController( ipaddr[3], ipaddr[2], ipaddr[1], ipaddr[0] );

    if ( !spidrcontrol || !spidrcontrol->isConnected() ) {
        qDebug() << "[ERR ] Device not connected !";
        return;
    }

    // Make it update the Tab so the drawing is smooth
    //connect( this, SIGNAL( fillText(QString) ), _dacs->GetUI()->tabDACs, SLOT( update() ) );
    connect( this, SIGNAL( fillText(QString) ), _dacs, SLOT( update() ) );

    // Clean up Labels first
    for (int i = 0 ; i < MPX3RX_DAC_COUNT ; i++) {

        connect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );
        fillText("");
        disconnect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );

    }

    int adc_val = 0;

    progress( 0 );

    for (int i = 0 ; i < MPX3RX_DAC_COUNT ; i++) {

        if ( ! _dacs->GetCheckBoxList()[i]->isChecked() ) continue;

        if ( !spidrcontrol->setSenseDac( _dacs->GetDeviceIndex(), MPX3RX_DAC_TABLE[i].code ) ) {

            qDebug() << "setSenseDac[" << i << "] | " << spidrcontrol->errorString().c_str();

        } else {

            adc_val = 0;

            if ( !spidrcontrol->getDacOut( _dacs->GetDeviceIndex(), &adc_val, _dacs->GetNSamples() ) ) {

                qDebug() << "getDacOut : " << i << " | " << spidrcontrol->errorString().c_str();

            } else {

                adc_val /= _dacs->GetNSamples();
                QString dacOut;
                if ( adc_val > __maxADCCounts || adc_val < 0 ) { // FIXME .. handle the clipping properly
                    dacOut = "clip'ng";
                } else {
                    dacOut = QString::number( (__voltage_DACS_MAX/(double)__maxADCCounts) * (double)adc_val, 'f', 2 );
                    dacOut += "V";
                }

                // Send signal to Labels.  Making connections one by one.
                connect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );
                fillText( dacOut );
                disconnect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );

                // Send signal to progress bar
                progress( floor( ( (double)i / MPX3RX_DAC_COUNT) * 100 ) );
                //cout << i << " --> " << adc_val << endl;

            }

        }

    }

    progress( 100 );

    disconnect( this, SIGNAL( fillText(QString) ), _dacs, SLOT( update() ) );

    // Disconnect the progress bar
    //disconnect( this, SIGNAL( progress(int) ), ui->progressBar, SLOT( setValue(int)) );


    delete spidrcontrol;
}

UpdateDACsThread::UpdateDACsThread (int devIdx, int nDACConfigsAvailable, QCstmDacs * dacs, SpidrController * sc) {

    _dacs = dacs;
    _spidrcontrol = sc;
    _deviceIndex = devIdx;
    _nDACConfigsAvailable = nDACConfigsAvailable;
    _updateInTheChip = false;

    // I need to do this here and not when already running the thread
    // Get the IP source address (SPIDR network interface) from the already connected SPIDR module.
    if( _spidrcontrol ) { _spidrcontrol->getIpAddrSrc( 0, &_srcAddr ); }
    else { _srcAddr = 0; }

}

void UpdateDACsThread::run(){

    // Open a new temporary connection to the spider to avoid collisions to the main one
    // Extract the ip address
    int ipaddr[4] = { 1, 1, 168, 192 };
    if( _srcAddr != 0 ) {
        ipaddr[3] = (_srcAddr >> 24) & 0xFF;
        ipaddr[2] = (_srcAddr >> 16) & 0xFF;
        ipaddr[1] = (_srcAddr >>  8) & 0xFF;
        ipaddr[0] = (_srcAddr >>  0) & 0xFF;
    }

    SpidrController * spidrcontrol = new SpidrController( ipaddr[3], ipaddr[2], ipaddr[1], ipaddr[0] );

    // Ready to take action when the scan is done
    connect( this, SIGNAL( updateFinished() ), _dacs, SLOT( updateFinished() ) );

    if ( !spidrcontrol || !spidrcontrol->isConnected() ) {
        qDebug() << "Device not connected !";
        return;
    }

    int chipMin = 0;
    int chipMax = _nDACConfigsAvailable;
    if( _deviceIndex != -1 ) { // Do only one
        chipMin = _deviceIndex;
        chipMax = _deviceIndex + 1;
    }


    for(int chip = chipMin ; chip < chipMax ; chip++) { // Chip

        // Check if the device is alive
        if ( ! _dacs->GetMpx3GUI()->getConfig()->detectorResponds( chip ) ) {
            qDebug() << "[ERR ] Device " << chip << " not responding.";
            // De-activate the spin boxes and sliders when a non-responding chip is selected
            // TODO !
            continue;
        }

        for(int i = 0 ; i < MPX3RX_DAC_COUNT; i++) { // DACs

            //cout << "chip " << chip << " | " << MPX3RX_DAC_TABLE[i].name
            //     << " | " << _dacs->GetDACValueFromConfig(chip, i) << endl;

            // If requested to send to the chip
            if ( _updateInTheChip ) spidrcontrol->setDac( chip, MPX3RX_DAC_TABLE[i].code,  _dacs->GetDACValueFromConfig(chip, i) );

            //Sleep(10);
            // Adjust the sliders and the SpinBoxes to the new value
            connect( this, SIGNAL( slideAndSpin(int, int) ), _dacs, SLOT( slideAndSpin(int, int) ) );
            slideAndSpin( i, _dacs->GetDACValueFromConfig(chip, i) );
            disconnect( this, SIGNAL( slideAndSpin(int, int) ), _dacs, SLOT( slideAndSpin(int, int) ) );

            //_dacSpinBoxes[i]->setValue( _dacVals[i][chip] );
            //_dacSliders[i]->setValue( _dacVals[i][chip] );

        }
    }
    // Send signal
    updateFinished();

    disconnect( this, SIGNAL( updateFinished() ), _dacs, SLOT( updateFinished() ) );

    // Get rid of this connection
    delete spidrcontrol;
}

void ScanDACsThread::run() {

    // Check if the device is alive
    if ( ! _dacs->GetMpx3GUI()->getConfig()->detectorResponds( _dacs->GetDeviceIndex() ) ) {
        qDebug() << "[ERR ] Device " << _dacs->GetDeviceIndex() << " not responding.";
        return;
    }

    // Open a new temporary connection to the spider to avoid collisions to the main one
    // Extract the ip address
    int ipaddr[4] = { 1, 1, 168, 192 };
    if( _srcAddr != 0 ) {
        ipaddr[3] = (_srcAddr >> 24) & 0xFF;
        ipaddr[2] = (_srcAddr >> 16) & 0xFF;
        ipaddr[1] = (_srcAddr >>  8) & 0xFF;
        ipaddr[0] = (_srcAddr >>  0) & 0xFF;
    }

    SpidrController * spidrcontrol = new SpidrController( ipaddr[3], ipaddr[2], ipaddr[1], ipaddr[0] );

    if ( !spidrcontrol || !spidrcontrol->isConnected() ) {
        qDebug() << "Device not connected !";
        return;
    }

    // Store starting values in order to replace them later
    vector<int> currentDACs;
    int dac_val = 0;
    for (int i = 0 ; i < MPX3RX_DAC_COUNT ; i++) {
        spidrcontrol->getDac(  _dacs->GetDeviceIndex(), MPX3RX_DAC_TABLE[i].code, &dac_val);
        currentDACs.push_back( dac_val );
    }

    // Make it update the Tab so the drawing is smooth
    connect( this, SIGNAL( fillText(QString) ), _dacs, SLOT( update() ) );

    // Connect the plots on DACs
    connect( this, SIGNAL( addData(int, int, double) ), _dacs, SLOT( addData(int, int, double) ) );
    // This SLOT will be taking care of creating the new plot
    connect( this, SIGNAL( addData(int) ), _dacs, SLOT( addData(int) ) );
    connect( this, SIGNAL( scanFinished() ), _dacs, SLOT( scanFinished() ) );

    // For the progress bar verify how many DACs are scheduled for scanning
    // and calculate the number of steps.
    int nStepsToScan = 0;
    for (int i = 0 ; i < MPX3RX_DAC_COUNT ; i++) {
        if ( _dacs->GetCheckBoxList()[i]->isChecked() ) {
            nStepsToScan += (MPX3RX_DAC_TABLE[i].dflt * 2) / _dacs->GetScanStep();
        }
    }

    int adc_val = 0;
    progress( 0 );
    int progressBarCntr = 0;

    for (int i = 0 ; i < MPX3RX_DAC_COUNT ; i++) {

        if ( ! _dacs->GetCheckBoxList()[i]->isChecked() ) continue;

        // Create the plot coming
        addData(i);

        // Clean up the corresponding label
        connect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );
        fillText("");
        disconnect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );

        // Scan !
        for ( int dacValI = 0 ; dacValI < (1<<MPX3RX_DAC_TABLE[i].bits) ; dacValI += _dacs->GetScanStep() ) {


            if ( !spidrcontrol->setDac( _dacs->GetDeviceIndex(), MPX3RX_DAC_TABLE[i].code, dacValI ) ) {

                qDebug() << "setDac[" << i << "] | " << spidrcontrol->errorString().c_str();


            } else {

                // Adjust the sliders and the SpinBoxes to the new value
                connect( this, SIGNAL( slideAndSpin(int, int) ), _dacs, SLOT( slideAndSpin(int, int) ) );
                slideAndSpin( i, dacValI );
                disconnect( this, SIGNAL( slideAndSpin(int, int) ), _dacs, SLOT( slideAndSpin(int, int) ) );

                if ( !spidrcontrol->setSenseDac( _dacs->GetDeviceIndex(), MPX3RX_DAC_TABLE[i].code ) ) {

                    qDebug() << "setSenseDac[" << i << "] | " << spidrcontrol->errorString().c_str();

                } else {

                    adc_val = 0;

                    if ( !spidrcontrol->getDacOut( _dacs->GetDeviceIndex(), &adc_val, _dacs->GetNSamples() ) ) {

                        qDebug() << "getDacOut : " << i << " | " << spidrcontrol->errorString().c_str();

                    } else {

                        // Here we completed the chain: setDac --> setSenseDac --> getDacOut

                        adc_val /= _dacs->GetNSamples();
                        QString dacOut;
                        double adc_volt = 0.;
                        if ( adc_val > __maxADCCounts || adc_val < 0 ) { // FIXME .. handle the clipping properly
                            dacOut = "clip'ng";
                        } else {

                            adc_volt = (__voltage_DACS_MAX/(double)__maxADCCounts) * (double)adc_val;
                            dacOut = QString::number( adc_volt, 'f', 2 );
                            dacOut += "V";
                            addData(i, dacValI, adc_volt);

                        }

                        // Send signal to Labels.  Making connections one by one.
                        connect( this, SIGNAL( fillTextWithIdx(QString, int) ), _dacs, SLOT(setTextWithIdx(QString,int)) );
                        fillTextWithIdx( dacOut, i );
                        disconnect( this, SIGNAL( fillTextWithIdx(QString, int) ), _dacs, SLOT(setTextWithIdx(QString,int) ) );
                        //connect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );
                        //fillText( dacOut );
                        //disconnect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );

                        // Send signal to progress bar
                        progress( floor( ( (double)progressBarCntr / (double)nStepsToScan) * 100 ) );
                        progressBarCntr++;


                    }

                }

            }

        }

        //////////////////////////////////////////////////////////////////////////////
        // And bring back the DAC to it's original value
        int retryCntr = __N_RETRY_ORIGINAL_SETTING;

        if( !spidrcontrol->setDac( _dacs->GetDeviceIndex(), MPX3RX_DAC_TABLE[i].code, currentDACs[i] ) ) {

            if( retryCntr == 0 ) {
                retryCntr = __N_RETRY_ORIGINAL_SETTING;
                qDebug() << "setDac[" << i << "] | " << spidrcontrol->errorString().c_str() << " ... tried 3 times, giving up." << endl;
                continue;
            }
            qDebug() << "setDac[" << i << "] | " << spidrcontrol->errorString().c_str() << " ... retry " << __N_RETRY_ORIGINAL_SETTING - retryCntr + 1 << endl;
            retryCntr--;

        } else {

            // setDac successful
            // Bring the slider and spinBox to the right position
            // Adjust the sliders and the SpinBoxes to the new value
            connect( this, SIGNAL( slideAndSpin(int, int) ), _dacs, SLOT( slideAndSpin(int, int) ) );
            slideAndSpin( i, currentDACs[i] );
            disconnect( this, SIGNAL( slideAndSpin(int, int) ), _dacs, SLOT( slideAndSpin(int, int) ) );

            //////////////////////////////////////////////////////////////////////////////
            // And sense it in the original position
            if ( !spidrcontrol->setSenseDac( _dacs->GetDeviceIndex(), MPX3RX_DAC_TABLE[i].code ) ) {

                qDebug() << "setSenseDac[" << i << "] | " << spidrcontrol->errorString().c_str() << endl;

            } else {

                adc_val = 0;

                if ( !spidrcontrol->getDacOut( _dacs->GetDeviceIndex(), &adc_val, _dacs->GetNSamples() ) ) {

                    qDebug() << "getDacOut : " << i << " | " << spidrcontrol->errorString().c_str();

                } else {

                    adc_val /= _dacs->GetNSamples();
                    QString dacOut;
                    if ( adc_val > __maxADCCounts || adc_val < 0 ) { // FIXME .. handle the clipping properly
                        dacOut = "clip'ng";
                    } else {
                        dacOut = QString::number( (__voltage_DACS_MAX/(double)__maxADCCounts) * (double)adc_val, 'f', 2 );
                        dacOut += "V";
                    }

                    // Send signal to Labels.  Making connections one by one.
                    connect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );
                    fillText( dacOut );
                    disconnect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );

                }

            } //////////////////////////////////////////////////////////////////////////////

        } //////////////////////////////////////////////////////////////////////////////

    }

    progress( 100 );

    scanFinished();

    disconnect( this, SIGNAL( fillText(QString) ), _dacs, SLOT( update() ) );
    disconnect( this, SIGNAL( addData(int, int, double) ), _dacs, SLOT( addData(int, int, double) ) );
    disconnect( this, SIGNAL( addData(int) ), _dacs, SLOT( addData(int) ) );
    disconnect( this, SIGNAL( scanFinished() ), _dacs, SLOT( scanFinished() ) );

    delete spidrcontrol;
}

void QCstmDacs::openWriteMenu(){//TODO: change to signal slot method
    qDebug() << "[INFO] Openwritemenu called!";
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Config"), tr("."), tr("json Files (*.json)"));
    WriteDACsFile(fileName.toStdString());
    //this->WriteDACsFile()
}
