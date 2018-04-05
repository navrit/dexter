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

    // Some defaults
    _deviceIndex = 2;
    _nTriggers = 1;
    _spacing = 4;

    // This will be recalculated
    _nchipsX = 2;
    _nchipsY = 2;
    _fullsize_x = __matrix_size_x * _nchipsX;
    _fullsize_y = __matrix_size_y * _nchipsY;

    // Suggest a descendant scan
    _maxScanTHL = 0;
    _minScanTHL = (1 << MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_0].bits) / 2;
    _scanDescendant = true;
    _busy = false;
    _resdataset = 0x0;
    _gridLayoutHistograms = 0x0;

    _stepScan = __default_step_scan;
    _setId = 0;
    _nChips = 1;
    _eqMap.clear();
    _workChipsIndx.clear();
    _scanAllChips = true;

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

    _fineTuningLoops = 32;
    _ui->fineTuningLoopsSpinBox->setValue( _fineTuningLoops );

    _ui->equalizationTHLTHHCombo->addItem( QString("THL and THH") );
    _ui->equalizationTHLTHHCombo->addItem( QString("Only THL") );
    _ui->equalizationTHLTHHCombo->addItem( QString("Only THH") );
    _equalizationCombination = __THLandTHH; // item 0

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

    _eqStatus = __INIT;
    _scanIndex = 0;
    _stepDone = new bool[__EQStatus_Count];
    for(int i = 0 ; i < __EQStatus_Count ; i++) _stepDone[i] = false;
}

Ui::QCstmEqualization * QCstmEqualization::GetUI() {
    return _ui;
}

QCstmEqualization::~QCstmEqualization()
{
    delete _ui;
}

void QCstmEqualization::setNHits(int val){
    _nHits = val;
}

void QCstmEqualization::SetLimits(){

    _ui->nHitsSpinBox->setMinimum( 1 );
    _ui->nHitsSpinBox->setMaximum( 100 );
    _ui->nHitsSpinBox->setValue( 3 );
    _nHits = 3;

    //
    _ui->devIdSpinBox->setMinimum( 0 );
    _ui->devIdSpinBox->setMaximum( 3 );
    _ui->devIdSpinBox->setValue( _deviceIndex );

    _ui->nTriggersSpinBox->setMinimum( 1 );
    _ui->nTriggersSpinBox->setMaximum( 1000 );
    _ui->nTriggersSpinBox->setValue( _nTriggers );

    _ui->spacingSpinBox->setMinimum( 0 );
    _ui->spacingSpinBox->setMaximum( 64 );
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

void QCstmEqualization::on_logYCheckBox_toggled(bool checked) {

    // All of them
    int chipListSize = (int)_workChipsIndx.size();
    // Report the pixels scheduled for equalization
    for ( int i = 0 ; i < chipListSize ; i++ ) {
        if ( checked ) {
            GetBarChart(_workChipsIndx[i])->SetLogY( true );
        } else {
            GetBarChart(_workChipsIndx[i])->SetLogY( false );
        }
        GetBarChart(_workChipsIndx[i])->replot( QCustomPlot::rpQueued );
    }

}

void QCstmEqualization::setFineTuningLoops(int nLoops) {
    _fineTuningLoops = nLoops;
}

void QCstmEqualization::ShowEqualizationForChip(bool /*checked*/) {

    // At least one has to be checked, otherwise refuse
    bool nothingChecked = true;
    for ( int i = 0 ; i < (int)_checkBoxes.size() ; i++ ) {
        if ( _checkBoxes[i]->isChecked() ) {
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
    for ( int i = (int)_workChipsIndx.size() - 1 ; i >= 0 ; i-- ) {
        GetBarChart( _workChipsIndx[i] )->hide();
    }

    // Show the new ones
    for ( int i = 0 ; i < (int)_checkBoxes.size() ; i++ ) {
        if ( _checkBoxes[i]->isChecked() ) GetBarChart( _workChipsIndx[i] )->show();
    }

}

void QCstmEqualization::NewRunInitEqualization() {

    // Rewind min and max suggesting a descendant scan.
    SetMinScan( (1 << MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_0].bits) / 2 );
    SetMaxScan( 0 );

    // Delete scans
    _scans.clear();

    //
    _ui->eqLabelFineTuningLoopProgress->setText("-/-");

    // Rewind state machine variables
    _eqStatus = __INIT;
    _scanIndex = 0;
    for(int i = 0 ; i < __EQStatus_Count ; i++) _stepDone[i] = false;

    // No sets available
    _setId = 0;

    // And step
    _stepScan = __default_step_scan;
    _ui->eqStepSpinBox->setValue( _stepScan );


    int chipListSize = (int)_workChipsIndx.size();
    for ( int i = 0 ; i < chipListSize ; i++ ) {

        // Steering info is already there ... only initialize vars
        //_steeringInfo.push_back( new equalizationSteeringInfo );

        // Decide on how many thresholds will be equalized
        _steeringInfo[i]->equalizationCombination = _equalizationCombination;
        // Type: Noise Edge, etc ....
        _steeringInfo[i]->equalizationType = _equalizationType;

        // Global adj setting used for DAC_DISC_X optimization
        _steeringInfo[i]->globalAdj = 0;

        // Which one to start with
        // If both ! --> start with TH0
        if ( _equalizationCombination == __OnlyTHL || _equalizationCombination == __THLandTHH ) {
            _steeringInfo[i]->currentTHx = MPX3RX_DAC_THRESH_0;
            _steeringInfo[i]->currentTHx_String = "THRESH_0";
            _steeringInfo[i]->currentDAC_DISC = MPX3RX_DAC_DISC_L;
            _steeringInfo[i]->currentDAC_DISC_String = "DAC_DISC_L";
        } else if ( _equalizationCombination == __OnlyTHH ) {
            _steeringInfo[i]->currentTHx = MPX3RX_DAC_THRESH_1;
            _steeringInfo[i]->currentTHx_String = "THRESH_1";
            _steeringInfo[i]->currentDAC_DISC = MPX3RX_DAC_DISC_H;
            _steeringInfo[i]->currentDAC_DISC_String = "DAC_DISC_H";
        }

        // optimization value for the corresponding DAC_DISC
        _steeringInfo[i]->currentDAC_DISC_OptValue = 0;
        // Eta and Cut for the THL_DACDisc function (DAC_DISC Optimization)
        _steeringInfo[i]->currentEta_THx_DAC_Disc = 0.;
        _steeringInfo[i]->currentCut_THx_DAC_Disc = 0.;
        // Eta and Cut for the Adj_THL function (Adj extrapolation)
        _steeringInfo[i]->currentEta_Adj_THx = 0.;
        _steeringInfo[i]->currentCut_Adj_THx = 0.;

    }

    // Clean all the BarCharts
    vector<BarChart * >::iterator ic  = _chart.begin();
    vector<BarChart * >::iterator icE = _chart.end();

    // clear the charts to start
    for ( ; ic != icE ; ic++ ) {
        (*ic)->Clean();
    }

    //
    QString startS = "\n-- Eq: ";
    startS += _steeringInfo[0]->currentTHx_String;
    startS += " ----- ";

    // Report the pixels scheduled for equalization
    startS += " CHIP ";
    for ( int i = 0 ; i < chipListSize ; i++ ) {
        startS += "[";
        startS += QString::number(_workChipsIndx[i], 'd', 0);
        startS += "] ";
    }
    startS += "-----------";

    AppendToTextBrowser( startS );


}

bool QCstmEqualization::InitEqualization(int chipId) {

    // Rewind state machine variables
    _eqStatus = __INIT;
    _scanIndex = 0;
    for(int i = 0 ; i < __EQStatus_Count ; i++) _stepDone[i] = false;

    //
    ClearTextBrowser();

    //
    _ui->eqLabelFineTuningLoopProgress->setText("-/-");

    // No sets available
    _setId = 0;

    // And step
    _stepScan = __default_step_scan;
    _ui->eqStepSpinBox->setValue( _stepScan );
    // Clear the list of chips
    Rewind();

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
    _nChips = (int) _workChipsIndx.size();

    // Create a steering structure for each chip
    // How many chips to equalize
    int chipListSize = (int)_workChipsIndx.size();
    for ( int i = 0 ; i < chipListSize ; i++ ) {

        // Create the steering info for this chip
        _steeringInfo.push_back( new equalizationSteeringInfo );

        // Decide on how many thresholds will be equalized
        _steeringInfo[i]->equalizationCombination = _equalizationCombination;
        // Type: Noise Edge, etc ....
        _steeringInfo[i]->equalizationType = _equalizationType;

        // Global adj setting used for DAC_DISC_X optimization
        _steeringInfo[i]->globalAdj = 0;

        // Which one to start with
        // If both ! --> start with TH0
        if ( _equalizationCombination == __OnlyTHL || _equalizationCombination == __THLandTHH ) {
            _steeringInfo[i]->currentTHx = MPX3RX_DAC_THRESH_0;
            _steeringInfo[i]->currentTHx_String = "THRESH_0";
            _steeringInfo[i]->currentDAC_DISC = MPX3RX_DAC_DISC_L;
            _steeringInfo[i]->currentDAC_DISC_String = "DAC_DISC_L";
        } else if ( _equalizationCombination == __OnlyTHH ) {
            _steeringInfo[i]->currentTHx = MPX3RX_DAC_THRESH_1;
            _steeringInfo[i]->currentTHx_String = "THRESH_1";
            _steeringInfo[i]->currentDAC_DISC = MPX3RX_DAC_DISC_H;
            _steeringInfo[i]->currentDAC_DISC_String = "DAC_DISC_H";
        }

        // optimization value for the corresponding DAC_DISC
        _steeringInfo[i]->currentDAC_DISC_OptValue = 0;
        // Eta and Cut for the THL_DACDisc function (DAC_DISC Optimization)
        _steeringInfo[i]->currentEta_THx_DAC_Disc = 0.;
        _steeringInfo[i]->currentCut_THx_DAC_Disc = 0.;
        // Eta and Cut for the Adj_THL function (Adj extrapolation)
        _steeringInfo[i]->currentEta_Adj_THx = 0.;
        _steeringInfo[i]->currentCut_Adj_THx = 0.;

    }

    // BarCharts !
    InitializeBarChartsEqualization();

    // THx scan label
    if( _steeringInfo[0]->currentTHx == MPX3RX_DAC_THRESH_0) _ui->thxLabel->setText( "THL:" );
    if( _steeringInfo[0]->currentTHx == MPX3RX_DAC_THRESH_1) _ui->thxLabel->setText( "THH:" );

    //
    QString startS = "\n-- Eq: ";
    startS += _steeringInfo[0]->currentTHx_String;
    startS += " ----- ";

    // Report the pixels scheduled for equalization
    startS += " CHIP ";
    for ( int i = 0 ; i < chipListSize ; i++ ) {
        startS += "[";
        startS += QString::number(_workChipsIndx[i], 'd', 0);
        startS += "] ";
    }
    startS += "-----------";

    AppendToTextBrowser( startS );

    //_ui->verticalLayoutBarChartEq->removeWidget( GetUI()->_histoWidget );

    // Pick what to show
    ShowEqualizationForChip( true );

    _nchipsX = 2; //_mpx3gui->getDataset()->getNChipsX();
    _nchipsY = 2; //_mpx3gui->getDataset()->getNChipsY();
    // TODO.  When one chips is connected the dataset returns 2,1 (which is good)
    _fullsize_x = __matrix_size_x * _nchipsX;
    _fullsize_y = __matrix_size_y * _nchipsY;

    // Create an equalization per chip
    for ( int i = 0 ; i < chipListSize ; i++ ) {
        _eqMap[_workChipsIndx[i]] = new Mpx3EqualizationResults;
    }

    return true;
}

void QCstmEqualization::InitializeBarChartsEqualization() {

    // There's one barChart object that was already instantiated in the GUI
    // But if we are equalizing more than 1 chip, we need the rest
    //_chart.push_back( GetUI()->_histoWidget );

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
    if ( _workChipsIndx.size() != _checkBoxes.size() ) return 0x0;

    // Find the index
    for (int i = 0 ; i < (int)_workChipsIndx.size() ; i++ ) {
        // return the corresponding results Ptr
        if ( _workChipsIndx[i] == chipIdx ) {
            return _checkBoxes[i];
        }
    }

    return 0x0; // otherwise

}

equalizationSteeringInfo * QCstmEqualization::GetSteeringInfo(int chipIdx) {

    // There should be as results objects as chips to be equalized
    if ( _workChipsIndx.size() != _steeringInfo.size() ) return 0x0;

    // Find the index
    for (int i = 0 ; i < (int)_workChipsIndx.size() ; i++ ) {
        // return the corresponding results Ptr
        if ( _workChipsIndx[i] == chipIdx ) {
            return _steeringInfo[i];
        }
    }

    return 0x0; // otherwise
}

BarChart * QCstmEqualization::GetAdjBarChart(int chipIdx, Mpx3EqualizationResults::lowHighSel sel) {

    // There should be twice as many AdjBarChart objects as chips to be equalized
    //  Low and High
    if ( _workChipsIndx.size() != _adjchart_L.size() ) return nullptr;
    if ( _workChipsIndx.size() != _adjchart_H.size() ) return nullptr;

    for (int i = 0 ; i < (int)_workChipsIndx.size() ; i++ ) {
        // return the corresponding results Ptr
        if ( _workChipsIndx[i] == chipIdx ) {
            if ( sel == Mpx3EqualizationResults::__ADJ_L) return _adjchart_L[i];
            if ( sel == Mpx3EqualizationResults::__ADJ_H) return _adjchart_H[i];
        }
    }

    return nullptr; // otherwise
}

BarChart * QCstmEqualization::GetBarChart(int chipIdx) {

    // There should be as results objects as chips to be equalized
    if ( _workChipsIndx.size() != _chart.size() ) return 0x0;

    // Find the index
    for (int i = 0 ; i < (int)_workChipsIndx.size() ; i++ ) {
        // return the corresponding results Ptr
        if ( _workChipsIndx[i] == chipIdx ) {
            return _chart[i];
        }
    }

    return 0x0; // otherwise
}

void QCstmEqualization::Rewind() {

    // List of chip indexes to equalize
    _workChipsIndx.clear();

    /*

    // Establish if it is needed to Equalize another chip
    if( _deviceIndex < _nChips - 1  ) {

        // Next chip
        _deviceIndex++;
        _ui->devIdSpinBox->setValue( _deviceIndex );

        InitEqualization();

        // Clear the previous scans !
        _scans.clear();

        StartEqualization( _deviceIndex );

    } else { // when done

        AppendToTextBrowser( "-- DONE ----------------" );
    }
     */

}

/**
 * See if the pixel is in the list of chips scheduled to Equalize
 */

bool QCstmEqualization::pixelInScheduledChips(int pix) {

    int chipListSize = (int)_workChipsIndx.size();

    for ( int i = 0 ; i < chipListSize ; i++ ) {

        if (
                pix >= ( _workChipsIndx[i] * __matrix_size )
                &&
                pix <  ( (_workChipsIndx[i] + 1) * __matrix_size )
                ) return true;

    }

    return false;
}

/**
 * When equalizing only one chip, other chips may be still connected and need to
 *  remain quiet.  Set thresholds for those chips high.
 */
void QCstmEqualization::KeepOtherChipsQuiet() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    int ndevs = _mpx3gui->getConfig()->getNDevicesSupported();
    int chipListSize = (int)_workChipsIndx.size();

    for ( int idx = 0 ; idx < ndevs ; idx++ ) {

        // If the device is not present continue
        if ( ! _mpx3gui->getConfig()->detectorResponds( idx ) ) continue;

        // If the device is present and it is not in the schedule-for-eq list
        //  apply the high thresholds
        for ( int i = 0 ; i < chipListSize ; i++ ) if ( _workChipsIndx[i] == idx ) continue;

        //
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_0,  (1 << MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_0-1].bits) / 2);
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_1,  (1 << MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_1-1].bits) / 2);
        // Work with all the thresholds if operating in Colour mode
        //if ( _mpx3gui->getConfig()->getColourMode() ) {
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_2,  (1 << MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_2-1].bits) / 2);
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_3,  (1 << MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_3-1].bits) / 2);
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_4,  (1 << MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_4-1].bits) / 2);
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_5,  (1 << MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_5-1].bits) / 2);
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_6,  (1 << MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_6-1].bits) / 2);
        SetDAC_propagateInGUI( spidrcontrol, idx, MPX3RX_DAC_THRESH_7,  (1 << MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_7-1].bits) / 2);
        //}
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

    if(makeTeaCoffeeDialog()){
        // Get busy !
        _busy = true;
        _scanAllChips = false;

        // Init
        if ( ! InitEqualization( _deviceIndex ) ) return;

        KeepOtherChipsQuiet();

        // Send Configuration
        //_mpx3gui->getConfig()->SendConfiguration( Mpx3Config::__ALL );

        StartEqualization( );

    } else {
        return;
    }

}

void QCstmEqualization::StartEqualizationAllChips() {

    if(makeTeaCoffeeDialog()){
        // Get busy !
        _busy = true;
        _scanAllChips = true;

        // Init
        if ( ! InitEqualization( -1 ) ) return;

        StartEqualization( ); // Default will equalize all chips

    } else {
        return;
    }

}

void QCstmEqualization::StartEqualizationSequentialSingleChips()
{
    //! TODO: Link it to _startEqAllSequential
    //!
    //! TODO: Make this work and connect to new button

    QMessageBox msgBox;
    msgBox.setWindowTitle("Information");
    msgBox.setText("This isn't connected to anything yet, do it manually :P ...");
    msgBox.addButton(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);

    return;

    // Try to go through all chips
    if( _deviceIndex < _nChips - 1 && _deviceIndex >= 0) {

        // Next chip
        _deviceIndex++;
        _ui->devIdSpinBox->setValue( _deviceIndex );

        // Initialise
        if ( ! InitEqualization( -1 ) ) return;

        KeepOtherChipsQuiet();

        // Clear the previous scans!
        _scans.clear();

        StartEqualizationSingleChip();

        // Save results separately for now. Maybe merge and then save one config file later
        SaveEqualization( QDir::homePath() );

    } else { // when done

        AppendToTextBrowser( qPrintable( QString("-- Results for chip %1 saved to %2").arg(_deviceIndex).arg(QDir::homePath()) ) );
        AppendToTextBrowser( qPrintable(QString("-- DONE chip %1 ----------------").arg(_deviceIndex)) );
    }
}

void QCstmEqualization::StartEqualization() {

    // I need to do this here and not when already running the thread
    // Get the IP source address (SPIDR network interface) from the already connected SPIDR module.
    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    if( spidrcontrol ) { spidrcontrol->getIpAddrSrc( 0, &_srcAddr ); }
    else { _srcAddr = 0; }

    // how many chips to equalize
    int chipListSize = (int)_workChipsIndx.size();

    //Rewind(); // FIXME, this is not needed anymore

    // Preliminary) Find out the equalization range

    // First) DAC_Disc Optimization
    if( EQ_NEXT_STEP( __INIT) ) {

        ////////////
        // STEP 1 //
        ////////////
        QString titleInit = "1) ";
        titleInit += _steeringInfo[0]->currentDAC_DISC_String; // it will be the same for all chips
        titleInit += " optimization ...";
        AppendToTextBrowser( titleInit );

        // CONFIG for all involved chips
        for ( int i = 0 ; i < chipListSize ; i++ ) {
            Configuration(_workChipsIndx[i], _steeringInfo[i]->currentTHx, true);
            _steeringInfo[i]->globalAdj = 0;
            _steeringInfo[i]->currentDAC_DISC_OptValue = 100; // for now make the opt value equal to the test value
        }

        // Prepare and launch the thread
        DAC_Disc_Optimization_100( );


    } else if ( EQ_NEXT_STEP(__DAC_Disc_Optimization_100 ) ) {

        // Check if the previous scan has been stopped by the user
        //		_scans
        for ( int i = 0 ; i < chipListSize ; i++ ) {
            // Extract results from immediately previous scan. Calc the stats now (this is quick)
            _scans[_scanIndex - 1]->ExtractStatsOnChart(_workChipsIndx[i], _setId - 1);
            // Show the results
            DAC_Disc_Optimization_DisplayResults( _scans[_scanIndex - 1]->GetScanResults(_workChipsIndx[i]) );
            // New text value
            _steeringInfo[i]->currentDAC_DISC_OptValue = 150; // for now make the opt value equal to the test value

        }

        // And go for next scan
        DAC_Disc_Optimization_150( );


    } else if ( EQ_NEXT_STEP(__DAC_Disc_Optimization_150 ) ) {

        for ( int i = 0 ; i < chipListSize ; i++ ) {
            // Extract results from immediately previous scan. Calc the stats now (this is quick)
            _scans[_scanIndex - 1]->ExtractStatsOnChart(_workChipsIndx[i], _setId - 1);
            // Show the results
            DAC_Disc_Optimization_DisplayResults( _scans[_scanIndex - 1]->GetScanResults(_workChipsIndx[i]) );
        }

        // And calculate the optimal DAC_Disc
        for ( int i = 0 ; i < chipListSize ; i++ ) {
            ScanResults * res_100 = _scans[_scanIndex - 2]->GetScanResults( _workChipsIndx[i] );
            ScanResults * res_150 = _scans[_scanIndex - 1]->GetScanResults( _workChipsIndx[i] );
            DAC_Disc_Optimization(_workChipsIndx[i], res_100, res_150);
        }

        // I could get rid of the previous two equalizations
        //delete _scans[_eqStatus - 2];
        //delete _scans[_eqStatus - 1];
        ////////////
        // STEP 2 //
        ////////////
        AppendToTextBrowser("2) Test adj-bits sensibility and extrapolate to target ...");
        PrepareInterpolation_0x0();

    } else if ( EQ_NEXT_STEP(__PrepareInterpolation_0x0) ) {

        // Results
        int nNonReactive = _scans[_scanIndex - 1]->NumberOfNonReactingPixels();
        // Correct in case not all chips are active
        nNonReactive -= (_mpx3gui->getConfig()->getNDevicesSupported() - chipListSize)*__matrix_size;
        if ( nNonReactive > 0 ) {
            cout << "[WARNING] there are non reactive pixels : " << nNonReactive << endl;
        }

        for ( int i = 0 ; i < chipListSize ; i++ ) {
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
            cout << "[WARNING] there are non reactive pixels : " << nNonReactive << endl;
        }

        // Use a data set to put the adj matrixes together
        if ( ! _resdataset ) _resdataset = new Dataset ( __matrix_size_x, __matrix_size_y, _nchipsX*_nchipsY );
        _resdataset->clear();

        for ( int i = 0 ; i < chipListSize ; i++ ) {

            _scans[_scanIndex - 1]->ExtractStatsOnChart(_workChipsIndx[i], _setId - 1);
            DisplayStatsInTextBrowser(_steeringInfo[i]->globalAdj, _steeringInfo[i]->currentDAC_DISC_OptValue, _scans[_scanIndex - 1]->GetScanResults(_workChipsIndx[i]));

            // Interpolate now
            //ScanResults * res_x0 = _scans[_scanIndex - 2]->GetScanResults( _workChipsIndx[i] );
            //ScanResults * res_x5 = _scans[_scanIndex - 1]->GetScanResults( _workChipsIndx[i] );

            ThlScan * scan_x0 = _scans[_scanIndex - 2];
            ThlScan * scan_x5 = _scans[_scanIndex - 1];
            int * adjdata = CalculateInterpolation( _workChipsIndx[i], scan_x0, scan_x5 );
            // Stack
            _resdataset->setFrame(adjdata, _workChipsIndx[i], 0);

        }

        // Once the frame is complete, extract info
        int * fulladjdata = _resdataset->getLayer( 0 );
        // Plot
        UpdateHeatMap(fulladjdata, _fullsize_x, _fullsize_y);

        // Perform now a scan with the extrapolated adjustments
        //    Here there's absolutely no need to go through the THL range.
        // New limits --> ask the last scan
        ScanOnInterpolation();


    } else if ( EQ_NEXT_STEP( __ScanOnInterpolation) ) {

        // Results
        int nNonReactive = _scans[_scanIndex - 1]->NumberOfNonReactingPixels();
        // Correct in case not all chips are active
        nNonReactive -= (_mpx3gui->getConfig()->getNDevicesSupported() - chipListSize)*__matrix_size;

        if ( nNonReactive > 0 ) {
            cout << "[WARNING] there are non reactive pixels : " << nNonReactive << endl;
        }

        for ( int i = 0 ; i < chipListSize ; i++ ) {
            _scans[_scanIndex - 1]->ExtractStatsOnChart(_workChipsIndx[i], _setId - 1);
            DisplayStatsInTextBrowser(-1, _steeringInfo[i]->currentDAC_DISC_OptValue, _scans[_scanIndex - 1]->GetScanResults(_workChipsIndx[i]));

        }

        // If fast equalization, skip finetunning, same for all chips
        if ( _steeringInfo[0]->equalizationType == __NoiseCentroidFAST
             ||
             _steeringInfo[0]->equalizationType == __NoiseEdgeFAST
             ) {

            // Go directly to next scan
            // Go to next step, except for fine tuning where I use the same previous scan
            _stepDone[_eqStatus] = true;
            _eqStatus++;
            StartEqualization( );

        } else {

            AppendToTextBrowser("3) Fine tuning ...");

            // 5) Attempt fine tuning
            FineTunning( );
        }

    }  else if ( EQ_NEXT_STEP( __FineTunning ) ) {

        // clear previous data
        _resdataset->clear();
        for ( int i = 0 ; i < chipListSize ; i++ ) {
            int * da = _eqMap[_workChipsIndx[i]]->GetAdjustementMatrix();
            _resdataset->setFrame(da, _workChipsIndx[i], 0);
        }
        int * fulladjdata = _resdataset->getLayer( 0 );
        UpdateHeatMap(fulladjdata, _fullsize_x, _fullsize_y);

        // Decide if the equalization needs to be ran again for THH or if we are done

        if ( _equalizationCombination == __THLandTHH ) {
            // At this point we finished THL. Go for THH now
            _equalizationCombination = __OnlyTHH;

            // partial re-initialization
            NewRunInitEqualization();

            // Good for single ship equalization
            //! Oooooh, well this would have been amazing to know before...
            if ( ! _scanAllChips ) KeepOtherChipsQuiet();

            // Start again
            StartEqualization();

        } else {

            // Write the results
            SaveEqualization();

            // done
            AppendToTextBrowser( "[DONE]" );

            QString msg = "Finished equalisation on: " + _mpx3gui->getVisualization()->getStatsString_deviceId();
            emit sig_statusBarAppend(msg, "green");

        }

    }


    // Second) First interpolation.  Coming close to the equalization target
    ////setId = PrepareInterpolation(setId, MPX3RX_DAC_DISC_L);

    // Third) Fine tunning.
    //setId = FineTunning(setId, MPX3RX_DAC_DISC_L);


    /////////////////////////////////////////////
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
    /*_ui->dockWidget->setWidget(_ui->_intermediatePlot);
    addDockWidget???
    */
}


int * QCstmEqualization::CalculateInterpolation(int devId, ThlScan * scan_x0, ThlScan * scan_x5 ) { //ScanResults * res_x0, ScanResults * res_x5) {

    ////////////////////////////////////////////////////////////////////////////////////
    // 6) Stablish the dependency THL(Adj). It will be used to extrapolate to the
    //    Equalization target for every pixel
    ScanResults * res_x0 = scan_x0->GetScanResults( devId );
    ScanResults * res_x5 = scan_x5->GetScanResults( devId );

    double eta = 0., cut = 0.;
    GetSlopeAndCut_Adj_THL(res_x0, res_x5, eta, cut);
    GetSteeringInfo(devId)->currentEta_Adj_THx = eta;
    GetSteeringInfo(devId)->currentCut_Adj_THx = cut;

    Mpx3EqualizationResults::lowHighSel sel = Mpx3EqualizationResults::__ADJ_L;
    if ( GetSteeringInfo(devId)->currentDAC_DISC == MPX3RX_DAC_DISC_L ) sel = Mpx3EqualizationResults::__ADJ_L;
    if ( GetSteeringInfo(devId)->currentDAC_DISC == MPX3RX_DAC_DISC_H ) sel = Mpx3EqualizationResults::__ADJ_H;

    ////////////////////////////////////////////////////////////////////////////////////
    // 7) Extrapolate to the target using the last scan information and the knowledge
    //    on the Adj_THL dependency.
    _scans[_scanIndex - 1]->DeliverPreliminaryEqualization(devId, GetSteeringInfo(devId)->currentDAC_DISC, _eqMap[devId],  GetSteeringInfo(devId)->globalAdj );
    _eqMap[devId]->ExtrapolateAdjToTarget( __equalization_target, GetSteeringInfo(devId)->currentEta_Adj_THx, sel );

    int * adj_matrix = 0x0;
    if ( GetSteeringInfo(devId)->currentDAC_DISC == MPX3RX_DAC_DISC_L ) adj_matrix = _eqMap[devId]->GetAdjustementMatrix();
    if ( GetSteeringInfo(devId)->currentDAC_DISC == MPX3RX_DAC_DISC_H ) adj_matrix = _eqMap[devId]->GetAdjustementMatrix(Mpx3EqualizationResults::__ADJ_H);

    return adj_matrix;
}

void QCstmEqualization::DAC_Disc_Optimization_DisplayResults(ScanResults * res) {

    // Results
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

    ////////////////////////////////////////////////////////////////////////////////////
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
    cprop.color_r = 0;
    cprop.color_g = 127;
    cprop.color_b = 0;

    for ( int i = 0 ; i < (int)_workChipsIndx.size() ; i++ ) {
        cprop.name = BuildChartName( _workChipsIndx[i], legend );
        GetBarChart( _workChipsIndx[i] )->AppendSet( cprop );
    }

    // DAC_DiscL=100
    for ( int i = 0 ; i < (int)_workChipsIndx.size() ; i++ ) {
        SetDAC_propagateInGUI(spidrcontrol, _workChipsIndx[i], _steeringInfo[i]->currentDAC_DISC, _steeringInfo[i]->currentDAC_DISC_OptValue);
    }

    // This is a scan that I can truncate early ... I don't need to go all the way
    tscan->DoScan(  _steeringInfo[0]->currentTHx , _setId++, _steeringInfo[0]->currentDAC_DISC, 1 ); // THX and DAC_DISC_X same for all chips
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

    ////////////////////////////////////////////////////////////////////////////////////
    // 2) Scan with MPX3RX_DAC_DISC_L = 150
    ThlScan * tscan = new ThlScan(_mpx3gui, this);
    tscan->ConnectToHardware(spidrcontrol, spidrdaq);
    BarChartProperties cprop_150;
    cprop_150.min_x = 0;
    cprop_150.max_x = 200;
    cprop_150.nBins = 512;
    cprop_150.color_r = 127;
    cprop_150.color_g = 127;
    cprop_150.color_b = 10;
    for ( int i = 0 ; i < (int)_workChipsIndx.size() ; i++ ) {
        cprop_150.name = BuildChartName( _workChipsIndx[i], legend );
        GetBarChart( _workChipsIndx[i] )->AppendSet( cprop_150 );
    }

    // DAC_DiscL=150
    for ( int i = 0 ; i < (int)_workChipsIndx.size() ; i++ ) {
        spidrcontrol->setDac( _workChipsIndx[i], _steeringInfo[i]->currentDAC_DISC, _steeringInfo[i]->currentDAC_DISC_OptValue );
    }
    tscan->DoScan( _steeringInfo[0]->currentTHx, _setId++,  _steeringInfo[0]->currentDAC_DISC, 1 );
    tscan->SetWorkChipIndexes( _workChipsIndx, _steeringInfo );

    // Launch as thread.  Connect the slot which signals when it's done
    _scans.push_back( tscan ); _scanIndex++;
    connect( tscan, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
    tscan->start();

}

int QCstmEqualization::FineTunning() {

    // The fine tuning reports to a new BarChat but runs on the previous ThlScan
    BarChartProperties cprop_opt;
    cprop_opt.name = "Fine_tuning";
    cprop_opt.min_x = 0;
    cprop_opt.max_x = 511;
    cprop_opt.nBins = 512;
    cprop_opt.color_r = 0;
    cprop_opt.color_g = 0;
    cprop_opt.color_b = 0;
    //_ui->_histoWidget->AppendSet( cprop_opt );

    // The step goes down to 1 here
    _stepScan = 1;
    _ui->eqStepSpinBox->setValue( _stepScan );

    // Start from the last scan.
    int lastScanIndex = (int)_scans.size() - 1;
    ThlScan * lastScan = 0x0;
    if( lastScanIndex > 0 ) {
        lastScan = _scans[lastScanIndex];
    } else {
        return -1;
    }

    // Use its own previous limits
    lastScan->SetMinScan( lastScan->GetDetectedHighScanBoundary() );
    lastScan->SetMaxScan( lastScan->GetDetectedLowScanBoundary() );

    lastScan->DoScan( _steeringInfo[0]->currentTHx, _setId++, _steeringInfo[0]->currentDAC_DISC, -1 ); // -1: Do all loops
    lastScan->SetScanType( ThlScan::__FINE_TUNNING1_SCAN );
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

    ////////////////////////////////////////////////////////////////////////////////////
    // 4) Now IDAC_DISC optimal is such that:
    //    With an adj-bit of 00101[5] the optimal mean is at __equalization_target + 3.2 sigma

    // Desired mean value = __equalization_target + 3.2 sigma
    // Taking sigma from the first scan.
    double meanTHL_for_opt_IDAC_DISC = __equalization_target + 3.2*res_100->sigma;

    // Using the relation DAC_Disc[L/H](THL) we can find the value of DAC_Disc
    GetSteeringInfo(devId)->currentDAC_DISC_OptValue = (int) EvalLinear(GetSteeringInfo(devId)->currentEta_THx_DAC_Disc, GetSteeringInfo(devId)->currentCut_THx_DAC_Disc, meanTHL_for_opt_IDAC_DISC);

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

void QCstmEqualization::SaveEqualization(QString path) {

    // TODO CHECK THIS untested
    qDebug() << "[DISABLED] TODO CHECK THIS untested: QCstmEqualization::SaveEqualization \n _mpx3gui->getConfig()->toJsonFile(filename)...";

    QString filenameEqualisation;

    if (path == "") {
        //! Get folder to save equalisation files to
        QString path = QFileDialog::getExistingDirectory(this, tr("Open Directory to save equalisations to"),
                                                         QDir::currentPath(),
                                                         QFileDialog::ShowDirsOnly);
        //! User pressed cancel, offer another go at saving
        if (path.isEmpty()){
            QMessageBox::StandardButton reply;
            reply = QMessageBox::warning(this,
                                         tr("Warning"),
                                         tr("Are you sure you do not want to save equalisations and config files?"),
                                         QMessageBox::Save|QMessageBox::Cancel);
            if (reply == QMessageBox::Save) {
                path = QFileDialog::getExistingDirectory(this,
                                                         tr("Open Directory to save equalisations to"),
                                                         QDir::currentPath(),
                                                         QFileDialog::ShowDirsOnly);
            } else {
                sig_statusBarAppend(tr("Equalisation not saved, you may save them manually"),"red");
                return;
            }

        }
        path.append(QDir::separator());
        filenameEqualisation = path;
        filenameEqualisation.append("config.json");
    } else {
        path.append(QDir::separator());
        filenameEqualisation = path;
        filenameEqualisation.append( QString("config-chip%1.json").arg(_deviceIndex) );
    }



    resetThresholds();

    //! Save equalisations with DACs when you run an equalisation
    _mpx3gui->getConfig()->toJsonFile(filenameEqualisation, true);

    int chipListSize = (int)_workChipsIndx.size();

    //! Build adj and mask filename+path strings and save them
    for ( int i = 0 ; i < chipListSize ; i++ ) {

        QString adjfn = path;
        adjfn += "adj_";
        adjfn += QString::number(_workChipsIndx[i], 10);

        QString maskfn = path;
        maskfn += "mask_";
        maskfn += QString::number(_workChipsIndx[i], 10);

        // Binary file
        _eqMap[_workChipsIndx[i]]->WriteAdjBinaryFile( adjfn );
        // Masked pixels
        _eqMap[_workChipsIndx[i]]->WriteMaskBinaryFile( maskfn );

    }

}



//int QCstmEqualization::DetectStartEqualizationRange(int setId, int DAC_Disc_code) {
//	return setId;
//}

void QCstmEqualization::PrepareInterpolation_0x0() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    QString legend = _steeringInfo[0]->currentDAC_DISC_String;
    legend += "_Opt_adj";
    legend += QString::number(_steeringInfo[0]->globalAdj, 'd', 0);

    ////////////////////////////////////////////////////////////////////////////////////
    // 5)  See where the pixels fall now for adj0 and keep the pixel information
    ThlScan * tscan_opt_adj0 = new ThlScan( _mpx3gui, this);
    tscan_opt_adj0->SetMinScan(  );
    tscan_opt_adj0->SetMaxScan(  );

    tscan_opt_adj0->ConnectToHardware(spidrcontrol, spidrdaq);
    BarChartProperties cprop_opt_adj0;
    cprop_opt_adj0.min_x = 0;
    cprop_opt_adj0.max_x = 511;
    cprop_opt_adj0.nBins = 512;
    cprop_opt_adj0.color_r = 0;
    cprop_opt_adj0.color_g = 10;
    cprop_opt_adj0.color_b = 127;

    for ( int i = 0 ; i < (int)_workChipsIndx.size() ; i++ ) {
        cprop_opt_adj0.name = BuildChartName( _workChipsIndx[i], legend );
        GetBarChart( _workChipsIndx[i] )->AppendSet( cprop_opt_adj0 );
    }

    // Let's assume the mean falls at the equalization target
    //tscan_opt_adj0->SetStopWhenPlateau(true);
    tscan_opt_adj0->DoScan(  _steeringInfo[0]->currentTHx, _setId++, _steeringInfo[0]->currentDAC_DISC, -1 ); // -1: Do all loops
    tscan_opt_adj0->SetWorkChipIndexes( _workChipsIndx, _steeringInfo );

    // Launch as thread.  Connect the slot which signals when it's done
    _scans.push_back( tscan_opt_adj0 ); _scanIndex++;
    connect( tscan_opt_adj0, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
    tscan_opt_adj0->start();

}

void QCstmEqualization::ScanOnInterpolation() {

    // The step goes down to 1 here
    _stepScan = 1;
    _ui->eqStepSpinBox->setValue( _stepScan );

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    QString legend = _steeringInfo[0]->currentDAC_DISC_String;
    legend += "_Opt_adjX";

    // Note that this suggests a descending scan.
    ThlScan * scan_adj5 = _scans[_scanIndex - 1];

    ThlScan * tscan_opt_ext = new ThlScan(_mpx3gui, this);
    tscan_opt_ext->SetMinScan( scan_adj5->GetDetectedHighScanBoundary() );
    tscan_opt_ext->SetMaxScan( scan_adj5->GetDetectedLowScanBoundary() );

    tscan_opt_ext->ConnectToHardware(spidrcontrol, spidrdaq);
    BarChartProperties cprop_opt_ext;
    cprop_opt_ext.min_x = 0;
    cprop_opt_ext.max_x = 511;
    cprop_opt_ext.nBins = 512;
    cprop_opt_ext.color_r = 192;
    cprop_opt_ext.color_g = 192;
    cprop_opt_ext.color_b = 192;
    //_ui->_histoWidget->AppendSet( cprop_opt_ext );

    for ( int i = 0 ; i < (int)_workChipsIndx.size() ; i++ ) {
        cprop_opt_ext.name = BuildChartName( _workChipsIndx[i], legend );
        GetBarChart( _workChipsIndx[i] )->AppendSet( cprop_opt_ext );
    }

    // Let's assume the mean falls at the equalization target
    tscan_opt_ext->DoScan( _steeringInfo[0]->currentTHx, _setId++, _steeringInfo[0]->currentDAC_DISC, -1 ); // -1: Do all loops
    tscan_opt_ext->SetAdjustmentType( ThlScan::__adjust_to_equalizationMatrix );
    // A global_adj doesn't apply here anymore.  Passing -1.
    tscan_opt_ext->SetWorkChipIndexes( _workChipsIndx, _steeringInfo );

    // Launch as thread.  Connect the slot which signals when it's done
    _scans.push_back( tscan_opt_ext); _scanIndex++;
    connect( tscan_opt_ext, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
    tscan_opt_ext->start();

}

void QCstmEqualization::PrepareInterpolation_0x5() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    ////////////////////////////////////////////////////////////////////////////////////
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
    cprop_opt_adj5.color_r = 127;
    cprop_opt_adj5.color_g = 10;
    cprop_opt_adj5.color_b = 0;
    //_ui->_histoWidget->AppendSet( cprop_opt_adj5 );

    for ( int i = 0 ; i < (int)_workChipsIndx.size() ; i++ ) {
        cprop_opt_adj5.name = BuildChartName( _workChipsIndx[i], legend );
        GetBarChart( _workChipsIndx[i] )->AppendSet( cprop_opt_adj5 );
    }

    // Let's assume the mean falls at the equalization target
    //tscan_opt_adj5->SetStopWhenPlateau(true);
    tscan_opt_adj5->DoScan(  _steeringInfo[0]->currentTHx, _setId++, _steeringInfo[0]->currentDAC_DISC, -1 ); // -1: Do all loops
    tscan_opt_adj5->SetWorkChipIndexes( _workChipsIndx, _steeringInfo );

    // Launch as thread.  Connect the slot which signals when it's done
    _scans.push_back( tscan_opt_adj5 ); _scanIndex++;
    connect( tscan_opt_adj5, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
    tscan_opt_adj5->start();

}

/*

    // TODO
    // For the moment mask the non reactive pixels
    vector<int> nonReactive = tscan_opt_ext->GetNonReactingPixels();
    vector<int>::iterator nrItr = nonReactive.begin();
    for ( ; nrItr != nonReactive.end() ; nrItr++ ) {
        _eqresults->maskPixel( *nrItr );
    }
    SetAllAdjustmentBits();
 */


void QCstmEqualization::DisplayStatsInTextBrowser(int adj, int dac_disc, ScanResults * res) {

    QString statsString = "[";
    statsString += QString::number(res->chipIndx, 'd', 0);
    statsString += "] Adj=0x";
    if (adj >= 0) statsString += QString::number(adj, 'd', 0);
    else statsString += "X";
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
    // Now revisit the equalization.
    // It knows where to pick up.
    StartEqualization( );
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

/*
 * The way the next two functions relate looks redundant but I need
 * it for when this is called from inside a thread.
 */
void QCstmEqualization::SetAllAdjustmentBits() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    SetAllAdjustmentBits(spidrcontrol);

}
/*
void QCstmEqualization::SetAllAdjustmentBits(SpidrController * spidrcontrol, int chipIndex ) {

    int nChips = _mpx3gui->getConfig()->getNDevicesSupported();
    if ( chipIndex < 0 || chipIndex > nChips - 1) {
        cout << "[ERROR] wrong chip index !" << endl;
        return;
    }
    _deviceIndex = chipIndex;
    _eqresults = _eqMap[chipIndex];

    // This comes from hand-picked masking operation
    // Clean the chip first
    pair<int, int> pix;
    for ( int i = 0 ; i < __matrix_size ; i++ ) {
        pix = XtoXY(i, __array_size_x);
        spidrcontrol->setPixelMaskMpx3rx(pix.first, pix.second, false);
    }
    spidrcontrol->setPixelConfigMpx3rx( _deviceIndex );

    // Now set the right adjustments
    SetAllAdjustmentBits( spidrcontrol );

}
 */

/**
 * Send the configuration to all chips
 */
void QCstmEqualization::SetAllAdjustmentBits(SpidrController * spidrcontrol) {

    int nChips = _mpx3gui->getConfig()->getNDevicesSupported();

    for ( int i = 0 ; i < nChips ; i++ ) {

        // Check if the device is alive
        if ( ! _mpx3gui->getConfig()->detectorResponds( i ) ) {
            continue;
        }

        // Go ahead and configure this chip
        SetAllAdjustmentBits(spidrcontrol, i);

    }

}

void QCstmEqualization::SetAllAdjustmentBits(SpidrController * spidrcontrol, int chipIndex, bool applymask) {

    if( !spidrcontrol ) {
        QMessageBox::information(this, tr("Clear configuration"), tr("The system is disconnected. Nothing to clear.") );
        return;
    }

    int nChips = _mpx3gui->getConfig()->getNDevicesSupported();
    if ( chipIndex < 0 || chipIndex > nChips - 1) {
        qDebug() << "[ERROR] wrong chip index !";
        return;
    }

    // Adj bits
    pair<int, int> pix;

    for ( int i = 0 ; i < __matrix_size ; i++ ) {

        pix = XtoXY(i, __array_size_x);
        //qDebug() << _eqMap[chipIndex]->GetPixelAdj(i) << _eqMap[chipIndex]->GetPixelAdj(i, Mpx3EqualizationResults::__ADJ_H);
        spidrcontrol->configPixelMpx3rx(pix.first,
                                        pix.second,
                                        _eqMap[chipIndex]->GetPixelAdj(i),
                                        _eqMap[chipIndex]->GetPixelAdj(i, Mpx3EqualizationResults::__ADJ_H)
                                        );

        // Fill adj distributions
        //_adjchart_L[chipIndex]->SetValueInSet(0, _eqMap[chipIndex]->GetPixelAdj(i));
        //_adjchart_H[chipIndex]->SetValueInSet(0, _eqMap[chipIndex]->GetPixelAdj(i, Mpx3EqualizationResults::__ADJ_H));

    }

    // This may not the moment for a mask
    if ( applymask ) {
        // Mask
        if ( _eqMap[chipIndex]->GetNMaskedPixels() > 0 ) {
            QSet<int> tomask = _eqMap[chipIndex]->GetMaskedPixels();
            QSet<int>::iterator i = tomask.begin();
            QSet<int>::iterator iE = tomask.end();
            pair<int, int> pix;
            qDebug() << "[INFO] Masking ...";
            for ( ; i != iE ; i++ ) {
                pix = XtoXY( (*i), __matrix_size_x );
                qDebug() << "     devid:" << chipIndex << " | " << pix.first << "," << pix.second << " | " << XYtoX(pix.first, pix.second, _mpx3gui->getDataset()->x());
                spidrcontrol->setPixelMaskMpx3rx(pix.first, pix.second);
            }
        } else { // When the mask is empty go ahead and set all to zero
            for ( int i = 0 ; i < __matrix_size ; i++ ) {
                pix = XtoXY(i, __array_size_x);
                spidrcontrol->setPixelMaskMpx3rx(pix.first, pix.second, false);
            }
        }
    }

    spidrcontrol->setPixelConfigMpx3rx( chipIndex );

}

void QCstmEqualization::ClearAllAdjustmentBits(int devId) {

    // Clear all data structures
    _eqMap[devId]->ClearAdj();
    _eqMap[devId]->ClearMasked();
    _eqMap[devId]->ClearReactiveThresholds();

    // And now set it up
    SetAllAdjustmentBits();

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
        spidrcontrol->configPixelMpx3rx(pix.first, pix.second, val_L, val_H ); // 0x1F = 31 is the max adjustment for 5 bits
    }
    spidrcontrol->setPixelConfigMpx3rx( devId );

}

void QCstmEqualization::Configuration(int devId, int THx, bool reset) {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    // Reset pixel configuration
    if ( reset ) spidrcontrol->resetPixelConfig();

    // All adjustment bits to zero
    SetAllAdjustmentBits(spidrcontrol, devId, 0x0, 0x0);

    spidrcontrol->setLutEnable( ! _mpx3gui->getConfig()->getLUTEnable() );
    spidrdaq->setLutEnable( _mpx3gui->getConfig()->getLUTEnable() );

    // Force operation mode to sequential
    spidrcontrol->setContRdWr( _deviceIndex, false );

    // OMR
    spidrcontrol->setPolarity( _deviceIndex, _mpx3gui->getConfig()->getPolarity() );		// true: Holes collection
    // For Equalization
    spidrcontrol->setEqThreshH( devId, true );
    //spidrcontrol->setInternalTestPulse( true ); 			// Internal tests pulse
    spidrcontrol->setColourMode( devId, _mpx3gui->getConfig()->getColourMode() ); 		// false = Fine Pitch
    spidrcontrol->setCsmSpm( devId, 0 );                    // Single Pixel mode

    // 00: SHGM  0
    // 10: HGM   2
    // 01: LGM   1
    // 11: SLGM  3
    spidrcontrol->setGainMode( devId, 3 ); // SuperLow Gain Mode for Equalization
    spidrdaq->setDecodeFrames( true );
    spidrcontrol->setPixelDepth( devId, 12 );
    spidrdaq->setPixelDepth( 12 );

    ////////////////////////////////////////////////////////////////////////////////

    // When equalizing the TH1 (threshold high)
    if ( THx == MPX3RX_DAC_THRESH_1 ) {
        spidrcontrol->setDiscCsmSpm( devId, 1 );		// DiscH used
    } else {
        spidrcontrol->setDiscCsmSpm( devId, 0 );		// DiscL used
    }

    // Packet size reports NOT IMPLEMENTED in the Leon software
    // spidrcontrol->setMaxPacketSize( 1024 );

    ///////////////////////////////////////////////////////////
    // Trigger config
    // Sequential R/W
    int trig_mode      = SHUTTERMODE_AUTO;     // Auto-trigger mode
    //! TODO Increase frequency and decreae trigger length to speed up the process?
    int trig_length_us = 5000;  // This time shouldn't be longer than the period defined by trig_freq_hz
    int trig_freq_hz   = 50;   // One trigger every 20ms
                                //! TODO Units do not match function call. Is it Hz or mhz?
    int nr_of_triggers = _nTriggers;    // This is the number of shutter open I get
    //int trig_pulse_count;
    spidrcontrol->setShutterTriggerConfig( trig_mode,
                                           trig_length_us,
                                           trig_freq_hz,
                                           nr_of_triggers );

    ///////////////////////////////////////////////////////////
    // Continues R/W
    // One counter at a time, but no dead-time
    // To implement

}

Mpx3EqualizationResults * QCstmEqualization::GetEqualizationResults(int chipIndex) {

    int nChips = _mpx3gui->getConfig()->getNDevicesSupported();
    if ( chipIndex < 0 || chipIndex > nChips - 1) return 0x0;

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

    // Now locate them in a layout
    // Prepare a QGrid layout for the different equalizations
    //if ( !_gridLayoutHistograms ) _gridLayoutHistograms = new QGridLayout();
    //_ui->horizontalLayoutEqHistos->addLayout( _gridLayoutHistograms );
    //_gridLayoutHistograms->update();


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
                cprop.color_g = 0;
                cprop.color_b = 127;

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
                cprop.color_r = 127;
                cprop.color_g = 0;
                cprop.color_b = 0;

                nbc->AppendSet( cprop );

                _adjchart_H.push_back( nbc );
            }

        }
    }


    // Now locate them in the grid layout as needed.
    // This will depend on the number of chips available.
    //DistributeAdjHistogramsInGridLayout();

}

void QCstmEqualization::DistributeAdjHistogramsInGridLayout(){


    int chipListSize = (int)_workChipsIndx.size();

    int nCols = ceil( (double)chipListSize / 2.);

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
            if ( bc != 0x0 ) { // i might be asking for a chart that is not available 'cause a chip is unresponsive
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


void QCstmEqualization::RewindEqualizationStructure(){

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

bool QCstmEqualization::makeTeaCoffeeDialog()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Equalisation information");
    msgBox.setText("This operation will take a long time, get some tea/coffee.\n\nIMPORTANT: Check polarity and activate bias voltage!");
    msgBox.addButton(QMessageBox::Ok);
    msgBox.addButton(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);

    if (msgBox.exec() == QMessageBox::Ok){
        return true;
    } else {
        return false;
    }
}

void QCstmEqualization::LoadEqualization(bool getPath, QString path) {

    if (path == "") {
        path = "config/";
    } //! Else leave it

    //! Non-default path, get the folder from a QFileDialog
    if (getPath){
        // Absolute folder path
        path = QFileDialog::getExistingDirectory(
                    this,
                    tr("Select a directory containing equalisation files (adj_* and mask_*)"),
                    QDir::currentPath(),
                    QFileDialog::ShowDirsOnly);
        path += "/";
        if( !path.isNull() ) {
            //! Very basic check to proceed or not by seeing if the bare minimum files exist (adj_0 and mask_0) in the given path
            QString testAdjFile = path + QString("adj_0");
            QString testMaskFile = path + QString("mask_0");

            if (!(QFileInfo::exists(testAdjFile) && QFileInfo::exists(testMaskFile))){
                //! Failed to find adj_0 and mask_0, the bare minimum for this to work
                //! Return with no warnings or prompts
                emit sig_statusBarAppend(tr("Nothing loaded from equalisation"), "orange");
                qDebug() << "[INFO] Nothing loaded from equalisation, doing nothing.";
                return;
            }

        } else {
            //! Nothing selected, return with no prompt or warning
            qDebug() << "[INFO] Null path input, doing nothing.";
            return;
        }
    }

    int nChips = _mpx3gui->getConfig()->getNDevicesSupported();
    QProgressDialog pd(tr("Loading adjustment bits..."), tr("Cancel"), 0, nChips, this);
    pd.setCancelButton( 0 ); // No cancel button
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

        // And clear all previous adjustments
        // In case an equalisation was done in the same session
        //ClearAllAdjustmentBits();

        //! Check if the device is alive
        if ( ! _mpx3gui->getConfig()->detectorResponds( i ) ) {
            continue;
        }

        // Go to this device index
        //ChangeDeviceIndex( i );
        //_ui->devIdSpinBox->setValue( i );

        //! Builg strings for adj and mask file paths
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

        //if ( GetAdjBarChart(i, Mpx3EqualizationResults::__ADJ_H) != nullptr ) {
        //    GetAdjBarChart(i, Mpx3EqualizationResults::__ADJ_H)->show();
        //}

        // And talk to the hardware loading also the mask
        SetAllAdjustmentBits( _mpx3gui->GetSpidrController(), i, true );

        // Show the related histograms
//        if ( GetAdjBarChart(i, Mpx3EqualizationResults::__ADJ_L) != nullptr ) {
//            GetAdjBarChart(i, Mpx3EqualizationResults::__ADJ_L)->show();
//            //GetAdjBarChart(i, Mpx3EqualizationResults::__ADJ_L)->fitToHeight();
//        }

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
    emit sig_statusBarClean();
    emit sig_statusBarAppend(msg, "green");

    // And show
    ShowEqualization( _equalizationShow );

}

void QCstmEqualization::ShowEqualization(Mpx3EqualizationResults::lowHighSel sel) {

    // Use a data set to put the adj matrixes together
    if ( ! _resdataset ) _resdataset = new Dataset ( __matrix_size_x, __matrix_size_y, _nchipsX*_nchipsY );
    _resdataset->clear();

    int nChips = _mpx3gui->getConfig()->getNDevicesSupported();

    for(int i = 0 ; i < nChips ; i++) {

        // And clear all previous adjustments
        // In case an equalisation was done in the same session
        //ClearAllAdjustmentBits();

        // Check if the device is alive
        if ( ! _mpx3gui->getConfig()->detectorResponds( i ) ) {
            continue;
        }

        // Get adj matrix for one chip
        int * adj_matrix = _eqMap[i]->GetAdjustementMatrix( sel );
        //for( int j = 0 ; j < __matrix_size ; j++ ){
        //	adj_matrix[j] = j;
        //}
        // Stack
        _resdataset->setFrame(adj_matrix, i, 0);

    }

    // Once the frame is complete, extract info
    int * fulladjdata = _resdataset->getLayer( 0 );
    // Plot
    UpdateHeatMap(fulladjdata, _fullsize_x, _fullsize_y);
    _ui->_intermediatePlot->replot( QCustomPlot::rpQueued );

    /*
    // Display the equalisation
    int * adj_matrix = _eqMap[i]->GetAdjustementMatrix();

    for (int j = 0 ; j < __matrix_size_x * __matrix_size_y ; j++) {
        displaymatrix[i*(__matrix_size_x * __matrix_size_y) + j ] = adj_matrix[j];
    }
    _ui->_intermediatePlot->clear();
    //int lastActiveFrame = _ui->_intermediatePlot->GetLastActive();
    QRectF boundingBoxF =  _mpx3gui->getDataset()->computeBoundingBox();
    //QSize boundingBox =QSize(boundingBoxF.size().width()*_mpx3gui->x(), boundingBoxF.size().height()*_mpx3gui->y());

    QSize boundingBox = QSize(boundingBoxF.width()*_mpx3gui->getDataset()->x(), boundingBoxF.height()*_mpx3gui->getDataset()->y());

    cout << boundingBox.width() << ", " << boundingBox.height() << endl;
    _ui->_intermediatePlot->addData( displaymatrix, boundingBox.width(), boundingBox.height() );
    _ui->_intermediatePlot->setActive( 0 );
     */

}

void QCstmEqualization::SetupSignalsAndSlots() {

    connect(_ui->nHitsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setNHits(int)) );

    connect( _ui->_startEq, SIGNAL(clicked()), this, SLOT(StartEqualizationSingleChip()) );
    connect( _ui->_startEqAll, SIGNAL(clicked()), this, SLOT(StartEqualizationAllChips()) );
    connect( _ui->_startEqAllSequential, SIGNAL(clicked()), this, SLOT(StartEqualizationSequentialSingleChips()) );
    connect( _ui->_stopEq, SIGNAL(clicked()), this, SLOT(StopEqualization()) );

    //connect(_ui->_intermediatePlot, SIGNAL(mouseOverChanged(QString)), _ui->mouseHoveLabel, SLOT(setText(QString)));
    //_ui->_statusLabel->setStyleSheet("QLabel { background-color : gray; color : black; }");
    //_ui->_histoWidget->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );

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

    //cout << "react" << endl;

    if( (int)_equalizationShow == sel ) {
        ShowEqualization(_equalizationShow);
        return;
    }

    // see the order in the constructor
    if ( sel == 0 ) _equalizationShow = Mpx3EqualizationResults::__ADJ_L;
    if ( sel == 1 ) _equalizationShow = Mpx3EqualizationResults::__ADJ_H;

    //cout << "draw : " << _equalizationShow << endl;

    // and show again
    ShowEqualization(_equalizationShow);

}

void QCstmEqualization::setEqualizationTHLTHH(int sel) {
    if ( _equalizationCombination == sel ) return;
    _equalizationCombination = sel;
}

void QCstmEqualization::setEqualizationTHLType(int sel) {
    if ( _equalizationType == sel ) return;
    _equalizationType = sel;
}

void QCstmEqualization::ChangeNTriggers( int nTriggers ) {
    if( nTriggers < 0 ) return; // can't really happen cause the SpinBox has been limited
    _nTriggers = nTriggers;
}

void QCstmEqualization::ChangeDeviceIndex( int index ) {
    if( index < 0 ) return; // can't really happen cause the SpinBox has been limited
    _deviceIndex = index;
}

void QCstmEqualization::ChangeSpacing( int spacing ) {
    if( spacing < 0 ) return;
    _spacing = spacing;
}

void QCstmEqualization::ChangeMin(int min) {

    // Handle the mistakes
    // 1) Negative value
    // 2) min and max equal
    // 3) not enough range for a scan
    if( min < 0 || min == _maxScanTHL || qAbs(min - _maxScanTHL) <= _stepScan ) {
        _ui->eqMinSpinBox->setValue( _minScanTHL );
        return;
    }

    _minScanTHL = min;

    // decide if the scan is descendant
    if ( _minScanTHL > _maxScanTHL ) _scanDescendant = true;
    else _scanDescendant = false;

}
void QCstmEqualization::ChangeMax(int max) {

    if( max < 0 || max == _minScanTHL || qAbs(max - _minScanTHL) <= _stepScan ) {
        _ui->eqMaxSpinBox->setValue( _maxScanTHL );
        return;
    }

    _maxScanTHL = max;

    // decide if the scan is descendant
    if ( _maxScanTHL < _minScanTHL ) _scanDescendant = true;
    else _scanDescendant = false;

}

void QCstmEqualization::ChangeStep(int step) {
    if( step < 0 ) return;
    _stepScan = step;
}

void QCstmEqualization::StopEqualization() {

    //GetUI()->_histoWidget->Clean();
    emit stop_data_taking_thread();

}

void QCstmEqualization::CleanEqualization() {

    // Clean histograms
    //GetUI()->_histoWidget->Clean();

    // Clean text browser
    //_ui->eqTextBrowser->clear();

}

void QCstmEqualization::ConnectionStatusChanged(bool conn) {

    if ( conn ) {

        setWindowWidgetsStatus( win_status::connected );

        // Select first device ID in case of a single chip equalisation
        QVector<int> activeDevices = _mpx3gui->getConfig()->getActiveDevices();
        _ui->devIdSpinBox->setValue( activeDevices.at( 0 ) );


    } else {

        setWindowWidgetsStatus( win_status::disconnected );

    }

    // WARNING
    // This could imply talking to the chip at the same time than the dacscontrol or
    //  other widget.  Careful !

    //_ui->_statusLabel->setText("Connected");
    //_ui->_statusLabel->setStyleSheet("QLabel { background-color : blue; color : white; }");

}

void QCstmEqualization::AppendToTextBrowser(QString s){

    _ui->eqTextBrowser->append( s );

}

void QCstmEqualization::ClearTextBrowser(){
    _ui->eqTextBrowser->clear();
}

void QCstmEqualization::timerEvent( QTimerEvent * /*evt*/ ) {


    //double val = rand() % 510 + 1 ;
    //cout << "append : " << val << endl;

    // filling the histo with random numbers
    //_ui->_histoWidget->PushBackToSet( 0, val );
    ////QCPBarData bin_content = (*(_ui->_histoWidget->GetDataSet(0)->data()))[val];

    //double cont = (*dmap)[val];
    ////_ui->_histoWidget->GetDataSet(0)->addData( bin_content.key , bin_content.value +1 );
    //_ui->_histoWidget->DumpData();
    //_ui->_histoWidget->clearGraphs();
    ////_ui->_histoWidget->replot();
    //_ui->_histoWidget->update();

    //_ui->_histoWidget->SetValueInSet(0, 100);

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

/*
 * The adj binary files come one per chip.  First 64k correspond to THL and next 64k to THH
 */
bool Mpx3EqualizationResults::ReadAdjBinaryFile(QString fn) {

    qDebug() << "Mpx3EqualizationResults::ReadAdjBinaryFile   [INFO] Read Adj binary file: " << fn.toStdString().c_str();
    QFile file(fn);
    if ( !file.open(QIODevice::ReadOnly) ) {
        return false;
    }

    //! Read temporarily the entire chunk of 128k
    QByteArray temp_pixId_Adj_X;
    temp_pixId_Adj_X = file.readAll();
    file.close();
    int readSize = temp_pixId_Adj_X.size();
    qDebug() << "Mpx3EqualizationResults::ReadAdjBinaryFile   [INFO] read " << temp_pixId_Adj_X.size() << " bytes";

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

    //ofstream fd;
    cout << "Mpx3EqualizationResults::WriteAdjBinaryFile    [INFO] Writing adj file to: " << fn.toStdString() << endl;

    QFile file(fn);
    if (!file.open(QIODevice::WriteOnly)) return false;
    //if ( sel == __ADJ_L ) file.write(_pixId_Adj_L);
    //if ( sel == __ADJ_H ) file.write(_pixId_Adj_H);

    qDebug() << "Mpx3EqualizationResults::WriteAdjBinaryFile       Write L: " << _pixId_Adj_L.size() << endl;
    qDebug() << "Mpx3EqualizationResults::WriteAdjBinaryFile       Write H: " << _pixId_Adj_H.size() << endl;

    qint64 bytesWritten = file.write(_pixId_Adj_L);
    bytesWritten += file.write(_pixId_Adj_H);

    qDebug() << "Mpx3EqualizationResults::WriteAdjBinaryFile       Bytes written: " << bytesWritten << endl;

    /*fd.open (fn.toStdString().c_str(), ios::out | ios::binary);
    qDebug() << "Mpx3EqualizationResults::WriteAdjBinaryFile   Writing adjustment matrix to: " << fn.toStdString() << endl;
    // Each adjustment value is written as 8 bits val.  Each value is actually 5 bits.
    char buffer;
    for( int j = 0 ; j < __matrix_size ; j++ ){
        buffer = (char) ( _pixId_Adj[j] & 0xFF );
        fd.write( &buffer, 1 );   // _pixId_Adj[j];
    }

    fd.close();*/
    file.close();


    return true;
}

bool Mpx3EqualizationResults::WriteMaskBinaryFile(QString fn) {

    ofstream fd;
    fd.open (fn.toStdString().c_str(), ios::out);
    cout << "Mpx3EqualizationResults::WriteMaskBinaryFile   Writing mask file to: " << fn.toStdString() << endl;

    for ( int i = 0 ; i < __matrix_size ; i++ ) {
        //! If equalisation status is failed, then write the flattened coordinate to file.
        if ( _eqStatus_H[i] > __EQUALIZED || _eqStatus_L[i] > __EQUALIZED ) fd << i << endl;
    }

    return true;
}

bool Mpx3EqualizationResults::ReadMaskBinaryFile(QString fn) {

    ifstream fd;
    fd.open (fn.toStdString().c_str(), ios::out);
    qDebug() << "Mpx3EqualizationResults::ReadMaskBinaryFile   [MASK] Reading mask file from: " << fn.toStdString().c_str();

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

            if ( i < 2 ) {
                qDebug() << eta_Adj_THL << ", " << pixel_cut << ", " << adj << endl;
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
    qDebug() << "Mpx3EqualizationResults::ExtrapolateAdjToTarget [INFO] Extrapolation formula used. Pixels set at max adj : " << atmax << ". Pixels set at min adj : " << atmin << endl;

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

//void QCstmEqualization::on_heatmapCombobox_currentIndexChanged(const QString & /*arg1*/)//would be more elegant to do with signals and slots, but would require either specalizing the combobox, or making the heatmapMap globally visible.
//{
//}

//void QCstmEqualization::on_openfileButton_clicked()
//{
//}
