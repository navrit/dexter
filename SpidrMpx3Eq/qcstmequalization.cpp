#include "qcstmequalization.h"
#include "ui_qcstmequalization.h"

#include "qcustomplot.h"
#include "qcstmdacs.h"
#include "mpx3gui.h"
#include "ui_mpx3gui.h"
#include "mpx3eq_common.h"
#include "mpx3dacsdescr.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "barchart.h"
#include "ThlScan.h"

#include <QString>

static QCstmEqualization* eqInstance;

QCstmEqualization::QCstmEqualization(QWidget *parent) :
    QWidget(parent),
    _ui(new Ui::QCstmEqualization)
{
    _ui->setupUi(this);
    QList<int> defaultSizesMain; //The ratio of the splitters. Defaults to the golden ratio because "oh! fancy".
    defaultSizesMain.append(2971215);
    defaultSizesMain.append(1836312);
    QList<int> defaultSizesHist;
    defaultSizesHist.append(2971215);
    defaultSizesHist.append(1836312);

    for(int i = 0; i < _ui->mainSplitter->count();i++){
        QWidget *child = _ui->mainSplitter->widget(i);
        child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        child->setMinimumSize(1,1);
    }
    for(int i = 0; i < _ui->histSetupSplitter->count();i++){
        QWidget *child = _ui->histSetupSplitter->widget(i);
        child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        child->setMinimumSize(1,1);
    }
    _ui->histSetupSplitter->setSizes(defaultSizesHist);
    _ui->mainSplitter->setSizes(defaultSizesMain);

    // Defaults -> init and full-rewindable
    FullEqRewind();

    _fineTuningLoops = 10;
    _ui->fineTuningLoopsSpinBox->setValue( _fineTuningLoops );

    _ui->equalizationTHLTHHCombo->addItem( QString("THL and THH") );
    _ui->equalizationTHLTHHCombo->addItem( QString("Only THL") );
    _ui->equalizationTHLTHHCombo->addItem( QString("Only THH") );
    _equalizationCombination = __THLandTHH; // item 0
    _prevEqualizationCombination = _equalizationCombination;

    _ui->equalizationTypeCombo->addItem( QString("Noise edge") );
    _ui->equalizationTypeCombo->addItem( QString("Noise centroid") );
    _ui->equalizationTypeCombo->addItem( QString("Noise edge FAST") );
    _ui->equalizationTypeCombo->addItem( QString("Noise centroid FAST") );

    _equalizationType = __NoiseEdge; // item 0

    _ui->equalizationSelectTHLTHHCombo->addItem( QString("Show THL") );
    _ui->equalizationSelectTHLTHHCombo->addItem( QString("Show THH") );
    _ui->equalizationSelectTHLTHHCombo->setDisabled( true ); // Only when the equalization is finished this will be enabled
    _equalizationShow = Mpx3EqualizationResults::__ADJ_L;

    _ui->eqLabelFineTuningLoopProgress->setText("-/-");

    // Limits in the input widgets
    SetLimits();

    // Signals and slots
    SetupSignalsAndSlots();

    eqInstance = this;

    // TODO
    // These two buttons will come back as we progress improving the equalization
    _ui->_startEqAll->setVisible( false );
    _ui->h1LogyCheckBox->setVisible( false );
    _ui->h1CheckBox->setVisible( false );
}

void QCstmEqualization::FullEqRewind()
{

    //qDebug() << "[INFO] Eq rewind ...";
    // Initialized in:
    // 1) InitEqualization
    // 2) InitializeEqualizationStructure
    _eqMap.clear();

    _deviceIndex = 0;
    _nTriggers = 1;
    _spacing = 2;

    // This will be recalculated
    _nchipsX = 2;
    _nchipsY = 2;
    _fullsize_x = __matrix_size_x * _nchipsX;
    _fullsize_y = __matrix_size_y * _nchipsY;

    // Suggest a descendant scan
    _maxScanTHL = 0;
    _minScanTHL = MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_0].dflt;
    _scanDescendant = true;
    _busy = false;
    _resdataset = nullptr;
    _gridLayoutHistograms = nullptr;

    _stepScan = _ui->eqStepSpinBox->value(); //__default_step_scan;
    _setId = 0;
    _nChips = 1;
    // List of chip indexes to equalize
    _workChipsIndx.clear();

    _scanAllChips = true;
    _isSequentialAllChipsEqualization = false;

    //////////////////////////////////////////////////////////////////////////////////////////////
    // The first BarChart is created immediately to avoid having the empty space before connection
    _chart.clear();

    BarChart * nbc = new BarChart( GetUI()->layoutWidget );
    nbc->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );
    _chart.push_back( nbc ); // set as parent the same as the one delivered in the UI
    _ui->horizontalLayoutEqHistos->addWidget( nbc );
    nbc->setHidden(true);
    //////////////////////////////////////////////////////////////////////////////////////////////

    _checkBoxes.clear();

    _eqStatus = __INIT;
    _scanIndex = 0;
    for(int i = 0 ; i < __EQStatus_Count ; i++) {
        _stepDone[std::size_t(i)] = false;
    }

    // Clean up the left side
    QLayoutItem * item = nullptr;
    while ( (item = _ui->horizontalLayoutEqHistos->takeAt(0)) ) {
        delete item;
    }
    while ( (item = _ui->horizontalLayout_3->takeAt(0)) ) {
        delete item;
    }

    _steeringInfo.clear();
    _chart.clear();
    _checkBoxes.clear();
}


Ui::QCstmEqualization * QCstmEqualization::GetUI() {
    return _ui;
}

QCstmEqualization * QCstmEqualization::getInstance()
{
    return eqInstance;
}

QCstmEqualization::~QCstmEqualization()
{
    delete _ui;
    delete testPulseEqualisationDialog;
}

void QCstmEqualization::setNHits(int val){
    _nHits = val;
}

void QCstmEqualization::SetLimits(){

    _ui->nHitsSpinBox->setMinimum( 1 );
    _ui->nHitsSpinBox->setMaximum( 1000 );
    _ui->nHitsSpinBox->setValue( 10 );
    _nHits = 10;

    //! Which chip index
    _ui->devIdSpinBox->setMinimum( 0 );
    _ui->devIdSpinBox->setMaximum( 3 );
    _ui->devIdSpinBox->setValue( _deviceIndex );

    //! Number of frames
    _ui->nTriggersSpinBox->setMinimum( 1 );
    _ui->nTriggersSpinBox->setMaximum( 1000 );
    _ui->nTriggersSpinBox->setValue( _nTriggers );

    _ui->spacingSpinBox->setMinimum( 0 );
    _ui->spacingSpinBox->setMaximum( 15 );
    _ui->spacingSpinBox->setValue( _spacing );

    _ui->eqMinSpinBox->setMinimum( 0 );
    _ui->eqMinSpinBox->setMaximum( (MPX3RX_DAC_TABLE[ MPX3RX_DAC_THRESH_0 ].dflt * 2) - 1 );
    _ui->eqMinSpinBox->setValue( _minScanTHL );

    _ui->eqMaxSpinBox->setMinimum( 0 );
    _ui->eqMaxSpinBox->setMaximum( (MPX3RX_DAC_TABLE[ MPX3RX_DAC_THRESH_0 ].dflt * 2) - 1 );
    _ui->eqMaxSpinBox->setValue( _maxScanTHL );

    _ui->eqStepSpinBox->setMinimum( 1 );
    _ui->eqStepSpinBox->setMaximum( (MPX3RX_DAC_TABLE[ MPX3RX_DAC_THRESH_0 ].dflt * 2) - 1 );
    _ui->eqStepSpinBox->setValue( _stepScan );

}

void QCstmEqualization::on_h1LogyCheckBox_toggled(bool checked) {

    // All of them
    int chipListSize = int(_workChipsIndx.size());
    // Report the pixels scheduled for equalization
    for ( int i = 0 ; i < chipListSize ; i++ ) {
        if ( checked ) {
            GetBarChart(_workChipsIndx[std::size_t(i)])->SetLogY( true );
        } else {
            GetBarChart(_workChipsIndx[std::size_t(i)])->SetLogY( false );
        }
        GetBarChart(_workChipsIndx[std::size_t(i)])->replot( QCustomPlot::rpQueued );
    }


}

void QCstmEqualization::setFineTuningLoops(int nLoops) {
    _fineTuningLoops = nLoops;
}

void QCstmEqualization::ShowEqualizationForChip(bool /*checked*/) {

    // At least one has to be checked, otherwise refuse
    bool nothingChecked = true;
    for ( int i = 0 ; i < int(_checkBoxes.size()); i++ ) {
        if ( _checkBoxes[std::size_t(i)]->isChecked() ) {
            nothingChecked = false;
        }
    }
    if ( nothingChecked ) { // Force to leave something checked
        // Leave the check box corresponding to chipId=2 checked
        if ( ! GetCheckBox( 2 ) ) { // but if not available, just the first one
            _checkBoxes[0]->setChecked( true );
        } else {
            GetCheckBox( 2 )->setChecked( true );
        }
    }

    // And now go ahead. Hide all widgets
    for ( int i = int(_workChipsIndx.size() - 1); i >= 0 ; i-- ) {
        GetBarChart( _workChipsIndx[std::size_t(i)] )->hide();
    }

    // Show the new ones
    for ( int i = 0 ; i < int(_checkBoxes.size()); i++ ) {
        if ( _checkBoxes[std::size_t(i)]->isChecked() ) {
            GetBarChart( _workChipsIndx[std::size_t(i)] )->show();
        }
    }

}

void QCstmEqualization::NewRunInitEqualization() {

    // Rewind min and max suggesting a descendant scan.
    SetMinScan( _firstMinScanTHL ); //! This is needed really for noisy sensor/chip combinations, like GaAs MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_0].dflt;
    SetMaxScan( 0 );

    // Delete scans
    _scans.clear();

    _ui->eqLabelFineTuningLoopProgress->setText("-/-");

    // Rewind state machine variables
    _eqStatus = QCstmEqualization::__INIT;
    _scanIndex = 0;
    for(int i = 0 ; i < __EQStatus_Count ; i++) _stepDone[std::size_t(i)] = false;

    // No sets available
    _setId = 0;

    // And step
    //ChangeStep( __default_step_scan );

    int chipListSize = int(_workChipsIndx.size());
    for ( int i = 0 ; i < chipListSize ; i++ ) {

        // Decide on how many thresholds will be equalized
        _steeringInfo[std::size_t(i)]->equalizationCombination = _equalizationCombination;
        // Type: Noise Edge, etc ....
        _steeringInfo[std::size_t(i)]->equalizationType = _equalizationType;

        // Global adj setting used for DAC_DISC_X optimization
        _steeringInfo[std::size_t(i)]->globalAdj = 0;

        // Which one to start with
        // If both ! --> start with TH0
        if ( _equalizationCombination == __OnlyTHL || _equalizationCombination == __THLandTHH ) {
            _steeringInfo[std::size_t(i)]->currentTHx = MPX3RX_DAC_THRESH_0;
            _steeringInfo[std::size_t(i)]->currentTHx_String = "THRESH_0";
            _steeringInfo[std::size_t(i)]->currentDAC_DISC = MPX3RX_DAC_DISC_L;
            _steeringInfo[std::size_t(i)]->currentDAC_DISC_String = "DAC_DISC_L";
        } else if ( _equalizationCombination == __OnlyTHH ) {
            _steeringInfo[std::size_t(i)]->currentTHx = MPX3RX_DAC_THRESH_1;
            _steeringInfo[std::size_t(i)]->currentTHx_String = "THRESH_1";
            _steeringInfo[std::size_t(i)]->currentDAC_DISC = MPX3RX_DAC_DISC_H;
            _steeringInfo[std::size_t(i)]->currentDAC_DISC_String = "DAC_DISC_H";
        }

        // optimization value for the corresponding DAC_DISC
        _steeringInfo[std::size_t(i)]->currentDAC_DISC_OptValue = 0;
        // Eta and Cut for the THL_DACDisc function (DAC_DISC Optimization)
        _steeringInfo[std::size_t(i)]->currentEta_THx_DAC_Disc = 0.;
        _steeringInfo[std::size_t(i)]->currentCut_THx_DAC_Disc = 0.;
        // Eta and Cut for the Adj_THL function (Adj extrapolation)
        _steeringInfo[std::size_t(i)]->currentEta_Adj_THx = 0.;
        _steeringInfo[std::size_t(i)]->currentCut_Adj_THx = 0.;

    }

    // Clean all the BarCharts
    vector<BarChart * >::iterator ic  = _chart.begin();
    vector<BarChart * >::iterator icE = _chart.end();

    // clear the charts to start
    for ( ; ic != icE ; ic++ ) {
        (*ic)->Clean();
    }

    QString startS = "\n-- Eq: ";
    startS += _steeringInfo[0]->currentTHx_String;
    startS += " ----- ";

    // Report the pixels scheduled for equalization
    startS += " CHIP ";
    for ( int i = 0 ; i < chipListSize ; i++ ) {
        startS += "[";
        startS += QString::number(_workChipsIndx[std::size_t(i)], 'd', 0);
        startS += "] ";
    }
    startS += "-----------";

    AppendToTextBrowser( startS );
}

bool QCstmEqualization::InitEqualization(int chipId) {

    // Rewind state machine variables
    _eqStatus = QCstmEqualization::__INIT; //! Important state machine variable
    _scanIndex = 0;
    for(int i = 0 ; i < __EQStatus_Count ; i++) {
        _stepDone[std::size_t(i)] = false;
    }

    _ui->eqLabelFineTuningLoopProgress->setText("-/-");

    // No sets available
    _setId = 0;

    // Clear the previous scans!
    _scans.clear();

    // And step
    //ChangeStep( __default_step_scan );

    // Clear the list of chips
    _workChipsIndx.clear();

    // Check if we can talk to the chip
    if ( chipId != -1 ) {
        if ( ! _mpx3gui->getConfig()->detectorResponds( chipId ) ) {
            return false;   // NOTHING TO WORK WITH
        }
        // Only this chip
        _workChipsIndx.push_back( chipId );
    } else {
        // Check if at least one chip is ON.
        // In this case the routine will equalize all pixels present.
        int nChips = _mpx3gui->getConfig()->getNDevicesSupported();
        // Go through all chips avoiding those not present
        for ( int devIdx = 0 ; devIdx < nChips ; devIdx++ ) {
            if ( ! _mpx3gui->getConfig()->detectorResponds( devIdx ) ) continue;
            // push back the indexes of the good chips
            _workChipsIndx.push_back( devIdx );
        }
        if ( _workChipsIndx.size() == 0 ) return false;   // NOTHING TO WORK WITH
    }

    // _workChipsIndx
    _nChips = int(_workChipsIndx.size());

    // Create a steering structure for each chip
    // How many chips to equalize
    int chipListSize = int(_workChipsIndx.size());

    // Check if the structures are already there. For instance after a stop.
    for ( int i = 0 ; i < chipListSize ; i++ ) {

        // Create the steering info for this chip
        _steeringInfo.push_back( new equalizationSteeringInfo );

        // Decide on how many thresholds will be equalized
        _steeringInfo[std::size_t(i)]->equalizationCombination = _equalizationCombination;
        // Type: Noise Edge, etc ....
        _steeringInfo[std::size_t(i)]->equalizationType = _equalizationType;

        // Global adj setting used for DAC_DISC_X optimization
        _steeringInfo[std::size_t(i)]->globalAdj = 0;

        // Which one to start with
        // If both ! --> start with TH0
        if ( _equalizationCombination == __OnlyTHL || _equalizationCombination == __THLandTHH ) {
            _steeringInfo[std::size_t(i)]->currentTHx = MPX3RX_DAC_THRESH_0;
            _steeringInfo[std::size_t(i)]->currentTHx_String = "THRESH_0";
            _steeringInfo[std::size_t(i)]->currentDAC_DISC = MPX3RX_DAC_DISC_L;
            _steeringInfo[std::size_t(i)]->currentDAC_DISC_String = "DAC_DISC_L";
        } else if ( _equalizationCombination == __OnlyTHH ) {
            _steeringInfo[std::size_t(i)]->currentTHx = MPX3RX_DAC_THRESH_1;
            _steeringInfo[std::size_t(i)]->currentTHx_String = "THRESH_1";
            _steeringInfo[std::size_t(i)]->currentDAC_DISC = MPX3RX_DAC_DISC_H;
            _steeringInfo[std::size_t(i)]->currentDAC_DISC_String = "DAC_DISC_H";
        }

        // optimization value for the corresponding DAC_DISC
        _steeringInfo[std::size_t(i)]->currentDAC_DISC_OptValue = 0;
        // Eta and Cut for the THL_DACDisc function (DAC_DISC Optimization)
        _steeringInfo[std::size_t(i)]->currentEta_THx_DAC_Disc = 0.;
        _steeringInfo[std::size_t(i)]->currentCut_THx_DAC_Disc = 0.;
        // Eta and Cut for the Adj_THL function (Adj extrapolation)
        _steeringInfo[std::size_t(i)]->currentEta_Adj_THx = 0.;
        _steeringInfo[std::size_t(i)]->currentCut_Adj_THx = 0.;
    }

    // BarCharts !
    InitializeBarChartsEqualization();

    // THx scan label
    if( _steeringInfo[0]->currentTHx == MPX3RX_DAC_THRESH_0) _ui->thxLabel->setText( "THL:" );
    if( _steeringInfo[0]->currentTHx == MPX3RX_DAC_THRESH_1) _ui->thxLabel->setText( "THH:" );

    QString startS = "\n-- Eq: ";
    startS += _steeringInfo[0]->currentTHx_String;
    startS += " ----- ";

    // Report the pixels scheduled for equalization
    startS += " CHIP ";
    for ( int i = 0 ; i < chipListSize ; i++ ) {
        startS += "[";
        startS += QString::number(_workChipsIndx[std::size_t(i)], 'd', 0);
        startS += "] ";
    }
    startS += "-----------";

    AppendToTextBrowser( startS );

    // Pick what to show
    ShowEqualizationForChip( true );

    _nchipsX = 2; //_mpx3gui->getDataset()->getNChipsX();
    _nchipsY = 2; //_mpx3gui->getDataset()->getNChipsY();
    // TODO.  When one chips is connected the dataset returns 2,1 (which is good)
    _fullsize_x = __matrix_size_x * _nchipsX;
    _fullsize_y = __matrix_size_y * _nchipsY;


    // Create an equalization per chip
    for ( int i = 0 ; i < chipListSize ; i++ ) {
        _eqMap[_workChipsIndx[std::size_t(i)]] = new Mpx3EqualizationResults;
    }

    return true;
}

void QCstmEqualization::InitializeBarChartsEqualization() {

    // Otherwise push the rest
    vector<int>::iterator i  = _workChipsIndx.begin();
    vector<int>::iterator iE = _workChipsIndx.end();

    // The one built in the constructor will be erased here
    _ui->horizontalLayoutEqHistos->removeWidget( _chart[0] );
    _chart.clear();

    // One result object per chip to be equalized
    for ( ; i != iE ; i++ ) {

        QString barChartName = "histoWidget_";
        barChartName += QString::number(_workChipsIndx[0], 'd', 0);

        BarChart * nbc = new BarChart( GetUI()->layoutWidget );
        nbc->setObjectName(barChartName);
        nbc->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );

        //
        _ui->horizontalLayoutEqHistos->addWidget( nbc );

        _chart.push_back( nbc ); // set as parent the same as the one delivered in the UI
    }

    // Add the legend for the chip check boxes, and the check boxes
    QLabel * labelChipsEq = new QLabel(_ui->layoutWidget);
    labelChipsEq->setObjectName(QStringLiteral("labelChipsEq"));
    labelChipsEq->setText("Select chip: ");
    _ui->horizontalLayout_3->addWidget(labelChipsEq);

    for ( i = _workChipsIndx.begin() ; i != iE ; i++ ) {

        // Add the check boxes for them
        QCheckBox * chipOn = new QCheckBox(_ui->layoutWidget);
        chipOn->setObjectName(QStringLiteral("chipOn"));
        QString chipOn_S = "[";
        chipOn_S += QString::number(*i, 'd', 0);
        chipOn_S += "]";
        chipOn->setText( chipOn_S );
        _ui->horizontalLayout_3->addWidget(chipOn);

        // save the pointers
        _checkBoxes.push_back( chipOn );
        // And connect them
        connect( chipOn, SIGNAL(toggled(bool)), this, SLOT( ShowEqualizationForChip(bool) ) );
    }

    // Leave the check box corresponding to chipId=2 checked
    if ( ! GetCheckBox( 2 ) ) { // but if not available, just the first one
        _checkBoxes[0]->setChecked( true );
    } else {
        GetCheckBox( 2 )->setChecked( true );
    }

    // Clean all the BarCharts
    vector<BarChart * >::iterator ic  = _chart.begin();
    vector<BarChart * >::iterator icE = _chart.end();

    // clear the charts to start
    for ( ; ic != icE ; ic++ ) {
        (*ic)->Clean();
    }
}

QCheckBox * QCstmEqualization::GetCheckBox(int chipIdx) {

    // There should be as results objects as chips to be equalized
    if ( _workChipsIndx.size() != _checkBoxes.size() ) return nullptr;

    // Find the index
    for (int i = 0; i < int(_workChipsIndx.size()); i++ ) {
        // return the corresponding results Ptr
        if ( _workChipsIndx[std::size_t(i)] == chipIdx ) {
            return _checkBoxes[std::size_t(i)];
        }
    }

    return nullptr; // otherwise
}

equalizationSteeringInfo * QCstmEqualization::GetSteeringInfo(int chipIdx) {

    //i//
    // There should be as results objects as chips to be equalized
    //
    //if ( _workChipsIndx.size() != _steeringInfo.size() ) return nullptr;

    // There should be enough steering info objects for the chips requested to be equalized.
    // If there are more steering info information objects, find the right one.
    if ( _steeringInfo.size() < _workChipsIndx.size() ) return nullptr;

    // Find the index
    for (int i = 0 ; i < int(_workChipsIndx.size()); i++ ) {
        // return the corresponding results Ptr
        if ( _workChipsIndx[std::size_t(i)] == chipIdx ) {
            return _steeringInfo[std::size_t(i)];
        }
    }

    return nullptr; // otherwise
}

BarChart * QCstmEqualization::GetAdjBarChart(int chipIdx, Mpx3EqualizationResults::lowHighSel sel) {

    // There should be twice as many AdjBarChart objects as chips to be equalized
    //  Low and High
    if ( _workChipsIndx.size() != _adjchart_L.size() ) return nullptr;
    if ( _workChipsIndx.size() != _adjchart_H.size() ) return nullptr;

    for (int i = 0 ; i < int(_workChipsIndx.size()); i++ ) {
        // return the corresponding results Ptr
        if ( _workChipsIndx[std::size_t(i)] == chipIdx ) {
            if ( sel == Mpx3EqualizationResults::__ADJ_L) return _adjchart_L[std::size_t(i)];
            if ( sel == Mpx3EqualizationResults::__ADJ_H) return _adjchart_H[std::size_t(i)];
        }
    }

    return nullptr; // otherwise
}

BarChart * QCstmEqualization::GetBarChart(int chipIdx) {

    // There should be as results objects as chips to be equalized
    if ( _workChipsIndx.size() != _chart.size() ) {
        return nullptr;
    }

    // Find the index
    for (unsigned long i = 0; i < _workChipsIndx.size(); i++ ) {
        // return the corresponding results Ptr
        if ( _workChipsIndx[std::size_t(i)] == chipIdx ) {
            return _chart[std::size_t(i)];
        }
    }

    return nullptr; // otherwise
}

//! See if the pixel is in the list of chips scheduled to equalise
bool QCstmEqualization::pixelInScheduledChips(int pix) {

    unsigned long chipListSize = _workChipsIndx.size();

    for ( unsigned long i = 0 ; i < chipListSize ; i++ ) {

        if ( pix >= ( _workChipsIndx[std::size_t(i)] * __matrix_size )  &&
             pix <  ( (_workChipsIndx[std::size_t(i)] + 1) * __matrix_size )) {
            return true;
        }
    }
    return false;
}

//! When equalising only one chip, other chips may be still connected and need
//! to remain quiet. Set thresholds for those chips high.
void QCstmEqualization::KeepOtherChipsQuiet() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    int ndevs = _mpx3gui->getConfig()->getNDevicesSupported();
    int chipListSize = int(_workChipsIndx.size());

    for ( int idx = 0 ; idx < ndevs ; idx++ ) {

        // If the device is not present continue
        if ( ! _mpx3gui->getConfig()->detectorResponds( idx ) ) continue;

        // If the device is present and it is not in the schedule-for-eq list
        //  apply the high thresholds
        for ( int i = 0; i < chipListSize; i++ ) if ( _workChipsIndx[std::size_t(i)] == idx ) continue;

        //! Set all the thresholds to the same value. If we're not in colour mode then they aren't connected anyway so it doesn't matter
        for (int DAC_CODE = MPX3RX_DAC_THRESH_0; DAC_CODE <= MPX3RX_DAC_THRESH_7; DAC_CODE++) {
            SetDAC_propagateInGUI( spidrcontrol, idx, DAC_CODE, __low_but_above_noise_threshold);
        }
    }
}

void QCstmEqualization::resetThresholds()
{
    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    int ndevs = _mpx3gui->getConfig()->getNDevicesSupported();
    int default_value = 42;

    for ( int idx = 0 ; idx < ndevs ; idx++ ) {

        // If the device is not present continue
        if ( ! _mpx3gui->getConfig()->detectorResponds( idx ) ) continue;

        //! Reset THRESH DACs back to "default" values so I don't have to do it manually...

        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_0,  default_value);
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_1,  default_value+2);
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_2,  default_value+4);
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_3,  default_value+6);
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_4,  default_value+8);
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_5,  default_value+10);
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_6,  default_value+12);
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_7,  default_value+14);

    }
}

void QCstmEqualization::StartEqualizationSingleChip() {

    if ( ! _isSequentialAllChipsEqualization ) {
        if ( ! makeTeaCoffeeDialog() ) return;
    }

    // Get busy !
    _busy = true;
    _scanAllChips = false;

    // Init
    if ( ! InitEqualization( _deviceIndex ) ) return;

    KeepOtherChipsQuiet();

    StartEqualization( );

}

void QCstmEqualization::StartEqualizationSequentialSingleChips()
{

    // Only the first time
    // Start with chip 0
    if ( ! _isSequentialAllChipsEqualization ) {
        // ! artifact only for the first time we call this function
        // if the user accepts to continue _deviceIndex will start at 0
        _deviceIndex = -1;
        _isSequentialAllChipsEqualization = true;
        if(!_isRemotePath){
            if ( ! makeTeaCoffeeDialog() ) {
                ChangeDeviceIndex(0, true);
                return;
            }
        }
    }

    // Next chip
    ChangeDeviceIndex(++_deviceIndex, true);

    StartEqualizationSingleChip();
}

void QCstmEqualization::StartEqualizationSequentialSingleChipsRemotely(QString path)
{
    _isRemotePath = true;
    _remotePath = path;
    StartEqualizationSequentialSingleChips();
}

void QCstmEqualization::StartEqualizationAllChips() {

    if(makeTeaCoffeeDialog()){
        // Get busy !
        _busy = true;
        _scanAllChips = true;

        // Initialise
        if ( ! InitEqualization( -1 ) ) return;

        StartEqualization( ); // Default will equalize all chips

    } else {
        return;
    }
}

void QCstmEqualization::StartEqualization() {

    /* So that the equalisation won't save the mask file to the wrong folder during equalisation */
    emit equalizationPathExported("");

    // I need to do this here and not when already running the thread
    // Get the IP source address (SPIDR network interface) from the already connected SPIDR module.
    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    if( spidrcontrol ) { spidrcontrol->getIpAddrSrc( 0, &_srcAddr ); }
    else { _srcAddr = 0; }

    // how many chips to equalize
    uint chipListSize = uint(_workChipsIndx.size());
    //qDebug() << "... " << chipListSize;

    if (testPulseEqualisationDialog != nullptr) {
        defaultNoiseEqualisationTarget = testPulseEqualisationDialog->getEqualisationTarget();
        DAC_DISC_1_value = testPulseEqualisationDialog->get_1st_DAC_DISC_val();
        DAC_DISC_2_value = testPulseEqualisationDialog->get_2nd_DAC_DISC_val();
    }

    // Preliminary) Find out the equalization range

    // First) DAC_Disc Optimization
    if( EQ_NEXT_STEP( __INIT) ) {
        //! We only want to update this the first time
        if (_steeringInfo[0]->currentDAC_DISC_String == "DAC_DISC_L") {
            _firstMinScanTHL = _minScanTHL; //! This is used exclusively to guide the THH scan for noisy chip/sensor combos
        }
        ChangeMin( _firstMinScanTHL );

        // ------ //
        // STEP 1 //
        // ------ //
        QString titleInit = "1) ";
        titleInit += _steeringInfo[0]->currentDAC_DISC_String; // it will be the same for all chips
        titleInit += " optimization ...";
        AppendToTextBrowser( titleInit );

        // CONFIG for all involved chips
        for ( uint i = 0 ; i < chipListSize ; i++ ) {
            Configuration(_workChipsIndx[std::size_t(i)], _steeringInfo[std::size_t(i)]->currentTHx, true);
            _steeringInfo[std::size_t(i)]->globalAdj = 0;
            _steeringInfo[std::size_t(i)]->currentDAC_DISC_OptValue = int(DAC_DISC_1_value); // for now make the opt value equal to the test value
            //! Default = 100 for noise (SLGM)
            qDebug() << "[INFO]\tCurrent DAC DISC Value [" << i << "] =" << _steeringInfo[i]->currentDAC_DISC_OptValue;
        }

        //i//_spacing = 1; //! Need to have _spacing = 1 until __PrepareInterpolation_0x0
        qDebug() << "[INFO] \tEqualisation setting pixel spacing to 1 until Fine tuning";

        // Prepare and launch the thread
        DAC_Disc_Optimization_100( );

    } else if ( EQ_NEXT_STEP(__DAC_Disc_Optimization_100) ) {

        // Check if the previous scan has been stopped by the user

        for ( uint i = 0 ; i < chipListSize ; i++ ) {
            // Extract results from immediately previous scan. Calc the stats now (this is quick)
            _scans[_scanIndex - 1]->ExtractStatsOnChart(_workChipsIndx[i], _setId - 1);
            // Show the results
            DAC_Disc_Optimization_DisplayResults( _scans[_scanIndex - 1]->GetScanResults(_workChipsIndx[i]) );
            // New text value
            _steeringInfo[i]->currentDAC_DISC_OptValue = int(DAC_DISC_2_value); // for now make the opt value equal to the test value
            //! Default = 150 for noise (SLGM)
            qDebug() << "[INFO]\tCurrent DAC DISC Value [" << i << "] =" << _steeringInfo[i]->currentDAC_DISC_OptValue;
        }

        // And go for next scan
        DAC_Disc_Optimization_150( );

    } else if ( EQ_NEXT_STEP(__DAC_Disc_Optimization_150 ) ) {

        for ( unsigned long i = 0 ; i < chipListSize ; i++ ) {
            // Extract results from immediately previous scan. Calc the stats now (this is quick)
            _scans[_scanIndex - 1]->ExtractStatsOnChart(_workChipsIndx[i], _setId - 1);
            // Show the results
            DAC_Disc_Optimization_DisplayResults( _scans[_scanIndex - 1]->GetScanResults(_workChipsIndx[i]) );
        }

        // And calculate the optimal DAC_Disc
        for ( uint i = 0 ; i < chipListSize ; i++ ) {
            ScanResults * res_100 = _scans[_scanIndex - 2]->GetScanResults( _workChipsIndx[i] );
            ScanResults * res_150 = _scans[_scanIndex - 1]->GetScanResults( _workChipsIndx[i] );
            DAC_Disc_Optimization(_workChipsIndx[i], res_100, res_150);
        }

        // ------ //
        // STEP 2 //
        // ------ //

        AppendToTextBrowser("2) Test adj-bits sensibility and extrapolate to target ...");
        PrepareInterpolation_0x0();

    } else if ( EQ_NEXT_STEP(__PrepareInterpolation_0x0) ) {

        // Results
        int nNonReactive = _scans[_scanIndex - 1]->NumberOfNonReactingPixels();
        // Correct in case not all chips are active
        nNonReactive -= (_mpx3gui->getConfig()->getNDevicesSupported() - chipListSize)*__matrix_size;
        if ( nNonReactive > 0 ) {
            qDebug() << "[WARNING] there are non reactive (maybe dead) pixels : " << nNonReactive << "\n";
        }

        for ( uint i = 0 ; i < chipListSize ; i++ ) {
            // Results
            _scans[_scanIndex - 1]->ExtractStatsOnChart(_workChipsIndx[i], _setId - 1);
            DisplayStatsInTextBrowser(_steeringInfo[i]->globalAdj,_steeringInfo[i]->currentDAC_DISC_OptValue, _scans[_scanIndex - 1]->GetScanResults(_workChipsIndx[i]));
            _steeringInfo[i]->globalAdj = 5; // Global adjustment changed !
        }
        // Now adj=0x5
        PrepareInterpolation_0x5();

    } else if ( EQ_NEXT_STEP(__PrepareInterpolation_0x5) ) {

        // Results
        int nNonReactive = _scans[_scanIndex - 1]->NumberOfNonReactingPixels();
        // Correct in case not all chips are active
        nNonReactive -= (_mpx3gui->getConfig()->getNDevicesSupported() - chipListSize)*__matrix_size;
        if ( nNonReactive > 0 ) {
            qDebug() << "[WARNING] there are non reactive pixels : " << nNonReactive << endl;
        }

        // Use a data set to put the adj matrixes together
        if ( ! _resdataset ) _resdataset = new Dataset ( __matrix_size_x, __matrix_size_y, _nchipsX*_nchipsY );
        _resdataset->clear();

        for ( uint i = 0 ; i < chipListSize ; i++ ) {
            _scans[_scanIndex - 1]->ExtractStatsOnChart(_workChipsIndx[i], _setId - 1);
            DisplayStatsInTextBrowser(_steeringInfo[i]->globalAdj, _steeringInfo[i]->currentDAC_DISC_OptValue, _scans[_scanIndex - 1]->GetScanResults(_workChipsIndx[i]));

            // Interpolate now
            ThlScan * scan_x0 = _scans[_scanIndex - 2];
            ThlScan * scan_x5 = _scans[_scanIndex - 1];

            //! The equalisation target is noise based at this point
            int * adjdata = CalculateInterpolation( _workChipsIndx[i], scan_x0, scan_x5 );
            // Stack
            _resdataset->setFrame(adjdata, _workChipsIndx[i], 0);

        }

        // Once the frame is complete, extract info
        int * fulladjdata = _resdataset->getLayer( 0 );
        UpdateHeatMap(fulladjdata, _fullsize_x, _fullsize_y);

        ChangeSpacing(_ui->spacingSpinBox->value()); //! Change the spacing back to the GUI content

        // Perform now a scan with the extrapolated adjustments
        //    Here there's absolutely no need to go through the THL range.
        // New limits --> ask the last scan
        ScanOnInterpolation();
        ChangeSpacing(_ui->spacingSpinBox->value()); //! Change the spacing back to the GUI content


    } else if ( EQ_NEXT_STEP( __ScanOnInterpolation) ) {

        // Results
        int nNonReactive = _scans[_scanIndex - 1]->NumberOfNonReactingPixels();
        // Correct in case not all chips are active
        nNonReactive -= (_mpx3gui->getConfig()->getNDevicesSupported() - chipListSize)*__matrix_size;

        for ( uint i = 0 ; i < chipListSize ; i++ ) {
            _scans[_scanIndex - 1]->ExtractStatsOnChart(_workChipsIndx[i], _setId - 1);
            DisplayStatsInTextBrowser(-1, _steeringInfo[i]->currentDAC_DISC_OptValue, _scans[_scanIndex - 1]->GetScanResults(_workChipsIndx[i]));

        }

        if ( nNonReactive > 0 ) {
            qDebug() << "[WARNING] there are non reactive pixels : " << nNonReactive << endl;
        }

        //ScanThreadFinished(); //! Disable this temporarily for testing
        estimateEqualisationTarget();

    } else if ( EQ_NEXT_STEP(__EstimateEqualisationTarget) ) {

        // Results
        int nNonReactive = _scans[_scanIndex - 1]->NumberOfNonReactingPixels();
        // Correct in case not all chips are active
        nNonReactive -= (_mpx3gui->getConfig()->getNDevicesSupported() - chipListSize)*__matrix_size;

        if ( nNonReactive > 0 ) {
            qDebug() << "[WARNING] there are non reactive pixels : " << nNonReactive << endl;
        }

        for ( uint i = 0 ; i < chipListSize ; i++ ) {
            _scans[_scanIndex - 1]->ExtractStatsOnChart(_workChipsIndx[i], _setId - 1);
            DisplayStatsInTextBrowser(-1, _steeringInfo[i]->currentDAC_DISC_OptValue, _scans[_scanIndex - 1]->GetScanResults(_workChipsIndx[i]));
        }

        // If fast equalization, skip finetuning, same for all chips
        if ( _steeringInfo[0]->equalizationType == __NoiseCentroidFAST ||
             _steeringInfo[0]->equalizationType == __NoiseEdgeFAST ) {

            // Go directly to next scan
            // Go to next step, except for fine tuning where I use the same previous scan
            _stepDone[_eqStatus] = true;
            _eqStatus++;
            StartEqualization( );

        } else {
            AppendToTextBrowser("3) Fine tuning ...");

            // 5) Attempt fine tuning
            FineTuning( );
        }

    }  else if ( EQ_NEXT_STEP( __FineTuning ) ) {

        // clear previous data
        _resdataset->clear();
        for ( uint i = 0 ; i < chipListSize ; i++ ) {
            int * da = _eqMap[_workChipsIndx[i]]->GetAdjustementMatrix();
            _resdataset->setFrame(da, _workChipsIndx[i], 0);
        }
        int * fulladjdata = _resdataset->getLayer( 0 );
        UpdateHeatMap(fulladjdata, _fullsize_x, _fullsize_y);

        // Decide if the equalization needs to be ran again for THH or if we are done
        if ( _equalizationCombination == __THLandTHH ) {
            // At this point we finished THL. Go for THH now
            _prevEqualizationCombination = _equalizationCombination;
            _equalizationCombination = __OnlyTHH;

            // partial re-initialization
            NewRunInitEqualization();

            // Good for single chip equalization
            //! Oooooh, well this would have been amazing to know before...
            if ( ! _scanAllChips ) KeepOtherChipsQuiet();

            // Start again
            StartEqualization();

        } else {

            // come back to the initial setting for equalization combination
            _equalizationCombination = _prevEqualizationCombination;

            // If we're done see if we need to continue with the next chip or
            //  if we're really done.
            // TODO ! If the active chips are not in a sequence this won't work.
            //        If all chips are installed there is no problem.
            if ( _isSequentialAllChipsEqualization && _deviceIndex < _mpx3gui->getConfig()->getNActiveDevices()-1 ) {

                SaveEqualization( QDir::currentPath(), true );

                resetForNewEqualisation();

                StartEqualizationSequentialSingleChips();

            } else {

                SaveEqualization( "", false, true );

                resetForNewEqualisation();

                AppendToTextBrowser( "[DONE]" );

            }

            QString msg = "Finished equalisation on: " + _mpx3gui->getVisualization()->getStatsString_deviceId();
            emit sig_statusBarAppend(msg, "green");
        }
    }

    // -------------------------------------------------------------------------
    // !!! ATTENTION !!!
    // THL1 Eq, set the Equalization for THL1 eq
    //  but the results comes in the counter 0 !!!

}

void QCstmEqualization::UpdateHeatMap(int * data, int sizex, int sizey) {

    // The heat map takes the 1-D array _data and plots bottom left --> top right
    // _data comes:
    // -) For multiple chips
    // [0](0 ---> nx*ny) [1](0 ---> nx*ny) [2](0 ---> nx*ny) [3](0 ---> nx*ny)

    //
    int * _plotdata = new int[sizex * sizey];

    unsigned int xswitch = 1;
    unsigned int yswitch = 0;
    int xoffset = 0, yoffset = 0;

    int sizex_chip = sizex / 2;
    int sizey_chip = sizey / 2;

    int totCntr = 0;

    for ( int chipIdx = 0 ; chipIdx < _mpx3gui->getConfig()->getNDevicesSupported() ; chipIdx++ ) {

        // where to start in X
        if ( (xswitch>>1 & 0x1) == 0 ) xoffset = 0;
        else xoffset = sizex_chip;

        // where to start in Y
        if ( (yswitch>>1 & 0x1) == 0 ) yoffset = sizey_chip;
        else yoffset = 0;

        // Now go chip by chip
        for ( int j = 0 ; j < sizey_chip ; j++ ) {
            for ( int i = 0 ; i < sizex_chip ; i++ ) {
                _plotdata[ XYtoX( xoffset + i , yoffset + j, sizex ) ] = data[ totCntr++ ];
            }
        }
        xswitch++;
        yswitch++;
    }

    _ui->_intermediatePlot->setData( _plotdata, sizex, sizey);
}


int * QCstmEqualization::CalculateInterpolation(int devId, ThlScan * scan_x0, ThlScan * scan_x5 ) {

    // -------------------------------------------------------------------------
    // 6) Establish the dependency THL(Adj). It will be used to extrapolate to the
    //    Equalization target for every pixel

    ScanResults * res_x0 = scan_x0->GetScanResults( devId );
    ScanResults * res_x5 = scan_x5->GetScanResults( devId );

    double gradient = 0., y_intercept = 0.;

    qDebug() << "[INFO]\tEqualisation target =" << defaultNoiseEqualisationTarget;

    GetSlopeAndCut_Adj_THL(res_x0, res_x5, gradient, y_intercept);
    GetSteeringInfo(devId)->currentEta_Adj_THx = gradient;
    GetSteeringInfo(devId)->currentCut_Adj_THx = y_intercept;

    Mpx3EqualizationResults::lowHighSel sel = Mpx3EqualizationResults::__ADJ_L;
    if ( GetSteeringInfo(devId)->currentDAC_DISC == MPX3RX_DAC_DISC_L ) sel = Mpx3EqualizationResults::__ADJ_L;
    if ( GetSteeringInfo(devId)->currentDAC_DISC == MPX3RX_DAC_DISC_H ) sel = Mpx3EqualizationResults::__ADJ_H;

    // -------------------------------------------------------------------------
    // 7) Extrapolate to the target using the last scan information and the knowledge
    //    on the Adj_THL dependency.
    _scans[_scanIndex - 1]->DeliverPreliminaryEqualization(devId, GetSteeringInfo(devId)->currentDAC_DISC, _eqMap[devId],  GetSteeringInfo(devId)->globalAdj );
    _eqMap[devId]->ExtrapolateAdjToTarget( defaultNoiseEqualisationTarget, GetSteeringInfo(devId)->currentEta_Adj_THx, sel );

    int * adj_matrix = nullptr;
    if ( GetSteeringInfo(devId)->currentDAC_DISC == MPX3RX_DAC_DISC_L ) adj_matrix = _eqMap[devId]->GetAdjustementMatrix();
    if ( GetSteeringInfo(devId)->currentDAC_DISC == MPX3RX_DAC_DISC_H ) adj_matrix = _eqMap[devId]->GetAdjustementMatrix(Mpx3EqualizationResults::__ADJ_H);

    return adj_matrix;
}

void QCstmEqualization::DAC_Disc_Optimization_DisplayResults(ScanResults * res) {

    DisplayStatsInTextBrowser(0, res->DAC_DISC_setting, res);
}

string QCstmEqualization::BuildChartName(int val, QString leg) {

    QString chartName = "[";
    chartName += QString::number(val, 'd', 0);
    chartName += "] ";
    chartName += leg;

    return chartName.toStdString();
}

void QCstmEqualization::DAC_Disc_Optimization_100() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    QString legend = _steeringInfo[0]->currentDAC_DISC_String; // is the same for all chips
    legend += QString::number(_steeringInfo[0]->currentDAC_DISC_OptValue, 'd', 0);

    // arbitrary step for DAC_Disc_Optimization
//    ChangeStep( __DAC_Disc_Optimization_step, true );
    ChangeMax( 0, true );
    ChangeMin( __half_DAC_range-1, true );

    // -------------------------------------------------------------------------
    // 1) Scan with MPX3RX_DAC_DISC_L = 100
    ThlScan * tscan = new ThlScan(_mpx3gui, this);
    tscan->ConnectToHardware(spidrcontrol, spidrdaq);
    // Append the data set which will be used for this scan
    BarChartProperties cprop;
    cprop.xAxisLabel = "THL";
    cprop.yAxisLabel = "entries";
    cprop.min_x = 0;
    cprop.max_x = 511;
    cprop.nBins = 512;
    cprop.color_r = 90;
    cprop.color_g = 200;
    cprop.color_b = 250;

    for ( int i = 0 ; i < int(_workChipsIndx.size()); i++ ) {
        cprop.name = BuildChartName( _workChipsIndx[i], legend );
        GetBarChart( _workChipsIndx[i] )->AppendSet( cprop );
        GetBarChart( _workChipsIndx[i] )->rescaleX( double(_ui->eqMaxSpinBox->value()), double(_ui->eqMinSpinBox->value()));
    }

    // DAC_DiscL=100
    for ( int i = 0 ; i < int(_workChipsIndx.size()); i++ ) {
        SetDAC_propagateInGUI(spidrcontrol, _workChipsIndx[i], _steeringInfo[i]->currentDAC_DISC, _steeringInfo[i]->currentDAC_DISC_OptValue);
        spidrcontrol->setInternalTestPulse(_workChipsIndx[i], false);
    }

    // This is a scan that I can truncate early ... I don't need to go all the way
    tscan->DoScan(  _steeringInfo[0]->currentTHx , _setId++, _steeringInfo[0]->currentDAC_DISC, 1, false, false ); // THX and DAC_DISC_X same for all chips
    tscan->SetAdjustmentType( ThlScan::__adjust_to_global );
    tscan->SetWorkChipIndexes( _workChipsIndx, _steeringInfo );

    // Launch as thread.  Connect the slot which signals when it's done
    _scans.push_back( tscan ); _scanIndex++;
    connect( tscan, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
    tscan->start();
}

void QCstmEqualization::DAC_Disc_Optimization_150() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    QString legend = _steeringInfo[0]->currentDAC_DISC_String;
    legend += QString::number(_steeringInfo[0]->currentDAC_DISC_OptValue, 'd', 0);

    // arbitrary step for DAC_Disc_Optimization
//    ChangeStep( __DAC_Disc_Optimization_step, true );
    ChangeMax( 0, true );
    ChangeMin( __half_DAC_range-1, true );

    // -------------------------------------------------------------------------
    // 2) Scan with MPX3RX_DAC_DISC_L = 150
    ThlScan * tscan = new ThlScan(_mpx3gui, this);
    tscan->ConnectToHardware(spidrcontrol, spidrdaq);
    BarChartProperties cprop_150;
    cprop_150.min_x = 0;
    cprop_150.max_x = 200;
    cprop_150.nBins = 512;
    cprop_150.color_r = 0;
    cprop_150.color_g = 122;
    cprop_150.color_b = 255;
    for ( int i = 0 ; i < int(_workChipsIndx.size()); i++ ) {
        cprop_150.name = BuildChartName( _workChipsIndx[i], legend );
        GetBarChart( _workChipsIndx[i] )->AppendSet( cprop_150 );
    }

    // DAC_DiscL=150
    for ( int i = 0 ; i < int(_workChipsIndx.size()); i++ ) {
        spidrcontrol->setDac( _workChipsIndx[i], _steeringInfo[i]->currentDAC_DISC, _steeringInfo[i]->currentDAC_DISC_OptValue );
    }
    tscan->DoScan( _steeringInfo[0]->currentTHx, _setId++,  _steeringInfo[0]->currentDAC_DISC, 1, false, false );
    tscan->SetWorkChipIndexes( _workChipsIndx, _steeringInfo );

    // Launch as thread.  Connect the slot which signals when it's done
    _scans.push_back( tscan ); _scanIndex++;
    connect( tscan, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
    tscan->start();
}

int QCstmEqualization::FineTuning() {

    // The fine tuning reports to a new BarChart but runs on the previous ThlScan
    BarChartProperties cprop_opt;
    cprop_opt.name = "Fine_tuning";
    cprop_opt.min_x = 0;
    cprop_opt.max_x = 511;
    cprop_opt.nBins = 512;
    cprop_opt.color_r = 88;
    cprop_opt.color_g = 86;
    cprop_opt.color_b = 214;

    // The step goes down to 1 here
    ChangeStep(1);

    // Start from the last scan.
    int lastScanIndex = _scans.size() - 1;
    ThlScan * lastScan = nullptr;
    if( lastScanIndex > 0 ) {
        lastScan = _scans[lastScanIndex];
    } else {
        return -1;
    }

    // Use its own previous limits
    lastScan->SetMinScan( lastScan->GetDetectedHighScanBoundary() );
    lastScan->SetMaxScan( lastScan->GetDetectedLowScanBoundary()  );

    lastScan->DoScan( _steeringInfo[0]->currentTHx, _setId++, _steeringInfo[0]->currentDAC_DISC, -1, false, testPulseMode ); // -1: Do all loops
    lastScan->SetScanType( ThlScan::__FINE_TUNING1_SCAN );
    connect( lastScan, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
    lastScan->start();

    return 0;
}

void QCstmEqualization::DAC_Disc_Optimization (int devId, ScanResults * res_100, ScanResults * res_150) {

    ////////////////////////////////////////////////////////////////////////////////////
    // 3) With the results of step 1 and 2 I can obtain the dependency DAC_Disc[L/H](THL)
    double eta = 0., cut = 0.;
    GetSlopeAndCut_IDAC_DISC_THL(res_100, res_150, eta, cut);
    GetSteeringInfo(devId)->currentEta_THx_DAC_Disc = eta;
    GetSteeringInfo(devId)->currentCut_THx_DAC_Disc = cut;

    // -------------------------------------------------------------------------
    // 4) Now IDAC_DISC optimal is such that:
    //    With an adj-bit of 00101[5] the optimal mean is at defaultNoiseEqualisationTarget + 3.2 sigma

    // Desired mean value = defaultNoiseEqualisationTarget + 3.2 sigma
    // Taking sigma from the first scan.
    double meanTHL_for_opt_IDAC_DISC = defaultNoiseEqualisationTarget + 3.2*res_100->sigma;

    // Using the relation DAC_Disc[L/H](THL) we can find the value of DAC_Disc
    GetSteeringInfo(devId)->currentDAC_DISC_OptValue = int(EvalLinear(GetSteeringInfo(devId)->currentEta_THx_DAC_Disc, GetSteeringInfo(devId)->currentCut_THx_DAC_Disc, meanTHL_for_opt_IDAC_DISC));

    // -----------------------
    // Same as (I have checked this) :
    // double slope = (Mean1-Mean0)/(I_DAC_Disc_SecondScan-I_DAC_Disc_FirstScan);
    // int Calculated_I_DAC_Disc = floor(((SigmaTimesValue*Stdev0)-Mean0+(EqualizationTarget))/slope)+I_DAC_Disc_FirstScan;
    // -----------------------

    SetDAC_propagateInGUI(_mpx3gui->GetSpidrController(), devId, GetSteeringInfo(devId)->currentDAC_DISC, GetSteeringInfo(devId)->currentDAC_DISC_OptValue);

    // Show results
    QString statsString = "[";
    statsString += QString::number(devId, 'd', 0);
    statsString += "] Optimal ";
    statsString += GetSteeringInfo(devId)->currentDAC_DISC_String;
    statsString += " = ";
    statsString += QString::number(GetSteeringInfo(devId)->currentDAC_DISC_OptValue, 'd', 0);
    AppendToTextBrowser(statsString);
}

void QCstmEqualization::SaveEqualization(QString path, bool totempdir, bool fetchfromtempdir) {

    QString filenameEqualisation = "";
    QString savepath = path;

    if ( savepath == "" ) {
        //! Get folder to save equalisation files to
        if(!_isRemotePath)
            savepath = QFileDialog::getExistingDirectory(this, tr("Open Directory to save equalisations to"),
                                                         QDir::currentPath(),
                                                         QFileDialog::ShowDirsOnly);
        else
        {
            savepath = _remotePath;
            _isRemotePath = false;
        }
        //! User pressed cancel, offer another go at saving
        if (savepath.isEmpty()){
            QMessageBox::StandardButton reply;
            reply = QMessageBox::warning(this,
                                         tr("Warning"),
                                         tr("Are you sure you do not want to save equalisations and config files?"),
                                         QMessageBox::Save|QMessageBox::Cancel);
            if (reply == QMessageBox::Save) {
                savepath = QFileDialog::getExistingDirectory(this,
                                                             tr("Open Directory to save equalisations to"),
                                                             QDir::currentPath(),
                                                             QFileDialog::ShowDirsOnly);
            } else {
                sig_statusBarAppend(tr("Equalisation not saved, you may save them manually"),"red");
                return;
            }
        }
    } else {
        savepath = path;
        if ( totempdir ) {
            savepath.append( QDir::separator() );
            savepath += "tmp";
            _tempEqSaveDir = savepath;
        }
        // Create the folder if it doesn't exist
        if ( ! QDir( savepath ).exists() ) {
            _tempEqSaveDir = savepath;
            qDebug() << "[INFO] [Equalisation] creating temporary directory : " << savepath;
            QDir().mkdir( savepath );
        }
    }

    savepath.append( QDir::separator() );
    filenameEqualisation = savepath;
    filenameEqualisation.append("config.json"); //! Ie. all chips

    resetThresholds();

    //! Save equalisations with DACs when you run an equalisation
    _mpx3gui->getConfig()->toJsonFile(filenameEqualisation, true);

    unsigned long chipListSize = _workChipsIndx.size();

    //! Save adj and mask path+filename strings and save them
    for ( unsigned long i = 0 ; i < chipListSize ; i++ ) {
        // Binary file
        _eqMap[_workChipsIndx[i]]->WriteAdjBinaryFile( QString( savepath + "adj_" + QString::number(_workChipsIndx[i])) );

        // Masked pixels
        _eqMap[_workChipsIndx[i]]->WriteMaskBinaryFile( QString( savepath + "mask_" + QString::number(_workChipsIndx[i])) );
    }


    if(GetBarChart(0) == nullptr) {
        qDebug() << "[Equalisation] Plot" << 0 << "does not exist. Could not save the equalisation plot :(";
    } else {
        GetBarChart(0)->savePng(savepath + "chip_" + QString::number(0) + ".png", 1024, 1024, 2.0);
    }
    if(GetBarChart(1) == nullptr) {
        qDebug() << "[Equalisation] Plot" << 1 << "does not exist. Could not save the equalisation plot :(";
    } else {
        GetBarChart(1)->savePng(savepath + "chip_" + QString::number(1) + ".png", 1024, 1024, 2.0);
    }
    if(GetBarChart(2) == nullptr) {
        qDebug() << "[Equalisation] Plot" << 2 << "does not exist. Could not save the equalisation plot :(";
    } else {
        GetBarChart(2)->savePng(savepath + "chip_" + QString::number(2) + ".png", 1024, 1024, 2.0);
    }
    if(GetBarChart(3) == nullptr) {
        qDebug() << "[Equalisation] Plot" << 3 << "does not exist. Could not save the equalisation plot :(";
    } else {
        GetBarChart(3)->savePng(savepath + "chip_" + QString::number(3) + ".png", 1024, 1024, 2.0);
    }

    qDebug() << "[Equalisation] Save path: " << savepath;

    // use this when you need to get the rest from a temp dir
    if ( fetchfromtempdir ) {

        // move everything from temp dir to the current savepath(could be just selected by the user)
        // except the config. Only the last one is interesting.
        QString copyfrom = _tempEqSaveDir;
        copyfrom.append( QDir::separator() );
        copyfrom += "adj_*";

        QString copyto = savepath;
        QString command = "cp -f " +copyfrom+" "+copyto;

        // copy adj
        QProcess proc;
        proc.start("sh", QStringList() << "-c" << command);
        proc.waitForFinished(5000);

        copyfrom = _tempEqSaveDir;
        copyfrom.append( QDir::separator() );
        copyfrom += "mask_*";
        command = "cp -f " +copyfrom+" "+copyto;

        // copy masks
        proc.start("sh", QStringList() << "-c" << command);
        proc.waitForFinished(5000);

        copyfrom = _tempEqSaveDir;
        copyfrom.append( QDir::separator() );
        copyfrom += "chip_*";
        command = "cp -f " +copyfrom+" "+copyto;

        proc.start("sh", QStringList() << "-c" << command);
        proc.waitForFinished(5000);
    }
}

void QCstmEqualization::PrepareInterpolation_0x0() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    QString legend = _steeringInfo[0]->currentDAC_DISC_String;
    legend += "_Opt_adj";
    legend += QString::number(_steeringInfo[0]->globalAdj, 'd', 0);

    // arbitrary step for DAC_Disc_Optimization
//    ChangeStep( __DAC_Disc_Optimization_step, true );

    // -------------------------------------------------------------------------
    // 5)  See where the pixels fall now for adj0 and keep the pixel information
    ThlScan * tscan_opt_adj0 = new ThlScan( _mpx3gui, this);
    tscan_opt_adj0->SetMinScan(  );
    tscan_opt_adj0->SetMaxScan(  );

    tscan_opt_adj0->ConnectToHardware(spidrcontrol, spidrdaq);
    BarChartProperties cprop_opt_adj0;
    cprop_opt_adj0.min_x = 0;
    cprop_opt_adj0.max_x = 511;
    cprop_opt_adj0.nBins = 512;
    cprop_opt_adj0.color_r = 88;
    cprop_opt_adj0.color_g = 86;
    cprop_opt_adj0.color_b = 214;

    for ( unsigned long i = 0 ; i < _workChipsIndx.size() ; i++ ) {
        cprop_opt_adj0.name = BuildChartName( _workChipsIndx[i], legend );
        GetBarChart( _workChipsIndx[i] )->AppendSet( cprop_opt_adj0 );
    }

    // Let's assume the mean falls at the equalization target
    //tscan_opt_adj0->SetStopWhenPlateau(true);
    tscan_opt_adj0->DoScan(  _steeringInfo[0]->currentTHx, _setId++, _steeringInfo[0]->currentDAC_DISC, -1, false, false ); // -1: Do all loops
    tscan_opt_adj0->SetWorkChipIndexes( _workChipsIndx, _steeringInfo );

    // Launch as thread.  Connect the slot which signals when it's done
    _scans.push_back( tscan_opt_adj0 );
    _scanIndex++;
    connect( tscan_opt_adj0, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
    tscan_opt_adj0->start();

}

void QCstmEqualization::ScanOnInterpolation() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    QString legend = _steeringInfo[0]->currentDAC_DISC_String;
    legend += "_Opt_adjX";

    // arbitrary step for DAC_Disc_Optimization
    ChangeStep( 1, true );

    // Note that this suggests a descending scan.
    ThlScan * scan_last = nullptr;
    scan_last = _scans[_scanIndex - 1];

    ThlScan * tscan_opt_ext = new ThlScan(_mpx3gui, this);

    if ( _steeringInfo[0]->GetEqualizationTarget() == __default_equalizationtarget ) {
        tscan_opt_ext->SetMinScan( 30 );
        tscan_opt_ext->SetMaxScan( 0 );
        qDebug() << "[INFO]\tUsing fast scanning, hardcoded 30 to 0";
    } else {
        tscan_opt_ext->SetMinScan( scan_last->GetDetectedHighScanBoundary() );
        tscan_opt_ext->SetMaxScan( scan_last->GetDetectedLowScanBoundary() );
    }


    tscan_opt_ext->ConnectToHardware(spidrcontrol, spidrdaq);
    BarChartProperties cprop_opt_ext;
    cprop_opt_ext.min_x = 0;
    cprop_opt_ext.max_x = 511;
    cprop_opt_ext.nBins = 512;
    cprop_opt_ext.color_r = 76;
    cprop_opt_ext.color_g = 217;
    cprop_opt_ext.color_b = 100;

    for ( int i = 0 ; i < int(_workChipsIndx.size()); i++ ) {
        cprop_opt_ext.name = BuildChartName( _workChipsIndx[i], legend );
        GetBarChart( _workChipsIndx[i] )->AppendSet( cprop_opt_ext );
    }

    // Let's assume the mean falls at the equalization target
    tscan_opt_ext->DoScan( _steeringInfo[0]->currentTHx, _setId++, _steeringInfo[0]->currentDAC_DISC, -1, false, false ); // -1: Do all loops
    tscan_opt_ext->SetAdjustmentType( ThlScan::__adjust_to_equalizationMatrix );
    // A global_adj doesn't apply here anymore.  Passing -1.
    tscan_opt_ext->SetWorkChipIndexes( _workChipsIndx, _steeringInfo );

    // Launch as thread.  Connect the slot which signals when it's done
    _scans.push_back( tscan_opt_ext);
    _scanIndex++;
    connect( tscan_opt_ext, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
    tscan_opt_ext->start();
}

void QCstmEqualization::PrepareInterpolation_0x5() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    // arbitrary step for DAC_Disc_Optimization
//    ChangeStep( __DAC_Disc_Optimization_step, true );

    // -------------------------------------------------------------------------
    // 5)  See where the pixels fall now for adj5 and keep the pixel information

    QString legend = _steeringInfo[0]->currentDAC_DISC_String;
    legend += "_Opt_adj";
    legend += QString::number(_steeringInfo[0]->globalAdj, 'd', 0);

    // New limits
    // The previous scan is a complete scan on all pixels.  Now we can cut the scan up to
    //  a few sigmas from the previous scan.  Note that this suggests a descending scan.
    SetMinScan( _scans[_scanIndex - 1]->GetDetectedHighScanBoundary() );
    SetMaxScan( _scans[_scanIndex - 1]->GetDetectedLowScanBoundary() );

    ThlScan * tscan_opt_adj5 = new ThlScan(_mpx3gui, this);
    tscan_opt_adj5->ConnectToHardware(spidrcontrol, spidrdaq);
    BarChartProperties cprop_opt_adj5;
    cprop_opt_adj5.min_x = 0;
    cprop_opt_adj5.max_x = 511;
    cprop_opt_adj5.nBins = 512;
    cprop_opt_adj5.color_r = 255;
    cprop_opt_adj5.color_g = 45;
    cprop_opt_adj5.color_b = 85;

    for ( int i = 0 ; i < int(_workChipsIndx.size()); i++ ) {
        cprop_opt_adj5.name = BuildChartName( _workChipsIndx[i], legend );
        GetBarChart( _workChipsIndx[i] )->AppendSet( cprop_opt_adj5 );
    }

    // Let's assume the mean falls at the equalization target
    //tscan_opt_adj5->SetStopWhenPlateau(true);
    tscan_opt_adj5->DoScan(  _steeringInfo[0]->currentTHx, _setId++, _steeringInfo[0]->currentDAC_DISC, -1, false, false ); // -1: Do all loops
    tscan_opt_adj5->SetWorkChipIndexes( _workChipsIndx, _steeringInfo );

    // Launch as thread.  Connect the slot which signals when it's done
    _scans.push_back( tscan_opt_adj5 ); _scanIndex++;
    connect( tscan_opt_adj5, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
    tscan_opt_adj5->start();

}

void QCstmEqualization::DisplayStatsInTextBrowser(int adj, int dac_disc, ScanResults * res) {

    QString statsString = "[";
    statsString += QString::number(res->chipIndx, 'd', 0);
    statsString += "] Adj=";
    if (adj >= 0) statsString += QString::number(adj, 'd', 0);
    else {
        if (_eqStatus >= __EstimateEqualisationTarget) {
            statsString += "TP";
        } else {
            statsString += "X";
        }
    }

    statsString += " | ";
    statsString += _steeringInfo[0]->currentDAC_DISC_String;
    statsString += QString::number(dac_disc, 'd', 0);
    statsString += " | Mean = ";
    statsString += QString::number(res->weighted_arithmetic_mean, 'f', 1);
    statsString += ", Sigma = ";
    statsString += QString::number(res->sigma, 'f', 1);
    AppendToTextBrowser(statsString);
}

void QCstmEqualization::ScanThreadFinished(){

    // This step was done
    _stepDone[_eqStatus] = true;
    // disconnect the signal that brought us here.  _scanIndex is incremented as the thread is created.
    disconnect( _scans[_scanIndex-1], SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
    // Go to next step, except for fine tuning where I use the same previous scan
    _eqStatus++;

    // 1) Now revisit the equalization. It knows where to pick up.
    // 2) Handel when the equalization has been stopped by the user.
    //    The thread will finish pematurely and then this function gets called.
    if ( _stopEq ) {
        // In this case do the big rewind.
        qDebug() << "[INFO] Equalization stopped --> Rewind.";
        _stopEq = false;
        // Full rewind
        FullEqRewind();
    } else {
        StartEqualization( );
    }
}



double QCstmEqualization::EvalLinear(double eta, double cut, double x){
    return x*eta + cut;
}

void QCstmEqualization::GetSlopeAndCut_IDAC_DISC_THL(ScanResults * r1, ScanResults * r2, double & eta, double & cut) {

    // The slope is =  (THLmean2 - THLmean1) / (DAC_DISC_L_setting_2 - DAC_DISC_L_setting_1)
    eta = (r2->DAC_DISC_setting - r1->DAC_DISC_setting) / (r2->weighted_arithmetic_mean - r1->weighted_arithmetic_mean);
    cut = r2->DAC_DISC_setting - (eta * r2->weighted_arithmetic_mean);

}

void QCstmEqualization::GetSlopeAndCut_Adj_THL(ScanResults * r1, ScanResults * r2, double & eta, double & cut) {

    eta = (r2->global_adj - r1->global_adj) / (r2->weighted_arithmetic_mean - r1->weighted_arithmetic_mean);
    cut = r2->global_adj - (eta * r2->weighted_arithmetic_mean);

}

//! The way the next two functions relate looks redundant but I need
//!    it for when this is called from inside a thread.
void QCstmEqualization::SetAllAdjustmentBits() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    SetAllAdjustmentBits(spidrcontrol);

}

//! Send the configuration to all chips
void QCstmEqualization::SetAllAdjustmentBits(SpidrController * spidrcontrol) {
    int nChips = _mpx3gui->getConfig()->getNDevicesSupported();

    for ( int i = 0 ; i < nChips ; i++ ) {
        // Check if the device is alive
        if ( ! _mpx3gui->getConfig()->detectorResponds( i ) ) {
            continue;
        }

        SetAllAdjustmentBits(spidrcontrol, i);
    }
}

void QCstmEqualization::SetAllAdjustmentBits(SpidrController * spidrcontrol, int devId, int val_L, int val_H) {

    //SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    if( !spidrcontrol ) {
        QMessageBox::information(this, tr("Clear configuration"), tr("The system is disconnected. Nothing to clear.") );
        return;
    }


    // Adjustment bits
    pair<int, int> pix;
    for ( int i = 0 ; i < __matrix_size ; i++ ) {
        pix = XtoXY(i, __array_size_x);
        spidrcontrol->configPixelMpx3rx(pix.first, pix.second, val_L, val_H, testPulseMode ); // 0x1F = 31 is the max adjustment for 5 bits
    }
    spidrcontrol->setPixelConfigMpx3rx( devId );

}

void QCstmEqualization::SetAllAdjustmentBits(SpidrController * spidrcontrol, int chipIndex, bool applymask, bool testbit) {
    // Adj bits
    pair<int, int> pix;

    if( !spidrcontrol || spidrcontrol == nullptr ) {
        QMessageBox::information(this, tr("Clear configuration"), tr("The system is disconnected. Nothing to clear.") );
        return;
    }

    int nChips = _mpx3gui->getConfig()->getNDevicesSupported();
    if ( chipIndex < 0 || chipIndex > nChips - 1) {
        qDebug() << "[ERROR] wrong chip index !";
        return;
    }

    QSet<int> maskedPixels = _eqMap[chipIndex]->GetMaskedPixels();
    int testBitsOn = 0;
    for ( int i = 0 ; i < __matrix_size ; i++ ) {
        pix = XtoXY(i, __array_size_x);
        //qDebug() << _eqMap[chipIndex]->GetPixelAdj(i) << _eqMap[chipIndex]->GetPixelAdj(i, Mpx3EqualizationResults::__ADJ_H);

        bool val;
        if (testbit && !maskedPixels.contains(i)) {
            val = true;
        } else {
            val = false;
        }

        if ( pix.second == 0 ) {
            spidrcontrol->configCtpr( chipIndex, pix.first, int(testbit) );
        }

        //! Test pulses are turned off here if testbit=true isn't passed
        spidrcontrol->configPixelMpx3rx(
                    pix.first,
                    pix.second,
                    _eqMap[chipIndex]->GetPixelAdj(i),
                    _eqMap[chipIndex]->GetPixelAdj(i, Mpx3EqualizationResults::__ADJ_H),
                    val );
        if (val) testBitsOn++;
    }
    if (testbit) {
        spidrcontrol->setCtpr( chipIndex );
        qDebug() << "[INFO]\tPixels with test bits ON :" << testBitsOn;
    }

    //! Note : this masking is only used outside of the equalisation procedure
    //!         Eg. loading equalisation files or masking pixels from the visualisation
    //! It isn't used in the equalisation procedure itself so it's totally independent
    //!   to the test pulse code

    if ( applymask ) {
        // Mask
        //        if(_eqMap[chipIndex]->GetNMaskedPixels2D() > 0){
        //            QSet<QPair<int,int>> tomask = _eqMap[chipIndex]->GetMaskedPixels2D();
        //            QSet<QPair<int,int>>::iterator i = tomask.begin();
        //            QSet<QPair<int,int>>::iterator iE = tomask.end();
        //            qDebug() << "[INFO] Masking pixels :";
        //            for ( ; i != iE ; i++ ) {
        //                spidrcontrol->setPixelMaskMpx3rx((*i).first, (*i).second, true);
        //                qDebug() << "     chipID:" << chipIndex << " | " <<(*i).first << "," << (*i).second;
        //            }

        if ( _eqMap[chipIndex]->GetNMaskedPixels() > 0 ) {
            QSet<int> tomask = _eqMap[chipIndex]->GetMaskedPixels();
            QSet<int>::iterator i = tomask.begin();
            QSet<int>::iterator iE = tomask.end();
            pair<int, int> pix;
            qDebug() << "[INFO] Masking pixels :";
            resetMaskMatrix(chipIndex);
            for ( ; i != iE ; i++ ) {
                pix = XtoXY( (*i), __matrix_size_x );
                int xToXy = XYtoX(pix.first, pix.second, _mpx3gui->getDataset()->x());
                qDebug() << "     chipID:" << chipIndex << " | " << pix.first << "," << pix.second << " | " << xToXy;
                spidrcontrol->setPixelMaskMpx3rx(pix.first, pix.second, true);
                QPoint previewIndx = chipIndexToPreviewIndex(QPoint(pix.first,pix.second),chipIndex);
                if(previewIndx.x() >= 0 && previewIndx.x() < 512 && previewIndx.y() >= 0 && previewIndx.y() < 512)
                    maskMatrix[previewIndx.x()][previewIndx.y()] = true;
            }
        } else {
            //! When the mask is empty go ahead and unmask all pixels
            for ( int i = 0 ; i < __matrix_size ; i++ ) {
                pix = XtoXY(i, __array_size_x);
                spidrcontrol->setPixelMaskMpx3rx(pix.first, pix.second, false);
            }
        }
    }

    spidrcontrol->setPixelConfigMpx3rx( chipIndex );

    _mpx3gui->getVisualization()->onPixelsMasked(chipIndex, GetEqualizationResults(chipIndex)->GetMaskedPixels());
}

void QCstmEqualization::ClearAllAdjustmentBits(int devId) {

    // Clear all data structures
    _eqMap[devId]->ClearAdj();
    _eqMap[devId]->ClearMasked();
    _eqMap[devId]->ClearReactiveThresholds();

    SetAllAdjustmentBits();
}

void QCstmEqualization::Configuration(int devId, int THx, bool reset) {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    // Reset pixel configuration
    if ( reset ) spidrcontrol->resetPixelConfig();

    SetAllAdjustmentBits(spidrcontrol, devId, 0x0, 0x0);

    auto config = _mpx3gui->getConfig();

    //! ------------------------------------------------------------------------
    //! OMR bit
    //! Force operation mode to sequential, continuous may be implemented elsewhere eventually
    spidrcontrol->setContRdWr( _deviceIndex, false );

    //! ------------------------------------------------------------------------
    //! OMR bit
    //! True  : Holes collection (positive polarity)
    //! False : Electron collection (negative polarity)
    spidrcontrol->setPolarity( _deviceIndex, config->getPolarity() );
    qDebug() << "[Equalisation]\tPolarity = " << config->getPolarity() << "\tTrue = + False = -";

    //! ------------------------------------------------------------------------
    //! OMR bit for Equalization
    //! Must always be true since we are equalising a chip
    //! Make it false when done?
    spidrcontrol->setEqThreshH( devId, true );

    //    //! ------------------------------------------------------------------------
    //    //! OMR bit
    //    //! False : default
    //    //! True  : if doing a test pulse based equalisation
    //    spidrcontrol->setInternalTestPulse( devId, testPulseMode );
    //    qDebug() << "[Equalisation]\tSPIDR Internal Test pulses = " << testPulseMode;

    //! ------------------------------------------------------------------------
    //! OMR bit
    //! Use colour mode depending on GUI
    //! True  : Colour mode
    //! False : Fine pitch (FPM)
    //! Always set it to FPM
    spidrcontrol->setColourMode( devId, config->getColourMode() );
    qDebug() << "[Equalisation]\tColour mode = " << config->getColourMode();

    //! ------------------------------------------------------------------------

    //! OMR bit
    //! 0 : Single pixel mode
    //! 1 : Charge summing mode
    spidrcontrol->setCsmSpm( devId, config->getCsmSpm() );

    qDebug() << "[Equalisation]\tCsm_Spm = " << config->getCsmSpm();

    //! Set gainMode based on if this is a test pulse scan or noise
    if (testPulseMode) {
        gainMode = config->getGainMode();
    } else {
        //! ALWAYS SLGM FOR NOISE EQUALISATIONS
        //! DO NOT CHANGE THIS!
        //! IT DOES NOT MATTER IF IT IS CSM OR SPM
        gainMode = 3; // SLGM
    }

    spidrcontrol->setGainMode( devId, gainMode ); //! SLGM for equalisation
    //! ALWAYS for noise equalisations
    qDebug() << "[Equalisation]\tGain mode = " << gainMode << "\tAlways 3 (SLGM) for noise based equalisations";

    //! Important defaults
    spidrcontrol->setPixelDepth( devId, 12 );
    _mpx3gui->GetSpidrDaq()->setBothCounters(false);

    //! -------------------------------------------------------------------------

    //! OMR bit
    //! When equalizing the high thresholds
    if ( THx == MPX3RX_DAC_THRESH_1 || THx == MPX3RX_DAC_THRESH_3 || THx == MPX3RX_DAC_THRESH_5 || THx == MPX3RX_DAC_THRESH_7 ) {
        spidrcontrol->setDiscCsmSpm( devId, 1 );		//! Use DiscH
        qDebug() << "[Equalisation]\tDisc_Csm_Spm = " << "DiscH";
    } else {
        spidrcontrol->setDiscCsmSpm( devId, 0 );		//! Use DiscL
        qDebug() << "[Equalisation]\tDisc_Csm_Spm = " << "DiscL";
    }

    //! -------------------------------------------------------------------------
    // Trigger config
    // Sequential R/W
    int trig_mode      = SHUTTERMODE_AUTO;     // Auto-trigger mode
    int trig_length_us = 1000;  // This time shouldn't be longer than the period defined by trig_freq_hz
    int trig_freq_mhz   = 10000;   //
    int nr_of_triggers = _nTriggers;    // This is the number of shutter open I get
    spidrcontrol->setShutterTriggerConfig( trig_mode,
                                           trig_length_us,
                                           trig_freq_mhz,
                                           nr_of_triggers );

}

Mpx3EqualizationResults * QCstmEqualization::GetEqualizationResults(int chipIndex) {
    int nChips = _mpx3gui->getConfig()->getNDevicesSupported();
    if ( chipIndex < 0 || chipIndex > nChips - 1) return nullptr;

    return _eqMap[chipIndex];
}

void QCstmEqualization::InitializeEqualizationStructure(){

    int nChips = _mpx3gui->getConfig()->getNDevicesSupported();

    for(int i = 0 ; i < nChips ; i++) {

        if ( ! _mpx3gui->getConfig()->detectorResponds( i ) ) continue;

        // save working indexes
        _workChipsIndx.push_back( i );

        // build the results
        _eqMap[ i ] = new Mpx3EqualizationResults;

    }

}

void QCstmEqualization::InitializeBarChartsAdjustements(){

    // The one built in the constructor will be erased here
    _ui->horizontalLayoutEqHistos->removeWidget( _chart[0] );
    _chart.clear();

    int nChips = _mpx3gui->getConfig()->getNDevicesSupported();
    for(int i = 0 ; i < nChips ; i++) {

        if ( ! _mpx3gui->getConfig()->detectorResponds( i ) ) continue;

        // I need a number of objects to draw the adjustments low and high
        for(int j = 0 ; j < 2 ; j++) { // low and high

            QString barChartName = "histoWidget_";
            barChartName += QString::number(_workChipsIndx[0], 'd', 0);
            barChartName += "_";
            barChartName += QString::number(j%2, 'd', 0); // low or high

            BarChart * nbc = new BarChart( GetUI()->layoutWidget );
            nbc->setObjectName(barChartName);
            nbc->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );

            QString title = "[";
            title += QString::number(i, 'd', 0);
            BarChartProperties cprop;

            if ( j%2 == 0 ) {

                title += "] THL";

                cprop.name = title.toStdString();
                cprop.xAxisLabel = "adj";
                cprop.yAxisLabel = "entries";
                cprop.min_x = 0;
                cprop.max_x = 31;
                cprop.nBins = 32;
                cprop.color_r = 0;
                cprop.color_g = 122;
                cprop.color_b = 255;

                nbc->AppendSet( cprop );

                _adjchart_L.push_back( nbc );

            } else if ( j%2 == 1 ) {

                title += "] THH";

                cprop.name = title.toStdString();
                cprop.xAxisLabel = "adj";
                cprop.yAxisLabel = "entries";
                cprop.min_x = 0;
                cprop.max_x = 31;
                cprop.nBins = 32;
                cprop.color_r = 255;
                cprop.color_g = 59;
                cprop.color_b = 48;

                nbc->AppendSet( cprop );

                _adjchart_H.push_back( nbc );
            }

        }
    }
}

void QCstmEqualization::DistributeAdjHistogramsInGridLayout(){
    int chipListSize = int(_workChipsIndx.size());

    int nCols = int(ceil( double(chipListSize / 2.)));

    // I am generating the sequence row,col that matches the locations of the chips in the board
    //  -----------------
    //  - chip0 | chip1 -
    //  - chip3 | chip2 -
    //  -----------------

    int indx = 0, row = 0;
    while ( indx < chipListSize ) {

        int col = row;
        int colCntr = 0;
        while ( colCntr < nCols ) {

            col = ( col % nCols );

            BarChart * bc = GetAdjBarChart( indx, _equalizationShow );
            if ( bc != nullptr ) { // i might be asking for a chart that is not available 'cause a chip is unresponsive
                _gridLayoutHistograms->addWidget( bc , row, col );
                bc->show();
            }

            indx++;
            colCntr++;
            col++;
        }
        row++;
    }

    _gridLayoutHistograms->update();
}

void QCstmEqualization::setWindowWidgetsStatus(win_status s)
{
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

bool QCstmEqualization::estimate_V_TP_REF_AB(uint electrons, bool /*makeDialog*/)
{
    const double e_dividedBy_c_test = 3.20435324e-5;
    const double requestedInjectionVoltage = 0.3 + (electrons * e_dividedBy_c_test);

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    if ( requestedInjectionVoltage < 0.3 || requestedInjectionVoltage >= 1.20 ) {
        qDebug() << "[FAIL]\tRequested injection voltage out of range";
        return false;
    }
    if ( requestedInjectionVoltage >= 1.15 ) {
        qDebug() << "[WARN]\tThis could fail, try setting a lower value";
    }

    qDebug() << "[INFO]\tTest pulse equalisation --> Requested injection voltage for TP_REF_A and TP_REF_B:" << requestedInjectionVoltage;

    //! Make a modal progress bar as well for visual updates
    int totalDACsToSet = _mpx3gui->getConfig()->getNActiveDevices() * 3;
    int numberOfDACsSet = 0;
    QProgressDialog progress("Setting test pulse DACs...", QString(), 0, totalDACsToSet, this);
    progress.setModal(true);
    progress.setMinimum(0);
    progress.setMaximum( totalDACsToSet );
    progress.show();

    //! We need to scan for these
    for (int chipID=0; chipID < _mpx3gui->getConfig()->getNActiveDevices(); chipID++) {
        qApp->processEvents();

        setDACToVoltage(chipID, MPX3RX_DAC_TP_REF, double(0.3));
        numberOfDACsSet++;
        progress.setValue( numberOfDACsSet );
        qApp->processEvents();

        setDACToVoltage(chipID, MPX3RX_DAC_TP_REF_A, requestedInjectionVoltage);
        numberOfDACsSet++;
        progress.setValue( numberOfDACsSet );
        qApp->processEvents();

        setDACToVoltage(chipID, MPX3RX_DAC_TP_REF_B, requestedInjectionVoltage);
        numberOfDACsSet++;
        progress.setValue( numberOfDACsSet );
        qApp->processEvents();

        //! Always just set these to defaults
        SetDAC_propagateInGUI(spidrcontrol, chipID, MPX3RX_DAC_TP_BUF_IN, 128);
        SetDAC_propagateInGUI(spidrcontrol, chipID, MPX3RX_DAC_TP_BUF_OUT, 4);
    }
    progress.setValue( totalDACsToSet );

    return true;
}

uint QCstmEqualization::setDACToVoltage(int chipID, int dacCode, double V)
{
    uint dac_val = 0;
    bool foundTarget = false;
    int adc_val = 0;
    double adc_volt = 0;
    int nSamples = 1;
    double lowerVboundary = V * 0.99;
    double upperVboundary = V * 1.01;

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    //! Give them some reasonable starting values to save time
    //! Get these from this existing values in the config unless I know better ;)
    if (dacCode == MPX3RX_DAC_TP_REF)   dac_val = 60;
    if (dacCode == MPX3RX_DAC_TP_REF_A) dac_val = _mpx3gui->getConfig()->getDACValue(chipID, MPX3RX_DAC_TP_REF_A-1);
    if (dacCode == MPX3RX_DAC_TP_REF_B) dac_val = _mpx3gui->getConfig()->getDACValue(chipID, MPX3RX_DAC_TP_REF_B-1);

    while (!foundTarget) {
        if (dac_val >= 511) {
            qDebug() << "[FAIL]\tReached maximum DAC value, cannot find target. Set to maximum";
            dac_val = 511;
            foundTarget = true;

        }

        spidrcontrol->setDac(chipID, dacCode, int(dac_val));
        spidrcontrol->setSenseDac(chipID, dacCode);
        spidrcontrol->getDacOut(chipID, &adc_val, nSamples);

        adc_val /= nSamples;
        adc_volt = (__voltage_DACS_MAX/(double)__maxADCCounts) * (double)adc_val;

        //qDebug() << "adc_volt :" << adc_volt;

        if ( adc_volt <= lowerVboundary ) {
            if ( adc_volt < V*0.8 ) {
                dac_val += 5;
            } else {
                dac_val += 1;
            }
        } else if (adc_volt >= upperVboundary) {
            if ( adc_volt > V*1.2 ) {
                dac_val -= 5;
            } else {
                dac_val -= 1;
            }
        } else if ( adc_volt >= lowerVboundary && adc_volt <= upperVboundary ) {
            foundTarget = true;
        } else {
            qDebug() << "[FAIL]\tFix this... Could not find dac value to match given voltage within 1%" << dac_val << V;

            qDebug() << "\t Increasing tolerance to 5% and scanning again";
            lowerVboundary = V * 0.95;
            upperVboundary = V * 1.05;
        }
    }

    SetDAC_propagateInGUI(spidrcontrol, chipID, dacCode, int(dac_val));

    qDebug() << "[INFO]\tFound DAC value with voltage: " << dacCode << dac_val << adc_volt;

    return dac_val;
}

bool QCstmEqualization::initialiseTestPulses(SpidrController * spidrcontrol)
{
    if ( ! _mpx3gui->getConfig()->isConnected() ) {
        QMessageBox::warning ( this, tr("Activate Test Pulses"), tr( "No device connected." ) );
        return false;
    }

    //! Very basic error check
    if (spidrcontrol == nullptr) return false;

    //! Set Test Pulse frequency (millihertz!) Eg. --> 40000 * 25 ns = 1 ms = 1000 Hz
    //! Set Pulse width: 400 --> 10 us default
    spidrcontrol->setTpFrequency(true, int(testPulseEqualisationDialog->getTestPulsePeriod()), int(testPulseEqualisationDialog->getTestPulseLength()));

    //! Set some SPIDR registers, these should match the setTpFrequency call
    spidrcontrol->setSpidrReg(0x10C0, int(testPulseEqualisationDialog->getTestPulseLength()), true);
    spidrcontrol->setSpidrReg(0x10BC, int(testPulseEqualisationDialog->getTestPulsePeriod()), true);

    return true;
}

bool QCstmEqualization::activateTestPulses(SpidrController * spidrcontrol, int chipID, int offset_x, int offset_y, int * maskedPixels)
{
    pair<int, int> pix;
    bool testbit = false;
    int testBitsOn = 0;

    //! Turn test pulse bit on for that chip
    spidrcontrol->setInternalTestPulse(chipID, true);

    uint pixelSpacing = testPulseEqualisationDialog->getPixelSpacing();

    for ( int i = 0; i < __matrix_size; i++ ) {
        pix = _mpx3gui->XtoXY(i, __array_size_x);

        //! Unmask all pixels that we are going to inject test pulses into.
        //! --> mask all pixels that we aren't using

        if ( (pix.first + offset_x) % pixelSpacing == 0 && (pix.second + offset_y) % pixelSpacing == 0 ) {
            testbit = true;
            testBitsOn++;
            spidrcontrol->setPixelMaskMpx3rx(pix.first, pix.second, false);
            //qDebug() << "[TEST PULSES] Config CTPR on (x,y): (" << pix.first << "," << pix.second << ")";
        } else {
            testbit = false;
            spidrcontrol->setPixelMaskMpx3rx(pix.first, pix.second, true);
        }

        if ( pix.second == 0 ) {
            spidrcontrol->configCtpr( chipID, pix.first, 1 );
        }

        spidrcontrol->configPixelMpx3rx(pix.first,
                                        pix.second,
                                        _eqMap[chipID]->GetPixelAdj(i),
                                        _eqMap[chipID]->GetPixelAdj(i, Mpx3EqualizationResults::__ADJ_H),
                                        testbit);
    }
    spidrcontrol->setCtpr( chipID );

    //qDebug() << "[TEST PULSES] CTPRs set on chip" << chipID;
    qDebug() << "[TEST PULSES] Number of pixels testBit ON :"<< testBitsOn;

    if ( ! spidrcontrol->setPixelConfigMpx3rx( chipID ) ) {
        qDebug() << "[FAIL]\tCould not set pixel config... FIX ME";
        return false;
    }

    *maskedPixels = __matrix_size - testBitsOn;

    return true;
}

//! TODO New and untested feature so you can run multiple equalisations without closing the program
//! Doesn't work...
void QCstmEqualization::resetForNewEqualisation()
{
    _eqStatus = __INIT;
    _scanIndex = 0;

    _maxScanTHL = 0;
    _minScanTHL = _firstMinScanTHL; //! This is needed really for noisy sensor/chip combinations, like GaAs MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_0].dflt;
    _scanDescendant = true;
    _busy = false;

    _eqMap.clear();
    _workChipsIndx.clear();

    _chart.clear();
    BarChart * nbc = new BarChart( GetUI()->layoutWidget );
    nbc->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );
    _chart.push_back( nbc ); // set as parent the same as the one delivered in the UI
    _ui->horizontalLayoutEqHistos->addWidget( nbc );
    nbc->setHidden(true);
}

void QCstmEqualization::estimateEqualisationTarget()
{
    //! If test pulses are being used, then we want to start a the test pulse scanning procedure.
    //! Otherwise, set it to the default value


    if (testPulseMode) {
        //! Activate test pulses with the configuration from the GUI or the defaults

        SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
        SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

        initialiseTestPulses(spidrcontrol);
        qDebug() << "[INFO]\tInitialised test pulse mode for equalisation threshold scanning";

        QString legend = _steeringInfo[0]->currentDAC_DISC_String;
        legend += "_Opt_testPulses";

        //! New limits
        SetMinScan( 511 );
        SetMaxScan( 0 );

        ChangeStep( 1 );

        ThlScan * tscan_opt_testPulses = new ThlScan(_mpx3gui, this);
        tscan_opt_testPulses->ConnectToHardware(spidrcontrol, spidrdaq);
        BarChartProperties cprop_opt_testPulses;
        cprop_opt_testPulses.min_x = 0;
        cprop_opt_testPulses.max_x = 511;
        cprop_opt_testPulses.nBins = 512;
        cprop_opt_testPulses.color_r = 255;
        cprop_opt_testPulses.color_g = 149;
        cprop_opt_testPulses.color_b = 0;

        for ( unsigned long i = 0 ; i < _workChipsIndx.size() ; i++ ) {
            cprop_opt_testPulses.name = BuildChartName( _workChipsIndx[i], legend );
            GetBarChart( _workChipsIndx[i] )->AppendSet( cprop_opt_testPulses );
        }

        //tscan_opt_testPulses->SetStopWhenPlateau(true);
        //! Note the last flag here
        tscan_opt_testPulses->DoScan(  _steeringInfo[0]->currentTHx, _setId++, _steeringInfo[0]->currentDAC_DISC, -1, false, true );
        // -1: Do all loops
        tscan_opt_testPulses->SetAdjustmentType( ThlScan::__adjust_to_equalizationMatrix );
        tscan_opt_testPulses->SetWorkChipIndexes( _workChipsIndx, _steeringInfo );

        // Launch as thread.  Connect the slot which signals when it's done
        _scans.push_back( tscan_opt_testPulses );
        _scanIndex++;
        connect( tscan_opt_testPulses, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
        tscan_opt_testPulses->start();

    } else {
        //! Tell the state machine we've done a scan, oooooh cheeky
        qDebug() << "[INFO]\tEqualisation target left at default value";
        ScanThreadFinished();
    }

    return;
}

void QCstmEqualization::resetMaskMatrix(int chipid) {

    if(chipid == 0)
    {
        for(int i = 256; i <512; i++){
            for(int j = 256; j <512; j++){
                maskMatrix[i][j] = false;
            }
        }
    }
    if(chipid == 1)
    {
        for(int i = 256; i <512; i++){
            for(int j = 0; j <256; j++){
                maskMatrix[i][j] = false;
            }
        }
    }
    if(chipid == 2)
    {
        for(int i = 0; i <256; i++){
            for(int j =0 ; j <256; j++){
                maskMatrix[i][j] = false;
            }
        }
    }
    if(chipid == 3)
    {
        for(int i = 0; i <256; i++){
            for(int j = 256 ; j <512; j++){
                maskMatrix[i][j] = false;
            }
        }
    }

}

QPoint QCstmEqualization::chipIndexToPreviewIndex(QPoint chipIndex, int chipId)
{
    const int COL_SIZE = 512 , ROW_SIZE = 512;
    QPoint previewIndex;
    if(chipId == 0){
        previewIndex.setX(ROW_SIZE - 1 - chipIndex.y());
        previewIndex.setY(COL_SIZE - 1 - chipIndex.x());
    }
    else if(chipId == 1){ //needto be fixed
        previewIndex.setX((ROW_SIZE/2) - 1 - chipIndex.y());
        previewIndex.setY(COL_SIZE - 1 - chipIndex.x());
    }
    else if(chipId == 2){
        previewIndex.setX(chipIndex.y());
        previewIndex.setY(chipIndex.x());
    }
    else if(chipId == 3){//needto be fixed
        previewIndex.setX(chipIndex.x() + (ROW_SIZE/2));
        previewIndex.setY(chipIndex.y());
    }
    return previewIndex;
}

bool QCstmEqualization::makeTeaCoffeeDialog()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Equalisation information");
    msgBox.setText("This operation will take a long time, get some tea/coffee.\n\nIMPORTANT: Check polarity and activate bias voltage!");
    msgBox.addButton(QMessageBox::Ok);
    msgBox.addButton(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setStyleSheet("QPushButton { \
                         background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 rgb(0,127,255), stop:1 rgb(0, 110, 200)); \
            font : 16px 'Helvetica Neue'; \
    border-radius : 10px; \
padding : 5px; \
margin : 3px; \
color : white; \
}\
QPushButton:hover { \
color : #f9f9f9; \
border : 1px solid transparent; \
font : bold; \
} \
QPushButton:pressed { \
    background-color : #007acc; \
} \
");

if (msgBox.exec() == QMessageBox::Ok){
    return true;
} else {
return false;
}
}

void QCstmEqualization::LoadEqualization(bool getPath, bool remotely, QString path) {

    QCoreApplication::processEvents();

    if (path == "" || path == "/") {
        path = "config/";
    } //! Else leave it

    //! Non-default path, get the folder from a QFileDialog
    if (getPath){
        //! Absolute folder path
        if(remotely) {
            path += "/";
        } else {
            path = QFileDialog::getExistingDirectory(
                        this,
                        tr("Select a directory containing equalisation files (adj_* and mask_*)"),
                        QDir::currentPath(),
                        QFileDialog::ShowDirsOnly);
            path += "/";
        }

        if( path.isNull() || path == "/" ) {
            //! Nothing selected, return with no prompt or warning
            qDebug() << "[WARN]\tNull path input, doing nothing.";
            return;

        } else {
            //! Very basic check to proceed or not by seeing if the bare minimum files exist (adj_0 and mask_0) in the given path
            QString testAdjFile = path + QString("adj_0");
            QString testMaskFile = path + QString("mask_0");

            if (!(QFileInfo::exists(testAdjFile) && QFileInfo::exists(testMaskFile))){

                //! Failed to find adj_0 and mask_0, the bare minimum for this to work
                //! Return with no warnings or prompts

                emit sig_statusBarAppend(tr("Nothing loaded from equalisation"), "orange");
                qDebug() << "[INFO]\tNothing loaded from equalisation, doing nothing.";
                return;
            }
        }
    }

    emit equalizationPathExported(path);

    int nChips = _mpx3gui->getConfig()->getNDevicesSupported();
    QCoreApplication::processEvents();
    QProgressDialog pd(tr("Loading adjustment bits..."), tr("Cancel"), 0, nChips, this);
    pd.setCancelButton( nullptr ); // No cancel button
    pd.setWindowModality(Qt::WindowModal);
    pd.setMinimumDuration( 0 ); // show immediately
    pd.setWindowTitle(tr("Load equalisation"));
    pd.setValue( 0 );

    _ui->equalizationSelectTHLTHHCombo->setEnabled( true );

    //! Initialise the data structures necessary to hold the equalization for all the chips
    InitializeEqualizationStructure();

    //! Get the BarCharts in place
    InitializeBarChartsAdjustements();

    //! Part 1: Send equalisation loaded from ... to mpx3gui status bar
    //! Initialise string with prefix
    QString msg = QString(tr("Equalisations loaded from: "));

    for(int i = 0 ; i < nChips ; i++) {
        QCoreApplication::processEvents();

        //! Check if the device is alive
        if ( ! _mpx3gui->getConfig()->detectorResponds( i ) ) {
            continue;
        }

        //! Build strings for adj and mask file paths
        QString adjfn = path + "adj_";
        adjfn += QString::number(i, 10);
        QString maskfn = path + "mask_";
        maskfn += QString::number(i, 10);

        //! Part 2.1: Send equalisation loaded from ... to mpx3gui status bar
        //! Append new equalisation paths
        if (!getPath){
            //! Default path
            msg += adjfn + QString(" ") + maskfn + QString(" ");
        }

        if ( ! _eqMap[i]->ReadAdjBinaryFile( adjfn ) ) {
            QMessageBox::warning(this, tr("Loading equalisation"), tr("Failed. Can not open file: %1").arg(adjfn) );
            return;
        }
        if ( ! _eqMap[i]->ReadMaskBinaryFile( maskfn ) ) {
            QMessageBox::warning(this, tr("Loading equalisation"), tr("Failed. Can not open file: %1").arg(adjfn) );
            return;
        }

        _equalizationLoaded = true;

        // And talk to the hardware loading also the mask
        SetAllAdjustmentBits( _mpx3gui->GetSpidrController(), i, true );

        // Progression
        pd.setValue( i+1 );

    }


    //! Part 2.2: Send equalisation loaded from ... to mpx3gui status bar
    //! Append new equalisation path
    if (getPath){
        //! Just print the folder that files were loaded from
        msg += path;
    }

    //! Part 3: Send equalisation loaded from ... to mpx3gui status bar
    //! Actually send the equalisation data to the status bar
    //emit sig_statusBarClean();
    qDebug() << msg;
    emit sig_statusBarAppend("Equalisation loaded", "green");
    //emit sig_statusBarAppend(msg, "green");

    ShowEqualization( _equalizationShow );

}

void QCstmEqualization::ShowEqualization(Mpx3EqualizationResults::lowHighSel sel) {

    // Use a data set to put the adj matrixes together
    if ( ! _resdataset ) _resdataset = new Dataset ( __matrix_size_x, __matrix_size_y, _nchipsX*_nchipsY );
    _resdataset->clear();

    int nChips = _mpx3gui->getConfig()->getNDevicesSupported();

    for(int i = 0 ; i < nChips ; i++) {

        // Check if the device is alive
        if ( ! _mpx3gui->getConfig()->detectorResponds( i ) ) {
            continue;
        }

        // Get adj matrix for one chip
        int * adj_matrix = _eqMap[i]->GetAdjustementMatrix( sel );
        // Stack
        _resdataset->setFrame(adj_matrix, i, 0);
    }

    // Once the frame is complete, extract info
    int * fulladjdata = _resdataset->getLayer( 0 );
    // Plot
    UpdateHeatMap(fulladjdata, _fullsize_x, _fullsize_y);
    _ui->_intermediatePlot->replot( QCustomPlot::rpQueued );

}

void QCstmEqualization::SetupSignalsAndSlots() {

    connect(_ui->nHitsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setNHits(int)) );

    connect( _ui->_startEq, SIGNAL(clicked()), this, SLOT(StartEqualizationSingleChip()) );
    connect( _ui->_startEqAll, SIGNAL(clicked()), this, SLOT(StartEqualizationAllChips()) );
    connect( _ui->_startEqAllSequential, SIGNAL(clicked()), this, SLOT(StartEqualizationSequentialSingleChips()) );
    connect( _ui->_stopEq, SIGNAL(clicked()), this, SLOT(StopEqualization()) );

    // Spinboxes
    connect( _ui->nTriggersSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeNTriggers(int) ) );
    connect( _ui->devIdSpinBox, SIGNAL(valueChanged(int)), this, SLOT(  ChangeDeviceIndex(int) ) );
    connect( _ui->spacingSpinBox, SIGNAL(valueChanged(int)), this, SLOT(  ChangeSpacing(int) ) );

    connect( _ui->eqMinSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeMin(int) ) );
    connect( _ui->eqMaxSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeMax(int) ) );
    connect( _ui->eqStepSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeStep(int) ) );

    connect( _ui->fineTuningLoopsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setFineTuningLoops(int)) );

    connect( _ui->equalizationTHLTHHCombo, SIGNAL(activated(int)), this, SLOT(setEqualizationTHLTHH(int)) );
    connect( _ui->equalizationTypeCombo, SIGNAL(activated(int)), this, SLOT(setEqualizationTHLType(int)) );
    connect( _ui->equalizationSelectTHLTHHCombo, SIGNAL(activated(int)), this, SLOT(setEqualizationShowTHLTHH(int)) );
}

void QCstmEqualization::setEqualizationShowTHLTHH(int sel) {

    if( int(_equalizationShow == sel)) {
        ShowEqualization(_equalizationShow);
        return;
    }

    // see the order in the constructor
    if ( sel == 0 ) _equalizationShow = Mpx3EqualizationResults::__ADJ_L;
    if ( sel == 1 ) _equalizationShow = Mpx3EqualizationResults::__ADJ_H;

    // and show again
    ShowEqualization(_equalizationShow);
}

void QCstmEqualization::setEqualizationTHLTHH(int sel) {
    if ( _equalizationCombination == sel ) return;
    _equalizationCombination = sel;
    _prevEqualizationCombination = sel;
}

void QCstmEqualization::setEqualizationTHLType(int sel) {
    if ( _equalizationType == sel ) return;
    _equalizationType = sel;
}

void QCstmEqualization::ChangeNTriggers( int nTriggers ) {
    if( nTriggers < 0 ) return; // can't really happen cause the SpinBox has been limited
    _nTriggers = nTriggers;
}

void QCstmEqualization::ChangeDeviceIndex( int index, bool uisetting ) {
    if( index < 0 ) return; // can't really happen cause the SpinBox has been limited
    _deviceIndex = index;
    if( uisetting ) _ui->devIdSpinBox->setValue( _deviceIndex );
}

void QCstmEqualization::ChangeSpacing( int spacing ) {
    if( spacing < 0 ) return;
    _spacing = spacing;
}

void QCstmEqualization::ChangeMin(int min, bool uisetting) {

    // Handle the mistakes
    // 1) Negative value
    // 2) min and max equal
    // 3) not enough range for a scan
    if( min < 0 || min == _maxScanTHL || qAbs(min - _maxScanTHL) <= _stepScan ) {
        _ui->eqMinSpinBox->setValue( _minScanTHL );
        return;
    }

    _minScanTHL = min;

    if ( uisetting ) _ui->eqMinSpinBox->setValue( _minScanTHL );

    // decide if the scan is descendant
    if ( _minScanTHL > _maxScanTHL ) _scanDescendant = true;
    else _scanDescendant = false;

}
void QCstmEqualization::ChangeMax(int max, bool uisetting) {

    if( max < 0 || max == _minScanTHL || qAbs(max - _minScanTHL) <= _stepScan ) {
        _ui->eqMaxSpinBox->setValue( _maxScanTHL );
        return;
    }

    _maxScanTHL = max;

    if ( uisetting ) _ui->eqMaxSpinBox->setValue( _maxScanTHL );

    // decide if the scan is descendant
    if ( _maxScanTHL < _minScanTHL ) _scanDescendant = true;
    else _scanDescendant = false;

}

void QCstmEqualization::ChangeStep(int step, bool uisetting) {

    if( step < 0 ) return;

    _stepScan = step;

    if ( uisetting ) _ui->eqStepSpinBox->setValue( _stepScan );
}

void QCstmEqualization::StopEqualization() {
    qDebug() << "[INFO] attempting to stop the equalization ...";
    _stopEq = true;
    emit stop_data_taking_thread();
}

void QCstmEqualization::ConnectionStatusChanged(bool conn) {
    if ( conn ) {
        setWindowWidgetsStatus( win_status::connected );

        //! Select first device ID in case of a single chip equalisation
        QVector<int> activeDevices = _mpx3gui->getConfig()->getActiveDevices();
        _ui->devIdSpinBox->setValue( activeDevices.at( 0 ) );

    } else {
        setWindowWidgetsStatus( win_status::disconnected );
    }
}

void QCstmEqualization::AppendToTextBrowser(QString s){
    _ui->eqTextBrowser->append( s );
}

void QCstmEqualization::ClearTextBrowser(){
    _ui->eqTextBrowser->clear();
}

void QCstmEqualization::PrintFraction(int * buffer, int size, int first_last) {
    cout << "< ";
    for(int i = 0 ; i < first_last ; i++) {
        cout << buffer[i];
        if(i<first_last-1) cout << ", ";
    }
    cout << " . . . ";
    for(int i = size-1-first_last ; i < size ; i++) {
        cout << buffer[i];
        if(i<size-1) cout << ", ";
    }
    cout << " >" << endl;
}

int QCstmEqualization::GetNPixelsActive(int * buffer, int size, verblev verbose) {

    int NPixelsActive = 0;

    // 4 bits per pixel
    int nPixels = size/4;
    for(int i = 0 ; i < nPixels ; i++) {
        pair<int, int> pix = XtoXY(i, __matrix_size_x);
        if(buffer[i] != 0) {
            if ( verbose == __VERBL_DEBUG ) {
                printf("(%d,%d) counts: %d \n",pix.first, pix.second, buffer[i]);
            }
            NPixelsActive++;
        }
    }

    return NPixelsActive;
}

pair<int, int> QCstmEqualization::XtoXY(int X, int dimX){
    return make_pair(X % dimX, X/dimX);
}

void QCstmEqualization::SetMinScan(int min) {

    // default value
    if( min == -1 ) {
        _minScanTHL = _ui->eqMinSpinBox->value();
        return;
    }

    _minScanTHL = min;
    _ui->eqMinSpinBox->setValue( _minScanTHL );
}

void QCstmEqualization::SetMaxScan(int max) {

    // default value
    if( max == -1 ) {
        _maxScanTHL = _ui->eqMaxSpinBox->value();
        return;
    }

    _maxScanTHL = max;
    _ui->eqMaxSpinBox->setValue( _maxScanTHL );
}

Mpx3EqualizationResults::Mpx3EqualizationResults(){

}

Mpx3EqualizationResults::~Mpx3EqualizationResults(){

}

void Mpx3EqualizationResults::SetPixelAdj(int pixId, int adj, lowHighSel sel) {
    if ( sel == __ADJ_L ) _pixId_Adj_L[pixId] = adj;
    if ( sel == __ADJ_H ) _pixId_Adj_H[pixId] = adj;
}
void Mpx3EqualizationResults::SetPixelReactiveThl(int pixId, int thl, lowHighSel sel) {
    if ( sel == __ADJ_L ) _pixId_Thl_L[pixId] = thl;
    if ( sel == __ADJ_H ) _pixId_Thl_H[pixId] = thl;
}
int Mpx3EqualizationResults::GetPixelAdj(int pixId, lowHighSel sel) {
    if ( sel == __ADJ_L ) return _pixId_Adj_L[pixId];
    if ( sel == __ADJ_H ) return _pixId_Adj_H[pixId];
    return _pixId_Adj_L[pixId];
}
int Mpx3EqualizationResults::GetPixelReactiveThl(int pixId, lowHighSel sel) {
    if ( sel == __ADJ_L ) return _pixId_Thl_L[pixId];
    if ( sel == __ADJ_H ) return _pixId_Thl_H[pixId];
    return _pixId_Thl_L[pixId];
}
void Mpx3EqualizationResults::SetStatus(int pixId, eq_status st, lowHighSel sel) {
    if ( sel == __ADJ_L ) _eqStatus_L[pixId] = st;
    if ( sel == __ADJ_H ) _eqStatus_H[pixId] = st;
}

Mpx3EqualizationResults::eq_status Mpx3EqualizationResults::GetStatus(int pixId, lowHighSel sel) {
    if ( sel == __ADJ_L ) return _eqStatus_L[pixId];
    if ( sel == __ADJ_H ) return _eqStatus_H[pixId];
    return _eqStatus_L[pixId];
}

//! The adj binary files come one per chip.  First 64k correspond to THL and next 64k to THH
bool Mpx3EqualizationResults::ReadAdjBinaryFile(QString fn) {

    qDebug() << "[INFO]\tRead Adj binary file: " << fn.toStdString().c_str();
    QFile file(fn);
    if ( !file.open(QIODevice::ReadOnly) ) {
        return false;
    }

    //! Read temporarily the entire chunk of 128k
    QByteArray temp_pixId_Adj_X;
    temp_pixId_Adj_X = file.readAll();
    file.close();
    int readSize = temp_pixId_Adj_X.size();
    qDebug() << "[INFO]\tRead " << temp_pixId_Adj_X.size() << " bytes";

    //! Now split in Low and High
    //!
    //! __matrix_size == 256*256=65536
    //! Adjustment files are stored as 1xN matrix
    //! THL: 0-65535, THH: 65536-131072
    for ( int i = 0 ; i < __matrix_size ; i++) {
        _pixId_Adj_L[i] = temp_pixId_Adj_X[i];
    }
    int cntr = 0;
    for ( int i = __matrix_size ; i < 2*__matrix_size ; i++) {
        if ( readSize > __matrix_size ) _pixId_Adj_H[cntr++] = temp_pixId_Adj_X[i];
        else _pixId_Adj_H[cntr++] = 0;
    }

    return true;
}

bool Mpx3EqualizationResults::WriteAdjBinaryFile(QString fn) {

    qDebug() << "[INFO]\tWriting adj file to: " << fn;

    QFile file(fn);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    qDebug() << "[INFO]\tWrite L: " << _pixId_Adj_L.size();
    qDebug() << "[INFO]\tWrite H: " << _pixId_Adj_H.size();

    qint64 bytesWritten = file.write(_pixId_Adj_L);
    bytesWritten += file.write(_pixId_Adj_H);

    qDebug() << "[INFO]\tBytes written: " << bytesWritten;

    file.close();

    return true;
}

bool Mpx3EqualizationResults::WriteMaskBinaryFile(QString fn) {

    ofstream fd;
    fd.open (fn.toStdString().c_str(), ios::out);
    qDebug() << "[INFO]\tWriting mask file to: " << fn;

    for ( int i = 0 ; i < __matrix_size ; i++ ) {
        //! If equalisation status is failed, then write the flattened coordinate to file.
        if ( _eqStatus_H[i] > __EQUALIZED || _eqStatus_L[i] > __EQUALIZED ) fd << i << endl;
    }

    return true;
}

bool Mpx3EqualizationResults::ReadMaskBinaryFile(QString fn) {

    ifstream fd;
    fd.open (fn.toStdString().c_str(), ios::out);
    qDebug() << "[INFO]\tReading mask file from: " << fn;

    int val;
    while ( fd.good() ) {
        fd >> val;
        if ( fd.eof() ) break;
        maskedPixels.insert( val );
    }

    return true;
}

/**
 * Convert the map to an array to feed the heatmap in the display
 */
int * Mpx3EqualizationResults::GetAdjustementMatrix(lowHighSel sel) {

    int * mat = new int[__matrix_size];
    for ( int j = 0 ; j < __matrix_size ; j++ ) {
        if ( sel == __ADJ_L ) mat[j] = _pixId_Adj_L[j];
        if ( sel == __ADJ_H ) mat[j] = _pixId_Adj_H[j];
    }

    return mat;
}

void Mpx3EqualizationResults::ClearAdj(){

    if ( _pixId_Adj_L.size() == __matrix_size &&  _pixId_Adj_H.size() == __matrix_size ) {
        for ( int i = 0 ; i < __matrix_size ; i++ ) {
            _pixId_Adj_L[i] = 0x0;
            _pixId_Adj_H[i] = 0x0;
        }
    } else {
        qDebug() << "Mpx3EqualizationResults::ClearAdj   [ERROR] the pixAdj ByteArray doesn't match the matrix size" << endl;
    }

}
void Mpx3EqualizationResults::ClearMasked(){
    maskedPixels.clear();
}
void Mpx3EqualizationResults::ClearReactiveThresholds(){
    _pixId_Thl_L.clear();
    _pixId_Thl_H.clear();
}
void Mpx3EqualizationResults::Clear() {
    ClearAdj();
    ClearMasked();
    ClearReactiveThresholds();
}

void Mpx3EqualizationResults::ExtrapolateAdjToTarget(int target, double eta_Adj_THL, lowHighSel sel) {

    // Here simply I go through every pixel and decide, according to the behavior of
    //   Adj(THL), which is the best adjustment.

    int atmax = 0;
    int atmin = 0;

    if ( sel == __ADJ_L ) {
        for ( int i = 0 ; i < __matrix_size ; i++ ) {

            // Every pixel has a different reactive thl.  At this point i know the behavior of
            //  Adj(THL) based on the mean.  I will assume that the slope is the same, but now
            //  I need to adjust the linear behavior to every particular pixel.  Let's obtain the
            //  cut.
            double pixel_cut = _pixId_Adj_L[i] - (eta_Adj_THL * _pixId_Thl_L[i]);
            // Now I can throw the extrapolation for every pixel to the equalisation target
            int adj = floor( (eta_Adj_THL * (double)target)  +  pixel_cut );

            if ( i < 5 ) {
                qDebug() << "ExtrapolateAdjToTarget eta_Adj_THL :" << eta_Adj_THL << ", pixel cut :" << pixel_cut << ", adj :" << adj << "\n";
            }

            // Replace the old matrix with this adjustment
            if ( adj > 0x1F ) { 			// Deal with an impossibly high adjustment
                _pixId_Adj_L[i] = 0x1F;
            } else if ( adj < 0x0 ) { 		// Deal with an impossibly low adjustment
                _pixId_Adj_L[i] = 0x0;
            } else {
                _pixId_Adj_L[i] = adj;
            }

            if ( int(_pixId_Adj_L[i]) == 0x1F ) atmax++;
            if ( int(_pixId_Adj_L[i]) == 0x0 ) atmin++;

        }
    } else if ( sel == __ADJ_H ) {
        for ( int i = 0 ; i < __matrix_size ; i++ ) {

            // Every pixel has a different reactive thl.  At this point i know the behavior of
            //  Adj(THL) based on the mean.  I will assume that the slope is the same, but now
            //  I need to adjust the linear behavior to every particular pixel.  Let's obtain the
            //  cut.
            double pixel_cut = _pixId_Adj_H[i] - (eta_Adj_THL * _pixId_Thl_H[i]);
            // Now I can throw the extrapolation for every pixel to the equalisation target
            int adj = floor( (eta_Adj_THL * (double)target)  +  pixel_cut );


            // Replace the old matrix with this adjustment
            if ( adj > 0x1F ) { 			// Deal with an impossibly high adjustment
                _pixId_Adj_H[i] = 0x1F;
            } else if ( adj < 0x0 ) { 		// Deal with an impossibly low adjustment
                _pixId_Adj_H[i] = 0x0;
            } else {
                _pixId_Adj_H[i] = adj;
            }

            if ( int(_pixId_Adj_H[i]) == 0x1F ) atmax++;
            if ( int(_pixId_Adj_H[i]) == 0x0 ) atmin++;
        }
    }
    qDebug() << "[INFO]\tExtrapolation formula used. Pixels set at max adj : " << atmax << ". Pixels set at min adj : " << atmin << "\n";

}

void QCstmEqualization::SetDAC_propagateInGUI(SpidrController * spidrcontrol, int devId, int dac_code, int dac_val ){

    // Set Dac
    spidrcontrol->setDac( devId, dac_code, dac_val );
    // Adjust the sliders and the SpinBoxes to the new value
    connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    // Get the DAC back just to be sure and then slide&spin
    //int dacVal = 0;
    //spidrcontrol->getDac( devId,  dac_code, &dacVal);
    // SlideAndSpin works with the DAC index, no the code.
    int dacIndex = _mpx3gui->getDACs()->GetDACIndex( dac_code );
    //slideAndSpin( dacIndex,  dacVal );
    slideAndSpin( dacIndex,  dac_val );
    disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );

    // Set in the local config.  This function also takes the dac_index and not the dac_code
    _mpx3gui->getDACs()->SetDACValueLocalConfig( devId, dacIndex, dac_val);

}

void QCstmEqualization::on_pushButton_testPulses_clicked()
{
    if (!testPulseEqualisationDialog) {
        testPulseEqualisationDialog = new testPulseEqualisation(_mpx3gui, this);
    }
    testPulseEqualisationDialog->show();
}
