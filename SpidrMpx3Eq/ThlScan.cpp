/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#include "ThlScan.h"
#include "SpidrController.h"
#include "SpidrDaq.h"

#include "barchart.h"
#include "qcstmplotheatmap.h"
#include "qcstmequalization.h"
#include "ui_qcstmequalization.h"
#include "mpx3dacsdescr.h"

#include "mpx3defs.h"
#include "mpx3eq_common.h"
#include "ui_mpx3gui.h"

#include <iostream>
#include <iterator>
#include <map>
#include <vector>
using namespace std;

#include <fstream>

ThlScan::ThlScan(Mpx3GUI * mpx3gui, QCstmEqualization * ptr) {

    // keep these pointers
    _mpx3gui = mpx3gui;
    _equalization = ptr;
    _heatmap = nullptr;
    _spidrcontrol = nullptr; // Assuming no connection yet
    _spidrdaq = nullptr;     // Assuming no connection yet
    _frameId = 0;
    _adjType = __adjust_to_global;
    _stop = false;
    _scanType = __BASIC_SCAN;
    _fineTuningPixelsEqualized.clear();
    _results.clear();
    _maskedSet.clear();
    _plotdata = nullptr;
    _dataset = nullptr;

    _detectedScanBoundary_L = (1 << MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_0].bits);
    _detectedScanBoundary_H = 0;

    // Set to true for special scans
    _stopWhenPlateau = false;

    // number of reactive pixels
    _nReactivePixels = 0;

    _nchipsX = 2; //_mpx3gui->getDataset()->getNChipsX();
    _nchipsY = 2; //_mpx3gui->getDataset()->getNChipsY();
    // When one chips is connected the dataset returns 2,1 (which is good)
    _fullsize_x = __matrix_size_x * _nchipsX;
    _fullsize_y = __matrix_size_y * _nchipsY;

    // Extract the information needed when the thread will be launched
    _spacing = _equalization->GetSpacing();
    SetMinScan(); // default min max
    SetMaxScan();
    _stepScan = _equalization->GetStepScan();
    _deviceIndex = _equalization->GetDeviceIndex();

}

void ThlScan::SetWorkChipIndexes(vector<int> v, vector<equalizationSteeringInfo *> st) {

    _workChipsIndx = v;

    InitializeScanResults(st);

}

void ThlScan::InitializeScanResults(vector<equalizationSteeringInfo *> st) {

    vector<int>::iterator i  = _workChipsIndx.begin();
    vector<int>::iterator iE = _workChipsIndx.end();

    // One result object per chip to be equalized
    for (int i = 0 ; i < (int)_workChipsIndx.size() ; i++ ) {

        ScanResults * sr = new ScanResults;
        sr->weighted_arithmetic_mean = 0.;
        sr->sigma = 0.;
        sr->chipIndx = _workChipsIndx[i];

        // use the test value when adjusting to global
        sr->DAC_DISC_setting = st[i]->currentDAC_DISC_OptValue;

        // set global adjustment
        sr->global_adj = st[i]->globalAdj;

        sr->equalisationTarget = st[i]->GetEqualizationTarget();

        _results.push_back( sr );

    }

}

ScanResults * ThlScan::GetScanResults(int chipIdx) {

    // There should be as results objects as chips to be equalized
    if ( _workChipsIndx.size() != _results.size() ) return nullptr;

    // Find the index
    for (int i = 0 ; i < (int)_workChipsIndx.size() ; i++ ) {
        // return the corresponding results Ptr
        if ( _workChipsIndx[i] == chipIdx ) return _results[i];
    }

    return nullptr;
}

void ThlScan::ConnectToHardware(SpidrController * sc, SpidrDaq * sd) {

    _spidrcontrol = sc;
    _spidrdaq = sd;

    // I need to do this here and not when already running the thread
    // Get the IP source address (SPIDR network interface) from the already connected SPIDR module.
    if( _spidrcontrol ) { _spidrcontrol->getIpAddrSrc( 0, &_srcAddr ); }
    else { _srcAddr = 0; }

    // get the heatmap from the GUI
    _heatmap = _equalization->GetUI()->_intermediatePlot;

}

/**
 * Clear data structures and get ready for a new Scan
 */
void ThlScan::RewindData(int full_sizex, int full_sizey) {

    if ( !_pixelCountsMap.empty() ) {
        _pixelCountsMap.clear();
    }
    if ( !_pixelReactiveTHL.empty() ) {
        _pixelReactiveTHL.clear();
    }
    _nReactivePixels = 0;

    // Create entries at 0 for the whole matric
    for ( int i = 0 ; i < full_sizex*full_sizey ; i++ ) {
        _pixelCountsMap[i] = __NOT_TESTED_YET;
        _pixelReactiveTHL[i] = __UNDEFINED;
    }

    //cout << "[INFO] ThlScan::_pixelCountsMap rewinded. Contains " << _pixelCountsMap.size() << " elements." << endl;
    //cout << "[INFO] ThlScan::_pixelReactiveTHL rewinded. Contains " << _pixelReactiveTHL.size() << " elements." << endl;

    // Nothing should be masked
    _maskedSet.clear();

}


void ThlScan::DeliverPreliminaryEqualization(int devId, int currentDAC_DISC, Mpx3EqualizationResults * eq, int global_adj) {

    Mpx3EqualizationResults::lowHighSel sel = Mpx3EqualizationResults::__ADJ_L;
    if ( currentDAC_DISC == MPX3RX_DAC_DISC_L ) sel = Mpx3EqualizationResults::__ADJ_L;
    if ( currentDAC_DISC == MPX3RX_DAC_DISC_H ) sel = Mpx3EqualizationResults::__ADJ_H;

    // Fill the preliminary results
    // The reactive THL comes in the whole matrix
    // [0](0 ... 256*256), [0](0 ... 256*256), [0](0 ... 256*256), [3](0 ... 256*256)
    // 1) I need the find the right range.
    // 2) The results are divided by chip, so I need a second counter.
    int inChipCntr = 0;

    for ( int i = devId*__matrix_size ; i < (devId+1)*__matrix_size ; i++ ) {

        // First the adjustment is the same for all
        eq->SetPixelAdj(inChipCntr, global_adj, sel );

        // And here is the reactive threshold
        eq->SetPixelReactiveThl(inChipCntr, _pixelReactiveTHL[i], sel );

        inChipCntr++;
    }

}

void ThlScan::DoScan(int dac_code, int setId, int DAC_Disc_code, int numberOfLoops, bool blindScan, bool testPulses) {

    _dac_code = dac_code;
    _setId = setId;
    _DAC_Disc_code = DAC_Disc_code;
    _numberOfLoops = numberOfLoops;
    _blindScan = blindScan;
    _testPulses = testPulses;

}

void ThlScan::on_stop_data_taking_thread() {

    // Used to properly stop the data taking thread
    _stop = true;

}



void ThlScan::run() {

    // The normal scan starting from scratch
    if(_scanType == __BASIC_SCAN) EqualizationScan( );

    // This one is a fine tuning scan
    if(_scanType == __FINE_TUNING1_SCAN) {
        FineTuning( );
    }
}

void ThlScan::setEqualisationTargets()
{
    //! Find the mean of the thresholds from the map _pixelReactiveTHL
    //! Set class variable (ScanResults->equalisationTarget to new equalisation target

    std::map<int, int>::iterator it = _pixelReactiveTHL.begin();
    int sum = 0;
    int activePixels = 0;

    while (it != _pixelReactiveTHL.end())
    {
        // Accessing KEY from element pointed by it.
        //int pixelID = it->first;
        // If I wanted to get the pixelIDs as well

        // Accessing VALUE from element pointed by it.
        int turnOnThreshold = it->second;
        // The turn on thresholds

        //qDebug() << pixelID << " :: " << turnOnThreshold << "\n";

        //! Ignore my initialised shitty -1 values
        if (turnOnThreshold >= 0) {
            sum += turnOnThreshold;
            activePixels++;
        }
        it++;
    }

    vector<ScanResults *>::iterator i  = _results.begin();
    vector<ScanResults *>::iterator iE = _results.end();
    int equalisationTarget = int(sum/activePixels);

    for ( ; i != iE ; i++ ) {
        (*i)->equalisationTarget = equalisationTarget;
    }

    qDebug() << "[INFO]\tTest pulses average turn on threshold :" << equalisationTarget;
}

/**
 * Nsigma:
 * returns number of pixels adjusted.
 */
void ThlScan::FineTuning() {

    //! Initialisation ---------------------------------------------------------
    //! Is the SPIDR connected still?

    // Open a new temporary connection to the SPIDR to avoid collisions to the main one
    int ipaddr[4] = { 1, 1, 168, 192 };
    if ( _srcAddr != 0 ) {
        ipaddr[3] = (_srcAddr >> 24) & 0xFF;
        ipaddr[2] = (_srcAddr >> 16) & 0xFF;
        ipaddr[1] = (_srcAddr >>  8) & 0xFF;
        ipaddr[0] = (_srcAddr >>  0) & 0xFF;
    }
    SpidrController * spidrcontrol = new SpidrController( ipaddr[3], ipaddr[2], ipaddr[1], ipaddr[0] );

    if ( !spidrcontrol || !spidrcontrol->isConnected() ) {
        qDebug() << "[ERROR]\tDevice not connected\n";
        return;
    }

    //! Make a new dataset just to handle the assembling of compound frames
    if ( !_dataset ) _dataset = new Dataset ( __matrix_size_x, __matrix_size_y, _nchipsX*_nchipsY );

    //! Rewind local counters
    _dataset->clear();

    //! Note: No REWINDATA in FineTuning


    //! Extract all the pixels not responding at 'exactly' the target THL value.
    //! The reactive THL information is available because this scan is the
    //!   same object used for the on-extrapolation scan.
    _scheduledForFineTuning = ExtractPixelsNotOnTarget();


    //! Send configuration to the chip
    for ( unsigned int di = 0 ; di < _workChipsIndx.size() ; di++ ) {

        if ( ! _mpx3gui->getConfig()->detectorResponds( _workChipsIndx[di] ) ) continue;

        //! Send the adj bits
        _equalization->SetAllAdjustmentBits(spidrcontrol, _workChipsIndx[di], false, _testPulses);

        //! While equalizing one threshold the other should be set at a high value
        //!   to keep that circuit from reacting.  Set it at ~100
        //! TODO Check this number with Rafa
        if ( _DAC_Disc_code == MPX3RX_DAC_DISC_L ) {
            SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_1, __low_but_above_noise_threshold );
            if ( _mpx3gui->getConfig()->getColourMode() ) {
                SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_3, __low_but_above_noise_threshold );
                SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_5, __low_but_above_noise_threshold );
                SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_7, __low_but_above_noise_threshold );
            }
        } else if (  _DAC_Disc_code == MPX3RX_DAC_DISC_H ) {
            SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_0, __low_but_above_noise_threshold );
            if ( _mpx3gui->getConfig()->getColourMode() ) {
                SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_2, __low_but_above_noise_threshold );
                SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_4, __low_but_above_noise_threshold );
                SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_6, __low_but_above_noise_threshold );
            }
        }

    }

    //! Setup signals, 2 for GUI, 1 for data taking
    connect( this, SIGNAL( UpdateChartSignal(int, int, int) ), this, SLOT( UpdateChart(int, int, int) ) );
    connect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );
    connect(_equalization, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread())); // stop signal from qcstmglvis

    //! Start with the adjustments currently reacting
    //FillAdjReactTHLHistory();
    //DumpAdjReactTHLHistory( 10 );

    int processedLoops = 0;
    bool finishScan = false;
    bool doReadFrames = false;
    int progressMax = _numberOfLoops;
    int idDataFetch = _mpx3gui->getConfig()->getDataBufferId( _deviceIndex ); // Note: The data buffer id doesn't necessarily corresponds to _deviceIndex

    //_stepScan = 1;

    if ( _numberOfLoops < 0 ) progressMax = _spacing * _spacing;

    qDebug() << "[INFO] [Fine Tuning] Run a Scan. devIndex:" << _deviceIndex << " | databuffer:" << idDataFetch << "\n";

    file_fineTuningStats.open("log_fineTuningStats.csv", std::ios::app);
    file_fineTuningStats << "Changed adj bits, Not equalised, Scheduled, Done, Adj OoB, Non-reactive, Stuck left, Never react, adj-1, > target, < target, OoB \n";

    //! End of initialisation --------------------------------------------------



    //! Start fine tuning loop -------------------------------------------------

    // Decide when to stop trying different adj values for this particular mask
    int adjLoops = 0;
    while ( ! AdjScanCompleted(_scheduledForFineTuning, _maskedSet)
            && (adjLoops < _equalization->GetFineTuningLoops()) ) {

        //! Stop when a number of loops has been reached
        //! Compared to the GUI box value.
        adjLoops++;

        file_fineTuningStats << adjLoops;
        if (_testPulses) {
            _spacing = int(_equalization->testPulseEqualisationDialog->getPixelSpacing());
            progressMax = _spacing * _spacing;
        }

        //! Iterate through x then y
        for ( int maskOffsetItr_x = 0 ; maskOffsetItr_x < _spacing ; maskOffsetItr_x++ ) {
            for ( int maskOffsetItr_y = 0 ; maskOffsetItr_y < _spacing ; maskOffsetItr_y++ ) {

                // GUI stuff ---------------------------------------------------
                QString loopProgressS;
                loopProgressS =  QString::number( maskOffsetItr_x * _spacing + maskOffsetItr_y + 1, 'd', 0 );
                loopProgressS += "/";
                loopProgressS += QString::number( progressMax, 'd', 0 );
                connect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelLoopProgress, SLOT( setText(QString)) );
                fillText( loopProgressS );
                disconnect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelLoopProgress, SLOT( setText(QString)) );
                // -------------------------------------------------------------

                // Set a mask
                int nMasked = 0, pmasked = 0;

                for ( unsigned long devId = 0 ; devId < _workChipsIndx.size() ; devId++ ) {
                    if ( ! SetEqualizationMask(spidrcontrol, _workChipsIndx[devId], _spacing, maskOffsetItr_x, maskOffsetItr_y, &pmasked) ) {
                        qDebug() << "[FAIL]\tCould not set equalisation mask";
                        return;
                    }
                    nMasked += pmasked;
                }
                //qDebug() << "[INFO] [Fine Tuning] offset_x: " << maskOffsetItr_x << ", offset_y:" << maskOffsetItr_y
                //     <<  " | N pixels unmasked = " << int(_workChipsIndx.size()*__matrix_size) - nMasked << "\n";

                // GUI stuff ---------------------------------------------------
                QString ftLoopProgressS = QString::number( adjLoops, 'd', 0 );
                ftLoopProgressS += "/";
                ftLoopProgressS += QString::number( _equalization->GetFineTuningLoops(), 'd', 0 );
                connect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelFineTuningLoopProgress, SLOT( setText(QString)) );
                fillText( ftLoopProgressS );
                disconnect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelFineTuningLoopProgress, SLOT( setText(QString)) );
                // -------------------------------------------------------------

                //! Shift adjustment bits around according to if they're above or below the target
                int nNotInMask = ShiftAdjustments( _scheduledForFineTuning, _maskedSet );


                for ( unsigned long di = 0 ; di < _workChipsIndx.size(); di++ ) {
                    _equalization->SetAllAdjustmentBits(spidrcontrol, _workChipsIndx[di], false, _testPulses);
                }

                //! Reset reactions counters to Thl_Status::__NOT_TESTED_YET
                //!    after making the adjustment bit shift
                RewindReactionCounters( _scheduledForFineTuning );

                //! Use the limits based on chips
                QList<int> scanEqTargets;
                QList<double> scanSigmas;

                for ( int devId = 0 ; devId < int(_workChipsIndx.size()) ; devId++ ) {
                    if (GetScanResults(devId) != nullptr ) {
                        scanEqTargets.append(GetScanResults(devId)->equalisationTarget);
                        scanSigmas.append(GetScanResults(devId)->sigma);
                    }
                }
                qSort(scanEqTargets.begin(), scanEqTargets.end());
                qSort(scanSigmas.begin(), scanSigmas.end());

                _minScan =  scanEqTargets.first() - int(ceil(3.7*scanSigmas.last()));
                _maxScan =  scanEqTargets.last()  + int(ceil(3.7*scanSigmas.last()));
                if ( _minScan < 0 ) {
                    _minScan = 0;
                }
                if (_maxScan > 511 ) {
                    _maxScan = 511;
                }
                // qDebug() << "[INFO]\t Scanning range" << _minScan << "-" << _maxScan;

                //! Scan iterator observing direction
                _pixelReactiveInScan = 0;
                _thlItr = _minScan;

                if ( _equalization->isScanDescendant() ) _thlItr = _maxScan;
                bool scanContinue = true;

                //! Scan over thresholds
                for ( ; scanContinue ; ) {

                    // GUI stuff -----------------------------------------------
                    QString thlLabelS = QString::number( _thlItr, 'd', 0 );
                    // Send signal to Labels.  Making connections one by one.
                    connect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelTHLCurrentValue, SLOT( setText(QString)) );
                    fillText( thlLabelS );
                    disconnect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelTHLCurrentValue, SLOT( setText(QString)) );
                    // ---------------------------------------------------------

                    // Set the threshold on all chips
                    for ( unsigned long devId = 0 ; devId < _workChipsIndx.size() ; devId++ ) {
                        SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[devId], _dac_code, _thlItr );
                        if ( _mpx3gui->getConfig()->getColourMode() && _dac_code == MPX3RX_DAC_THRESH_0 ) {
                            SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[devId], MPX3RX_DAC_THRESH_2, _thlItr );
                            SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[devId], MPX3RX_DAC_THRESH_4, _thlItr );
                            SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[devId], MPX3RX_DAC_THRESH_6, _thlItr );
                        } else if ( _mpx3gui->getConfig()->getColourMode() && _dac_code == MPX3RX_DAC_THRESH_1 ) {
                            SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[devId], MPX3RX_DAC_THRESH_3, _thlItr );
                            SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[devId], MPX3RX_DAC_THRESH_5, _thlItr );
                            SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[devId], MPX3RX_DAC_THRESH_7, _thlItr );
                        }
                    }

                    spidrcontrol->startAutoTrigger(); // Start the trigger as configured

                    //! See if there is a frame available.
                    //! I should get as many frames as triggers
                    //! Assume the frame won't come
                    doReadFrames = false;

                    //! This needs to be high because of Debug mode running slow AF
                    while ( _spidrdaq->hasFrame( 100 ) ) {

                        doReadFrames = true;// A frame is here

                        //! Check quality
                        //! The total number of lost packets/pixels detected in the current frame
                        FrameSet *frameSet = _spidrdaq->getFrameSet();
                        if  (frameSet->pixelsLost() != 0) {
                            doReadFrames = false;
                        }

                        if ( doReadFrames ) {

                            int nChips = _mpx3gui->getConfig()->getNDevicesSupported();
                            // Go through all chips avoiding those not present
                            for ( int idx = 0 ; idx < nChips ; idx++ ) {
                                if ( ! _mpx3gui->getConfig()->detectorResponds( idx ) ) continue;

                                idDataFetch = _mpx3gui->getConfig()->getDataBufferId( idx );

                                _dataset->setFrame(frameSet, idx, 0); // Stack
                            }

                            // Once the frame is complete, extract info
                            _data = _dataset->getLayer( 0 );
                            // I am assuming that all the frames have the same size in bytes
                            _pixelReactiveInScan += ExtractScanInfo( _data, MPX_PIXELS * 4 * _nchipsX*_nchipsY, _thlItr );
                        }

                        _spidrdaq->releaseFrame(frameSet); // Release frame

                        if ( doReadFrames ) {
                            FillAdjReactTHLHistory(); // Keep track of the <adj, reactTHL> pairs
                            UpdateHeatMapSignal(_fullsize_x, _fullsize_y);
                        }
                    }

                    // GUI stuff -----------------------------------------------
                    QString reactiveLabelS;
                    reactiveLabelS = QString::number( _pixelReactiveInScan, 'd', 0 );
                    reactiveLabelS += "/";
                    reactiveLabelS += QString::number( nNotInMask, 'd', 0 );
                    // Send signal to Labels.  Making connections one by one.
                    connect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelNPixelsReactive, SLOT( setText(QString)) );
                    fillText( reactiveLabelS );
                    disconnect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelNPixelsReactive, SLOT( setText(QString)) );
                    // ---------------------------------------------------------

                    // Last scan boundaries
                    // This information is for the next scan
                    if ( _pixelReactiveInScan != 0 ) {
                        if ( _thlItr < _detectedScanBoundary_L ) _detectedScanBoundary_L = _thlItr;
                        if ( _thlItr > _detectedScanBoundary_H ) _detectedScanBoundary_H = _thlItr;
                    }

                    //! Increment or decrement
                    if( _equalization->isScanDescendant() ) _thlItr -= _stepScan;
                    else _thlItr += _stepScan;

                    //! Check termination conditions
                    if ( _equalization->isScanDescendant() ) {
                        if ( _thlItr >= _minScan ) scanContinue = true;
                        else scanContinue = false;
                    } else {
                        if ( _thlItr <= _maxScan ) scanContinue = true;
                        else scanContinue = false;
                    }
                }

                //DumpAdjReactTHLHistory( 10 );

                //! Check finish condition - a full 'spacing loop' completion
                processedLoops++;
                if( _numberOfLoops > 0 && _numberOfLoops == processedLoops ) finishScan = true;

                if( finishScan || _stop ) break;

            } // AdjScanCompleted

            if( finishScan || _stop ) break;
        }
    }

    //------------------------------ Done --------------------------------------
    //! Now for the list of pixels which need rework, select the best adj value from the history
    if ( ! _stop ) {

        SelectBestAdjFromHistory( 10 );

        //! Send the new configuration to the chip
        for ( int di = 0 ; di < (int)_workChipsIndx.size() ; di++ ) {
            _equalization->SetAllAdjustmentBits(spidrcontrol, _workChipsIndx[di], false, false);
            spidrcontrol->setInternalTestPulse(di, false);
        }

    }

    //! Cleanup ----------------------------------------------------------------

    //! Disconnect the signals we made before
    disconnect( this, SIGNAL( UpdateChartSignal(int, int, int) ), this, SLOT( UpdateChart(int, int, int) ) );
    disconnect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );
    disconnect( _equalization, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread()) );

    file_fineTuningStats.close();

    _stop = false; // in case the thread is used again

    // delete the extra space to plot.
    if ( _plotdata ) {
        delete [] _plotdata;
        _plotdata = nullptr;
    }

    // the dataset used here for frames stichting
    delete _dataset;
    _dataset = nullptr;

    // close the connection
    delete spidrcontrol;
}

void ThlScan::SelectBestAdjFromHistory(int showHeadAndTail) {

    // 1) Select the closest response to the equalization target
    // 2) if two values at the same distance are found at the right and left
    //    side, take the right side always (for no particular reason, but gotta be consistent).

    // select THx
    Mpx3EqualizationResults::lowHighSel sel;
    if ( _DAC_Disc_code == MPX3RX_DAC_DISC_L ) sel = Mpx3EqualizationResults::__ADJ_L;
    if ( _DAC_Disc_code == MPX3RX_DAC_DISC_H ) sel = Mpx3EqualizationResults::__ADJ_H;

    // Loop over the pixels scheduled for Fine tuning
    set<int>::iterator i = _scheduledForFineTuning.begin();
    set<int>::iterator iE = _scheduledForFineTuning.end();

    qDebug() << "[INFO]\t Selecting bests adjustments" << "\n" << "       ";
    int cntr = 0;
    int chipId = 0;
    for ( ; i != iE ; i++ ) {

        chipId = PixelBelongsToChip( *i );

        // Take a pixel and look at its history
        vector< pair<int, int> > pixHistory = _adjReactiveTHLFineTuning[*i];
        int historySize = (int) pixHistory.size();

        // Find the closes to eq target
        // It can happen that two points at the right and left side of eqTarget
        //  are to be found at the same distance from the target.
        // Calculate first all distances to target in history and see if this happens
        vector<int> distancesToTarget;
        for (int hI = 0 ; hI < historySize ; hI++ ) {
            distancesToTarget.push_back( pixHistory[hI].second - GetScanResults(chipId)->equalisationTarget );
        }
        // Check if there is more than one point at the same distance and at opposite sides
        // I will use a simple sort algorithm (these vectors have at most a size of 31).
        int minDistanceToTarget = 511;
        int minDistanceIndx = -1;
        for (int j = 0 ; j < historySize ; j++ ) {
            // here look for the smallest distance
            if ( qAbs( distancesToTarget[j] ) < minDistanceToTarget ) {
                minDistanceToTarget = qAbs( distancesToTarget[j] );
                minDistanceIndx = j;
            }
        }

        for (int j = 0 ; j < historySize ; j++ ) {
            for (int k = j+1 ; k < historySize ; k++ ) {

                if (
                        ( qAbs( distancesToTarget[j] ) ==  qAbs( distancesToTarget[k] ) ) 	// same distance
                        &&
                        (distancesToTarget[j] * distancesToTarget[k]) < 0 					// at opposite sides
                        ) {
                    // See if this enters in competition with the minimum
                    if ( qAbs(distancesToTarget[j]) <= minDistanceToTarget  ) {
                        // Then the one at the right side ought to be selected
                        if ( distancesToTarget[j] > 0 ) minDistanceIndx = j;
                        if ( distancesToTarget[k] > 0 ) minDistanceIndx = k;
                    }
                    // else this coincidence is irrelevant
                }

            }
        }

        // At this point the best adjustment has been found.
        // Send it to the data structure
        _equalization->GetEqualizationResults( chipId )->SetPixelAdj( (*i)%__matrix_size, pixHistory[minDistanceIndx].first, sel );
        // And tag as equalized
        _equalization->GetEqualizationResults( chipId )->SetStatus( (*i)%__matrix_size, Mpx3EqualizationResults::__EQUALIZED, sel);

        if ( cntr < showHeadAndTail || cntr >= (int)_scheduledForFineTuning.size() - showHeadAndTail ) {
            qDebug() << "[" << *i << "](" << pixHistory[minDistanceIndx].first << ") ";
        }

        cntr++;
    }
    qDebug() << "\n -------------------";

}

int ThlScan::PixelBelongsToChip(int pix) {

    int chipId = pix / __matrix_size;

    return chipId;
}

void ThlScan::FillAdjReactTHLHistory() {

    // Loop over the pixels scheduled for Fine tuning
    set<int>::iterator i = _scheduledForFineTuning.begin();
    set<int>::iterator iE = _scheduledForFineTuning.end();
    // select THx
    Mpx3EqualizationResults::lowHighSel sel;
    if ( _DAC_Disc_code == MPX3RX_DAC_DISC_L ) sel = Mpx3EqualizationResults::__ADJ_L;
    if ( _DAC_Disc_code == MPX3RX_DAC_DISC_H ) sel = Mpx3EqualizationResults::__ADJ_H;

    for ( ; i != iE ; i++ ) {

        int chipId = PixelBelongsToChip( *i );

        // Look at the current adjustment
        int adj = _equalization->GetEqualizationResults( chipId )->GetPixelAdj( (*i)%__matrix_size, sel );
        vector< pair<int, int> > pixHistory = _adjReactiveTHLFineTuning[*i];
        // search for this adj
        int foundIndx = -1;
        for ( int p = 0 ; p < (int)pixHistory.size(); p++ ) {
            if ( pixHistory[p].first == adj ) foundIndx = p; // already there
        }
        // If the adj is found, use the new reactive THL result.
        // If the adj is not found, push it with it's corresponding reactive THL
        if ( foundIndx != -1 ) {
            pixHistory[foundIndx].second = _pixelReactiveTHL[*i];
        } else {
            pixHistory.push_back( make_pair( adj, _pixelReactiveTHL[*i] ) );
        }

        // put it back
        _adjReactiveTHLFineTuning[*i] = pixHistory;

    }

}

void ThlScan::DumpAdjReactTHLHistory(int showHeadAndTail) {

    map<int, vector< pair<int, int> > >::iterator i  = _adjReactiveTHLFineTuning.begin();
    map<int, vector< pair<int, int> > >::iterator iE = _adjReactiveTHLFineTuning.end();
    vector< pair<int, int> >::iterator vi;
    vector< pair<int, int> >::iterator viE;
    int fullHistorySize = int(_adjReactiveTHLFineTuning.size());

    qDebug() << "[INFO] adjReactiveTHL history : [pix]{ (adj,reactTHL), ... } \n";
    int cntr = 0;
    for ( ; i != iE ; i++ ) {

        if ( ( cntr < showHeadAndTail ) || ( cntr >= (fullHistorySize - showHeadAndTail)  )) {
            //qDebug() << "\t[" << (*i).first << "]{";

            vi  = (*i).second.begin();
            viE = (*i).second.end();
            for ( ; vi != viE ; vi++ ) {

                qDebug() << "(" << (*vi).first << "," << (*vi).second << ")";
                if ( (vi+1) != viE ) qDebug() << ", ";
            }
            qDebug() << "}\n";
        }

        if ( cntr == showHeadAndTail ) {
            int skipping = fullHistorySize - 2*showHeadAndTail;
            qDebug() << " ... skip " << skipping << " ... " << "\n";
        }

        cntr++;
    }
    qDebug() << "\n" << "       And a list of pixels stuck non-reactive (if any) --> " << "\n";

    // search for a few special non-reactive pixels
    i  = _adjReactiveTHLFineTuning.begin();
    int cntrROI = 0;
    for ( ; i != iE ; i++ ) {

        vi  = (*i).second.begin();
        viE = (*i).second.end();
        if( (*vi).second == __UNDEFINED ) { // interesting pixel
            qDebug() << "       " << "[" << (*i).first << "]{";
            for ( ; vi != viE ; vi++ ) {
                qDebug() << "(" << (*vi).first << "," << (*vi).second << ")";
                if ( (vi+1) != viE ) qDebug() << ", ";
            }
            qDebug() << "}" << "\n";
            if(cntrROI++ > 5) break; // only a few of these pixels, finish here
        }
    }
}

void ThlScan::SetDAC_propagateInGUI(SpidrController * spidrcontrol, int devId, int dac_code, int dac_val ){

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

bool ThlScan::ThereIsAFalse(vector<bool> v){

    vector<bool>::iterator i  = v.begin();
    vector<bool>::iterator iE = v.end();

    for ( ; i != iE ; i++ ) {
        if ( (*i) == false ) return true;
    }
    return false;
}

void ThlScan::EqualizationScan() {

    // Open a new temporary connection to the SPIDR to avoid collisions to the main one
    int ipaddr[4] = { 1, 1, 168, 192 };
    if ( _srcAddr != 0 ) {
        ipaddr[3] = (_srcAddr >> 24) & 0xFF;
        ipaddr[2] = (_srcAddr >> 16) & 0xFF;
        ipaddr[1] = (_srcAddr >>  8) & 0xFF;
        ipaddr[0] = (_srcAddr >>  0) & 0xFF;
    }

    SpidrController * spidrcontrol = new SpidrController( ipaddr[3], ipaddr[2], ipaddr[1], ipaddr[0] );

    // 0 : DEBUG
    // 1 : INFO
    // 2 : WARNINGS, ERROR, FATAL
    spidrcontrol->setLogLevel( 2 );

    if ( !spidrcontrol || !spidrcontrol->isConnected() ) {
        qDebug() << "[ERROR]\tDevice not connected\n";
        return;
    }

    // -------------------------------------------------------------------------
    // Prepare a dataset just to handle the assembling of compound frames
    // Rewind local counters
    if ( !_dataset ) _dataset = new Dataset ( __matrix_size_x, __matrix_size_y, _nchipsX*_nchipsY );
    _dataset->clear();
    RewindData(_fullsize_x, _fullsize_y);

    for ( unsigned long di = 0 ; di < _workChipsIndx.size() ; di++ ) {

        if ( ! _mpx3gui->getConfig()->detectorResponds( _workChipsIndx[di] ) ) continue;


        // Send all the adjustment bits to a global value
        if ( _adjType == __adjust_to_global ) {
            if( _DAC_Disc_code == MPX3RX_DAC_DISC_L ) {
                _equalization->SetAllAdjustmentBits(spidrcontrol, _workChipsIndx[di], _equalization->GetSteeringInfo(_workChipsIndx[di])->globalAdj, 0x0);
                //qDebug() << "[INFO]\tSetting global L =  " << _equalization->GetSteeringInfo(_workChipsIndx[di])->globalAdj << endl;
            }
            if( _DAC_Disc_code == MPX3RX_DAC_DISC_H ) {
                _equalization->SetAllAdjustmentBits(spidrcontrol, _workChipsIndx[di], 0x0, _equalization->GetSteeringInfo(_workChipsIndx[di])->globalAdj);
                //qDebug() << "[INFO]\tSetting global H =  " << _equalization->GetSteeringInfo(_workChipsIndx[di])->globalAdj << endl;
            }
        } else if ( _adjType == __adjust_to_equalizationMatrix ) {
            qDebug() << "[INFO]\tSetting adjustment bits on chip:" << _workChipsIndx[di];
            _equalization->SetAllAdjustmentBits(spidrcontrol, _workChipsIndx[di], false, _testPulses);
        }

        // While equalizing one threshold the other should be set at a very high value
        //   to keep that circuit from reacting.  Set it at ~100
        //qDebug() << "presettings : " << (1<<MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_5-1].bits)/4 << ", " <<  (1<<MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_7-1].bits)/4 << endl;
        //qDebug() << "              " << MPX3RX_DAC_THRESH_5 << ", " << MPX3RX_DAC_THRESH_7 << endl;
        //qDebug() << " trigger ---> " << _mpx3gui->getConfig()->getTriggerLength();

        if ( _DAC_Disc_code == MPX3RX_DAC_DISC_L ) {
            SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_1, __low_but_above_noise_threshold );
            if ( _mpx3gui->getConfig()->getColourMode() ) {
                SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_3, __low_but_above_noise_threshold );
                SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_5, __low_but_above_noise_threshold );
                SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_7, __low_but_above_noise_threshold );
            }
        } else if (  _DAC_Disc_code == MPX3RX_DAC_DISC_H ) {
            SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_0, __low_but_above_noise_threshold );
            if ( _mpx3gui->getConfig()->getColourMode() ) {
                SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_2, __low_but_above_noise_threshold );
                SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_4, __low_but_above_noise_threshold );
                SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[di], MPX3RX_DAC_THRESH_6, __low_but_above_noise_threshold );
            }
        }

    }

    // Signals to draw out of the worker
    connect( this, SIGNAL( UpdateChartSignal(int, int, int) ), this, SLOT( UpdateChart(int, int, int) ) );
    connect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );
    connect(_equalization, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread())); // stop signal from qcstmglvis

    // Sometimes a reduced loop is selected
    int processedLoops = 0;
    bool finishScan = false;
    bool finishTHLLoop = false;
    // For a truncated scan
    int expectedInOneThlLoop = int( _workChipsIndx.size()*__matrix_size ) / ( _spacing*_spacing );

    // The data buffer id doesn't necessarily corresponds to _deviceIndex
    int idDataFetch = -1;
    bool accelerationApplied = false;
    int accelerationFlagCntr = 0; //The compiler thinks this is unused, it is used...
    int nMasked = 0, pmasked = 0;

    int progressMax = _numberOfLoops;
    if ( _numberOfLoops < 0 ) progressMax = _spacing * _spacing;

    for ( int maskOffsetItr_x = 0 ; maskOffsetItr_x < _spacing ; maskOffsetItr_x++ ) {

        for ( int maskOffsetItr_y = 0 ; maskOffsetItr_y < _spacing ; maskOffsetItr_y++ ) {

            QString loopProgressS;
            loopProgressS =  QString::number( maskOffsetItr_x * _spacing + maskOffsetItr_y + 1, 'd', 0 );
            loopProgressS += "/";
            loopProgressS += QString::number( progressMax, 'd', 0 );
            connect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelLoopProgress, SLOT( setText(QString)) );
            fillText( loopProgressS );
            disconnect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelLoopProgress, SLOT( setText(QString)) );

            // Set mask
            nMasked = 0;
            pmasked = 0;
            for ( unsigned int devId = 0 ; devId < _workChipsIndx.size() ; devId++ ) {
                if ( ! SetEqualizationMask(spidrcontrol, _workChipsIndx[devId], _spacing, maskOffsetItr_x, maskOffsetItr_y, &pmasked) ) {
                    qDebug() << "[FAIL]\tCould not set equalisation mask";
                    return;
                }

                nMasked += pmasked;
            }
            //qDebug() << "[INFO] [Equalisation] offset_x:" << maskOffsetItr_x << ", offset_y:" << maskOffsetItr_y <<  "| N pixels unmasked = " << int(_workChipsIndx.size()*__matrix_size) - nMasked << "\n";

            // Start the Scan for one mask
            _pixelReactiveInScan = 0;
            finishTHLLoop = false;
            accelerationFlagCntr = 0;
            vector<bool> doReadFrames;
            accelerationApplied = false;

            // limits from the GUI (no signals on them)
            _stepScan = _equalization->GetUI()->eqStepSpinBox->value();

            // THx scan
            bool scanContinue = true;
            // Min scan is in fact the start of the scan (name misleading).
            // In both cases descendant and ascendant the scan starts here.
            _thlItr = _minScan;

            for ( ; scanContinue ; ) {

                QString thlLabelS;
                thlLabelS = QString::number( _thlItr, 'd', 0 );
                if ( accelerationApplied ) thlLabelS += " acc";
                // Send signal to Labels.  Making connections one by one.
                connect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelTHLCurrentValue, SLOT( setText(QString)) );
                fillText( thlLabelS );
                disconnect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelTHLCurrentValue, SLOT( setText(QString)) );

                // Set the threshold on all chips
                for ( int devId = 0; devId < int(_workChipsIndx.size()); devId++ ) {
                    SetDAC_propagateInGUI( spidrcontrol, _workChipsIndx[devId], _dac_code, _thlItr );
                }

                // flush the history of good/bad frames
                doReadFrames.clear();

                // See if there is a frame available.  I should get as many frames as triggers
                int framesCntr = 0;
                unsigned long timeOutTime = 100; //ms

                // Start the trigger as configured
                spidrcontrol->startAutoTrigger();

                while ( _spidrdaq->hasFrame( timeOutTime ) ) {

                    // assume a good frame
                    doReadFrames.push_back( true );


                    // Check quality
                    FrameSet *frameSet = _spidrdaq->getFrameSet();
                    if  (frameSet->pixelsLost() != 0 ) { // The total number of lost packets/pixels detected in the current frame
                        // schedule a bad frame.  Don't 'continue' the loop or release frame just yet !
                        qDebug() << "bad frame, thl: " << _thlItr;
                        doReadFrames[framesCntr] = false;
                    }

                    if ( doReadFrames[framesCntr] ) {

                        int nChips = _mpx3gui->getConfig()->getNDevicesSupported();
                        // Go through all chips avoiding those not present
                        for ( int idx = 0 ; idx < nChips ; idx++ ) {

                            if ( ! _mpx3gui->getConfig()->detectorResponds( idx ) ) continue;

                            idDataFetch = _mpx3gui->getConfig()->getDataBufferId( idx );

                            // Stack
                            _dataset->setFrame(frameSet, idx, 0);

                        }

                        // Once the frame is complete, extract info
                        _data = _dataset->getLayer( 0 );
                        // I am assuming that all the frames have the same size in bytes
                        _pixelReactiveInScan += ExtractScanInfo( _data, MPX_PIXELS * 4 * _nchipsX*_nchipsY, _thlItr );
                    }

                    _spidrdaq->releaseFrame(frameSet);

                    if ( doReadFrames[framesCntr] ) {

                        // Report to heatmap
                        emit UpdateHeatMapSignal(_fullsize_x, _fullsize_y);

                        // Report to graph
                        for ( int devId = 0 ; devId < (int)_workChipsIndx.size() ; devId++ ) {
                            if ( !_blindScan ) emit UpdateChartSignal(_workChipsIndx[devId], _setId, _thlItr);
                        }

                        // Last scan boundaries
                        // This information could be useful for a next scan
                        if ( _pixelReactiveInScan != 0 ) {
                            if ( _thlItr < _detectedScanBoundary_L ) _detectedScanBoundary_L = _thlItr;
                            if ( _thlItr > _detectedScanBoundary_H ) _detectedScanBoundary_H = _thlItr;
                        }
                    }

                    framesCntr++;
                }

                // Try again if necessary
                if ( ThereIsAFalse( doReadFrames ) ) {
                    continue;
                }

                /*
                // FIXME
                // If the scan has reached the total number of pixels expected to react. Stop.
                //if ( pixelReactiveInScan % expectedInScan == 0 && pixelReactiveInScan > 0 ) {
                //	finishScan = true;
                //	qDebug() << "[INFO] All pixels in round found active. Scan stops at THL = " << i << endl;
                //}

                // See if this is a scan which can be aloud to truncate.
                // Useful in certain cases like DiscL optimization
                if ( _stopWhenPlateau ) {
                    if ( (double)_pixelReactiveInScan > (double)expectedInOneThlLoop*0.99 ) {
                        finishTHLLoop = true;
                        qDebug() << "[INFO] Truncate scan. 99% reached. Scan stops at THL = " << _thlItr << endl;
                    }
                }
                 */

                QString reactiveLabelS;
                reactiveLabelS = QString::number( _pixelReactiveInScan, 'd', 0 );
                reactiveLabelS += "/";
                reactiveLabelS += QString::number( expectedInOneThlLoop, 'd', 0);
                // Send signal to Labels.  Making connections one by one.
                connect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelNPixelsReactive, SLOT( setText(QString)) );
                fillText( reactiveLabelS );
                disconnect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelNPixelsReactive, SLOT( setText(QString)) );

                // If done with all pixels
                if ( _pixelReactiveInScan == expectedInOneThlLoop ) {
                    finishTHLLoop = true;
                }

                if( finishScan || finishTHLLoop) break;

                /*
                if( ! _equalization->isScanDescendant() ) {

                    // Scan upwards
                    // Accelerate if the pixels reactive reached a significant fraction
                    // Once the 99% limit is reached, wait 50 counts and accelerate
                    if ( (double)_pixelReactiveInScan > (double)expectedInOneThlLoop*0.99
                            && !accelerationApplied
                            && accelerationFlagCntr < __accelerationStartLimit
                    ) {
                        accelerationFlagCntr += _stepScan;
                        if ( accelerationFlagCntr >= __accelerationStartLimit ) {
                            _stepScan *= __step_scan_boostfactor;
                            accelerationApplied = true;
                        }
                    }

                } else {
                    // Scan backwards
                    // Come back to normal pace when a pixel reacts
                    if ( _pixelReactiveInScan != 0
                            && accelerationApplied
                    ) {
                        _stepScan = oldStep;
                        accelerationApplied = false;
                        _equalization->GetUI()->eqStepSpinBox->setValue( _stepScan );
                    }
                }
*/
                if ( _stop ) break;

                // increment
                if( _equalization->isScanDescendant() ) _thlItr -= _stepScan;
                else _thlItr += _stepScan;

                // See the termination condition
                if ( _equalization->isScanDescendant() ) {
                    if ( _thlItr >= _maxScan ) scanContinue = true;
                    else scanContinue = false;
                } else {
                    if ( _thlItr >= _minScan ) scanContinue = true;
                    else scanContinue = false;
                }

            }

            // A full spacing loop has been achieved here
            processedLoops++;
            if( _numberOfLoops > 0 && _numberOfLoops == processedLoops ) finishScan = true;

            if( finishScan || _stop ) break;
        }

        if( finishScan || _stop ) break;
    }

    // -------------------------------------------------------------------------
    // Scan finished

    // Here's on Scan completed.  Do the stats on it.

    if (_testPulses) {
        setEqualisationTargets();
    }

    // Signals to draw out of the thread
    disconnect( this, SIGNAL( UpdateChartSignal(int, int, int) ), this, SLOT( UpdateChart(int, int, int) ) );
    disconnect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );
    disconnect( _equalization, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread()) );

    // in case the thread is used again
    _stop = false;

    // delete the extra space to plot.
    if ( _plotdata ) {
        delete [] _plotdata;
        _plotdata = nullptr;
    }

    // the dataset used here for frames stichting
    delete _dataset;
    _dataset = nullptr;

    delete spidrcontrol;
}

bool ThlScan::OutsideTargetRegion(int devId, int pix, double Nsigma) {

    int offset =  floor( Nsigma * GetScanResults(devId)->sigma );
    if ( offset < 1 ) offset = 1;

    int currentEqualisationTarget = GetScanResults(devId)->equalisationTarget;
    int lowLim = currentEqualisationTarget - offset;
    int highLim = currentEqualisationTarget + offset;

    return ( _pixelReactiveTHL[pix] < lowLim  ||
             _pixelReactiveTHL[pix] > highLim ||
             _pixelReactiveTHL[pix] < _equalization->GetNTriggers() // simply never responded
             );
}

/**
 * As the fine tuning is started.  Tag the pixels in order to know who needs rework.
 * The vetolist has been obtained through ExtractFineTuningVetoList and lists the
 *   pixels which don't need rework.
 */

void ThlScan::TagPixelsEqualizationStatus(set<int> vetoList) {

    //! These pixels will be marked in the Mpx3EqualizationResults as __EQUALIZED

    for ( int i = 0 ; i < __matrix_size ; i++ ) {

        if( vetoList.find(i) == vetoList.end() ) {
            // if it is not in the veto-list it needs rework
            _equalization->GetEqualizationResults(_deviceIndex)->SetStatus( i, Mpx3EqualizationResults::__SCHEDULED_FOR_FINETUNING );

        } else {
            // if it is found in the veto-list this pixel is well equalized
            _equalization->GetEqualizationResults(_deviceIndex)->SetStatus( i, Mpx3EqualizationResults::__EQUALIZED );
            // These can be send to the histogram immediately
            _fineTuningPixelsEqualized.insert( i );
        }
    }

}


set<int> ThlScan::ExtractPixelsNotOnTarget() {

    // select THx
    Mpx3EqualizationResults::lowHighSel sel = Mpx3EqualizationResults::__ADJ_L;
    if ( _DAC_Disc_code == MPX3RX_DAC_DISC_L ) sel = Mpx3EqualizationResults::__ADJ_L;
    if ( _DAC_Disc_code == MPX3RX_DAC_DISC_H ) sel = Mpx3EqualizationResults::__ADJ_H;

    set<int> reworkList;
    int chipId = 0;

    int equalisedPixelCount = 0;

    for ( int i = 0 ; i < _fullsize_x*_fullsize_y ; i++ ) {

        //qDebug() << "[" << i << "]" << _pixelReactiveTHL[i] << ", ";
        //if ( i % 1024 == 0 && i != 0 ) qDebug() << "\n";

        chipId = PixelBelongsToChip( i );

        // consider the pixel only if it belongs in the _workChipsIndx
        if ( ! _equalization->pixelInScheduledChips( i ) ) continue;

        if ( _pixelReactiveTHL[i] != GetScanResults(chipId)->equalisationTarget ) {
            reworkList.insert( i );
            // WARNING: the numbering is per chip in the EqualizationResults
            _equalization->GetEqualizationResults( PixelBelongsToChip( i ) )->SetStatus( i % __matrix_size, Mpx3EqualizationResults::__SCHEDULED_FOR_FINETUNING, sel );
        } else {
            // Otherwise tag it as equalized
            _equalization->GetEqualizationResults( PixelBelongsToChip( i ) )->SetStatus( i % __matrix_size, Mpx3EqualizationResults::__EQUALIZED, sel);
            equalisedPixelCount++;
        }
    }
    qDebug() << "[INFO]\t Pixels found out of target :" << reworkList.size();
    qDebug() << "[INFO]\t Equalised pixels :" << equalisedPixelCount << "\n";

    return reworkList;
}

void ThlScan::RewindReactionCounters(set<int> reworkPixelsSet) {
    set<int>::iterator i = reworkPixelsSet.begin();
    set<int>::iterator iE = reworkPixelsSet.end();

    for( ; i != iE ; i++ ) {
        //_pixelReactiveTHL[ *i ] = __UNDEFINED; // NOT THIS ONE ! KEEP IT
        _pixelCountsMap[ *i ] = __NOT_TESTED_YET;
    }

}

void ThlScan::DumpSet(set<int> theset, QString name, int max) {

    qDebug() << "-- " << name << " [" << (int)theset.size() << "] --------" << "\n";

    if ( theset.empty() ) return;

    set<int>::iterator i = theset.begin();
    set<int>::iterator iE = theset.end();

    int cntr = 0;
    qDebug() << "< pixId{reactTHL,adj} : ";
    for ( ; i != iE ; ) {

        qDebug() << *i << "{" << _pixelReactiveTHL[*i] << ":" << _equalization->GetEqualizationResults(_deviceIndex)->GetPixelAdj( *i ) << "} ";

        if ( ++cntr >= max ) {
            qDebug() << "... printed " << max << " of " << theset.size();
            break;
        }

        if ( ++i != iE ) qDebug() << ", ";

    }

    qDebug() << " >" << "\n";

}

bool ThlScan::AdjScanCompleted(set<int> reworkSubset, set<int> /*activeMask*/) {

    // End the scan if
    //  1) all the pixels involved have touched (reactTHL val) the equalization target OR passed over it
    //  2) OR exhausted all adj values

    // select THx
    Mpx3EqualizationResults::lowHighSel sel;
    if ( _DAC_Disc_code == MPX3RX_DAC_DISC_L ) sel = Mpx3EqualizationResults::__ADJ_L;
    if ( _DAC_Disc_code == MPX3RX_DAC_DISC_H ) sel = Mpx3EqualizationResults::__ADJ_H;

    // Consider only pixels from the reworkSubset NOT in the activeMask
    set<int>::iterator i  = reworkSubset.begin();
    set<int>::iterator iE = reworkSubset.end();

    int pixelsConsidered = 0;
    int pixelsLimitsReached = 0;
    int pixelsAteqT = 0;
    bool passedUpTarget = false;
    bool passedOnTarget = false;
    bool passedUnderTarget = false;
    int chipId = 0;

    for ( ; i != iE ; i++ ) {

        // rewind some vars
        passedUpTarget = false;
        passedOnTarget = false;
        passedUnderTarget = false;

        chipId = PixelBelongsToChip( *i );

        // Skip if found in the mask
        //if ( activeMask.find( *i ) != activeMask.end() ) continue;

        // Also skip if the pixel has been previously marked as equalized or impossible to equalize ( > __EQUALIZED)
        if ( _equalization->GetEqualizationResults( chipId )->GetStatus( (*i)%__matrix_size, sel ) >= Mpx3EqualizationResults::__EQUALIZED ) continue;

        // Considering this pixel.  Count it.
        pixelsConsidered++;

        // See how many adj values have been tested for this pixel
        vector< pair<int, int> > adjTHLHistory = _adjReactiveTHLFineTuning[ *i ];
        vector< pair<int, int> >::iterator adjI  = adjTHLHistory.begin();
        vector< pair<int, int> >::iterator adjIE = adjTHLHistory.end();
        // Here we are searching for extremes.  The history doesn't have to contain all values [0:31].
        // If it contains one of the extremes 0 or 31, it's enough.
        for ( ; adjI != adjIE ; adjI++ ) {

            // Check the limits
            if ( (*adjI).first == 0x0 ||  (*adjI).first == __max_adj_val ) pixelsLimitsReached++; // TODO, probably i shouldn't consider here the high extreme = 31

            int currentEqualisationTarget = GetScanResults(chipId)->equalisationTarget;
            // Check if it's passing over the target
            if ( (*adjI).second  > currentEqualisationTarget) passedUpTarget = true;
            if ( (*adjI).second == currentEqualisationTarget) passedOnTarget = true;
            if ( (*adjI).second  < currentEqualisationTarget) passedUnderTarget = true;
        }

        // See if it passed over the target or right on the target
        if ( (passedUpTarget && passedUnderTarget) || passedOnTarget ) {
            pixelsAteqT++;
            // And tag the pixel as equalized already
            _equalization->GetEqualizationResults( chipId )->SetStatus( (*i)%__matrix_size, Mpx3EqualizationResults::__EQUALIZED, sel);
        }

    }

    // First condition
    // 1) all the pixels involved have touched (reactTHL val) the equalization target
    if ( pixelsConsidered == pixelsAteqT ) return true;

    // Second condition
    // 2) exhausted all adj values. If it contains one of the extremes 0 or 31, it's enough.
    if ( pixelsConsidered == pixelsLimitsReached ) return true;

    return false;
}

void ThlScan::SetMinScan(int min) {

    // default value
    if( min == -1 ) {
        _minScan = _equalization->GetUI()->eqMinSpinBox->value();
        return;
    }

    _minScan = min;
    _equalization->GetUI()->eqMinSpinBox->setValue( _minScan );
}

void ThlScan::SetMaxScan(int max) {

    // default value
    if( max == -1 ) {
        _maxScan = _equalization->GetUI()->eqMaxSpinBox->value();
        return;
    }

    _maxScan = max;
    _equalization->GetUI()->eqMaxSpinBox->setValue( _maxScan );
}

/**
 * return the number of pixels in the rework subset which are not in the mask
 */
int ThlScan::ShiftAdjustments(set<int> reworkSubset, set<int> activeMask) {

    set<int>::iterator i = reworkSubset.begin();
    set<int>::iterator iE = reworkSubset.end();

    int adj = 0, newadj = 0;
    int nPixelsNotInMask = 0;
    int chipId = 0;

    int changedAdj = 0;
    int p=0, p0=0, p1=0, p2=0, p3=0, p4=0, p5=0, p6=0, p7=0, p8=0, p9=0;

    // select THx
    Mpx3EqualizationResults::lowHighSel sel;
    if ( _DAC_Disc_code == MPX3RX_DAC_DISC_L ) sel = Mpx3EqualizationResults::__ADJ_L;
    if ( _DAC_Disc_code == MPX3RX_DAC_DISC_H ) sel = Mpx3EqualizationResults::__ADJ_H;

    for( ; i != iE ; i++ ) {

        // First see if the pixel is in the mask.
        // If it is found in the mask (masked pixels, i.e. not reacting) the pixel doesn't need an adjustment shift yet.
        if ( activeMask.find( *i ) != activeMask.end() ) {
            continue;
        }

        chipId = PixelBelongsToChip( *i );

        nPixelsNotInMask++;

        if ( _equalization->GetEqualizationResults( chipId )->GetStatus( (*i)%__matrix_size, sel ) == Mpx3EqualizationResults::__NOT_EQUALIZED ) {
            p++;
        }
        if ( _equalization->GetEqualizationResults( chipId )->GetStatus( (*i)%__matrix_size, sel ) == Mpx3EqualizationResults::__SCHEDULED_FOR_FINETUNING ) {
            p0++;
        }
        if ( _equalization->GetEqualizationResults( chipId )->GetStatus( (*i)%__matrix_size, sel ) == Mpx3EqualizationResults::__EQUALIZED ) {
            p1++;
        }
        if ( _equalization->GetEqualizationResults( chipId )->GetStatus( (*i)%__matrix_size, sel ) == Mpx3EqualizationResults::__EQUALIZATION_FAILED_ADJ_OUTOFBOUNDS ) {
            p2++;
        }
        if ( _equalization->GetEqualizationResults( chipId )->GetStatus( (*i)%__matrix_size, sel ) == Mpx3EqualizationResults::__EQUALIZATION_FAILED_NONREACTIVE ) {
            p3++;
        }

        // Also skip if the pixel has been previously marked as equalized or higher status
        if ( _equalization->GetEqualizationResults( chipId )->GetStatus( (*i)%__matrix_size, sel ) >= Mpx3EqualizationResults::__EQUALIZED ) {
            continue;
        }



        // Get the current adj
        adj = _equalization->GetEqualizationResults( chipId )->GetPixelAdj( (*i)%__matrix_size, sel);
        // assume no change for the moment
        newadj = adj;

        bool specialCase = false;
        // special cases
        if (  _pixelReactiveTHL[ *i ] == __UNDEFINED && ( adj == __max_adj_val ) ) {
            // this pixels possibly stuck at the left side in negative thl values.  Bring it out all the way !
            newadj = 0x0;
            specialCase = true;
            p4++;
        }
        if (  _pixelReactiveTHL[ *i ] == __UNDEFINED && ( adj == 0x0 ) ) {
            // this pixel will never react.  Mark immediately.
            _equalization->GetEqualizationResults( chipId )->SetStatus( (*i)%__matrix_size, Mpx3EqualizationResults::__EQUALIZATION_FAILED_NONREACTIVE, sel);
            specialCase = true;
            p5++;
        }

        int currentEqualisationTarget = GetScanResults(chipId)->equalisationTarget;
        if (lastEqualisationTarget != currentEqualisationTarget) {
            lastEqualisationTarget = currentEqualisationTarget;
            qDebug() << "[INFO]\tCurrent equalisation target =" << currentEqualisationTarget;
        }

        if ( ! specialCase ) {
            // take a decision on next adjustment
            if ( _pixelReactiveTHL[ *i ] == __UNDEFINED ) {
                // If the pixel is not at the right corner (this has been considered just before)
                newadj = adj - 1;
                p6++;
            } else if ( _pixelReactiveTHL[ *i ] > currentEqualisationTarget ) {
                newadj = adj + 1;
                p7++;
            } else if ( _pixelReactiveTHL[ *i ] < currentEqualisationTarget ) {
                newadj = adj - 1;
                p8++;
            }

            // If out of bounds don't change the value
            if ( newadj > __max_adj_val || newadj < 0 ) {
                newadj = adj;
                p9++;
            }
        }

        //! Set the new adjustment for this particular pixel (internally)
        //! It is different depending on the chipId
        //!
        //! Note: this changes the internal results, it does not set them
        _equalization->GetEqualizationResults( chipId )->SetPixelAdj( (*i)%__matrix_size , newadj, sel);

        if (newadj != adj) {
            changedAdj++;
        }
    }

    file_fineTuningStats << (changedAdj) << ", " <<
                            (p)  << ", " <<
                            (p0) << ", " <<
                            (p1) << ", " <<
                            (p2) << ", " <<
                            (p3) << ", " <<
                            (p4) << ", " <<
                            (p5) << ", " <<
                            (p6) << ", " <<
                            (p7) << ", " <<
                            (p8) << ", " <<
                            (p9) << "\n";

    return nPixelsNotInMask;
}

/**
 * reworkPixelsSet:	Contains the pixels which need rework.
 * reworkSubset: 	Will contain only those from reworkPixelsSet which respect the spacing.
 * 		Every time a pixel is picked from reworkPixelsSet, it also gets removed from it.
 * 		reworkSubset gets shrink by other functions below when rework pixels are tuned.
 */
int ThlScan::ExtractReworkSubsetSpacingAware(set<int> & reworkPixelsSet, set<int> & reworkSubset, int spacing) {

    // Iterate on the input set
    set<int>::iterator i = reworkPixelsSet.begin();
    set<int>::iterator iE = reworkPixelsSet.end();

    // Iterate on the output set
    set<int>::iterator ri = reworkSubset.begin();
    set<int>::iterator riE = reworkSubset.end();

    // Count the number of pixels moved to the reworkSubset
    int cntrToSubset = 0;

    // Keep a list of those pixels sent to the reworkSubset which need to be erased
    //  from the reworkPixelsSet.
    set<int> scheduleToErase;

    // Chances are the reworkSubset is empty.  Let's insert one value to get it started.
    if ( reworkSubset.empty() ) {
        reworkSubset.insert ( *i );
        scheduleToErase.insert ( *i );
        cntrToSubset++;
        i++;
    }

    bool clearToAdd = true;

    for ( ; i != iE ; i++ ) {

        // Rewind bounds on reworkSubset
        ri = reworkSubset.begin();
        riE = reworkSubset.end();

        // See if the current pixel is at a safe distance from all pixels in reworkSubset
        clearToAdd = true;
        for ( ; ri != riE ; ri++ ) {
            if ( ! TwoPixelsRespectMinimumSpacing( *i, *ri, spacing) ) clearToAdd = false;
        }

        // If it respects the minimum spacing, then put it in the reworkSubset and
        //  schedule for erasing.
        if ( clearToAdd ) {
            reworkSubset.insert ( *i );
            scheduleToErase.insert ( *i );
            cntrToSubset++;
        }

    }

    // Erase from reworkPixelsSet what was tossed in reworkSubset
    // Iterate on the output set
    set<int>::iterator ei = scheduleToErase.begin();
    set<int>::iterator eiE = scheduleToErase.end();
    set<int>::iterator itoerase;
    for ( ; ei != eiE ; ei++ ) {
        itoerase = reworkPixelsSet.find( *ei );
        if ( itoerase != reworkPixelsSet.end() ) reworkPixelsSet.erase( itoerase );
    }

    return cntrToSubset;
}

bool ThlScan::TwoPixelsRespectMinimumSpacing(int pix1, int pix2, int spacing) {

    pair<int, int> p1 = XtoXY(pix1, __matrix_size_x);
    pair<int, int> p2 = XtoXY(pix2, __matrix_size_x);

    if ( abs(p1.first  - p2.first) < spacing && abs(p1.second - p2.second) < spacing ) {
        return false;
    } else {
        return true;
    }
}


void ThlScan::ExtractStatsOnChart(int devId, int setId) {

    double weightedSum = 0.;
    double weights = 0.;
    // Normalization value (p val) used later when calculating the variance
    double norm_val = 0.;
    // Calculate the weighted arithmetic mean and standard deviation
    BarChart * bc = _equalization->GetBarChart(devId);
    QCPBarDataMap * dataSet = bc->GetDataSet( setId )->data();
    QCPBarDataMap::iterator i = dataSet->begin();

    for( ; i != dataSet->end() ; i++) {

        if( (*i).value != 0 ) {
            weightedSum += ( (*i).key * (*i).value );
            weights += (*i).value;

            norm_val += (*i).value;
        }
    }
    norm_val = 1. / norm_val;

    if ( weights != 0.) GetScanResults(devId)->weighted_arithmetic_mean = weightedSum / weights;
    else GetScanResults(devId)->weighted_arithmetic_mean = 0.;

    // Rewind the iterator and get the sigma
    // I need to weight the sigmas first

    i = dataSet->begin();
    for( ; i != dataSet->end() ; i++) {
        GetScanResults(devId)->sigma += ( (*i).value * norm_val ) * (
                    ( (*i).key - GetScanResults(devId)->weighted_arithmetic_mean )
                    *
                    ( (*i).key - GetScanResults(devId)->weighted_arithmetic_mean )
                    );
    }
    GetScanResults(devId)->sigma = sqrt( GetScanResults(devId)->sigma );

}


int ThlScan::ExtractScanInfo(int * data, int size_in_bytes, int thl) {

    int nPixels = size_in_bytes/4;
    int pixelsActive = 0;

    // Each 32 bits corresponds to the counts in each pixel already
    // in 'int' representation as the decoding has been requested

    for(int i = 0 ; i < nPixels ; i++) {

        if ( ( data[i] >= _equalization->GetNHits() ) ) {

            // Increase the counting in this pixel if it hasn't already reached the _nTriggers
            if ( _pixelCountsMap[i] < _equalization->GetNTriggers() ) {
                _pixelCountsMap[i]++;
            }

            // It it reached the number of triggers, set this Threshold as the reactive threshold
            if ( _pixelCountsMap[i] == _equalization->GetNTriggers() ) {

                // This way I mark the pixel as reactive +1
                _pixelCountsMap[i]++;

                //qDebug() << i << " | " << _pixelCountsMap[i] << " -- data --> " << data[i] << " | triggers : " << _equalization->GetNTriggers() << endl;

                _pixelReactiveTHL[i] = thl;
                _nReactivePixels++;
                pixelsActive++;
            }
        }
    }

    return pixelsActive;
}

int ThlScan::ExtractScanInfo(int * data, int size_in_bytes, int thl, int interestingpix) {

    int nPixels = size_in_bytes/4;
    int pixelsActive = 0;
    // Each 32 bits corresponds to the counts in each pixel already
    // in 'int' representation as the decoding has been requested

    for(int i = 0 ; i < nPixels ; i++) {

        if ( i == interestingpix ) {
            qDebug() << " { " << i << " | counts: " << _pixelCountsMap[i] << " -- data --> " << data[i] << " | reactive: " << _pixelReactiveTHL[i] << "} " << endl;
        }

        // I checked that the entry is not zero, and also that is not in the maskedMap
        if ( data[i] != 0 && ( _maskedSet.find( i ) == _maskedSet.end() ) ) {
            // Increase the counting in this pixel if it hasn't already reached the _nTriggers
            if ( _pixelCountsMap[i] < _equalization->GetNTriggers() ) {
                _pixelCountsMap[i]++;
            }
            // It it reached the number of triggers, set this Threshold as the reactive threshold
            if ( _pixelCountsMap[i] == _equalization->GetNTriggers() ) {

                // This way I mark the pixel as reactive +1
                _pixelCountsMap[i]++;

                //qDebug() << i << " | " << _pixelCountsMap[i] << " -- data --> " << data[i] << " | triggers : " << _equalization->GetNTriggers() << endl;

                _pixelReactiveTHL[i] = thl;
                _nReactivePixels++;
                pixelsActive++;
            }
        }
    }

    return pixelsActive;
}


int ThlScan::NumberOfNonReactingPixels() {

    int nNonReactive = 0;
    map<int, int>::iterator i = _pixelReactiveTHL.begin();
    map<int, int>::iterator iE = _pixelReactiveTHL.end();

    // Test for non reactive pixels
    for ( ; i != iE ; i++ ) {
        if( (*i).second == __UNDEFINED ) nNonReactive++;
    }

    return nNonReactive;
}

vector<int> ThlScan::GetNonReactingPixels() {

    vector<int> nonReactive;
    map<int, int>::iterator i = _pixelReactiveTHL.begin();
    map<int, int>::iterator iE = _pixelReactiveTHL.end();

    // Test for non reactive pixels
    for ( ; i != iE ; i++ ) {
        if( (*i).second == __UNDEFINED ) nonReactive.push_back( (*i).first );
    }
    return nonReactive;
}

//! UNUSED FUNCTION
void ThlScan::SetConfigurationToScanResults(int DAC_DISC_setting, int global_adj) {

    // Work on all chips
    vector<ScanResults *>::iterator i  = _results.begin();
    vector<ScanResults *>::iterator iE = _results.end();

    for ( ; i != iE ; i++ ) {
        (*i)->DAC_DISC_setting = DAC_DISC_setting;
        (*i)->global_adj = global_adj;
    }

}


void ThlScan::UpdateHeatMap(int sizex, int sizey) {

    // The heat map takes the 1-D array _data and plots bottom left --> top right
    // _data comes:
    // 1) For multiple chips
    // [0](0 ---> nx*ny) [1](0 ---> nx*ny) [2](0 ---> nx*ny) [3](0 ---> nx*ny)
    //
    // 2) For one chip.  No thing needs to be done.
    // [0](0 ---> nx*ny)


    // Consider 1)

    // Initialize if not ready
    if ( !_plotdata ) {
        _plotdata = new int[sizex * sizey];
    }

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
                _plotdata[ XYtoX( xoffset + i , yoffset + j, sizex ) ] = _data[ totCntr++ ];
            }
        }

        xswitch++;
        yswitch++;
    }

    _heatmap->setData( _plotdata, sizex, sizey);
}

void ThlScan::UpdateChart(int devId, int setId, int thlValue) {

    map<int, int>::iterator itr = _pixelCountsMap.begin();
    map<int, int>::iterator itrE = _pixelCountsMap.end();

    // offset for devId
    int offset = devId * __matrix_size;

    // I am going to plot for this threshold the number of
    //  pixels which reached _nTriggers counts.  The next time
    //  they won't be considered.
    int cntr = 0;

    for(int i = 0 ; i < _fullsize_x*_fullsize_y ; i++ ) {

        // Avoid the pixels not corresponding to this devId
        if ( ( i < offset ) || i >= ( offset + __matrix_size ) ) continue;

        if( _pixelCountsMap[i] ==  _equalization->GetNTriggers() + 1 ) { // Marked as +1 := reactive
            // Count how many pixels counted at this threshold
            cntr++; // Marked as +2 --> taken into account in Chart
            _pixelCountsMap[i]++; // This way we avoid re-plotting next time. The value _nTriggers+2 identifies these pixels
        }

    }

    if ( cntr > 0 ) {
        _equalization->GetBarChart(devId)->SetValueInSet( setId , thlValue, cntr );
        _equalization->GetBarChart(devId)->fitToHeight(); //replot( QCustomPlot::rpQueued );
    }
}

/**
 * Only for plotting purposes.  Take the pixels already counted as done and plot.
 */
void ThlScan::UpdateChartPixelsReady(int devId, int setId) {

    // <thl, cntr>
    map<int, int> thlCntr;
    set<int>::iterator i = _fineTuningPixelsEqualized.begin();
    set<int>::iterator iE = _fineTuningPixelsEqualized.end();

    // initialize
    for (i = _fineTuningPixelsEqualized.begin() ; i != iE ; i++ ) {
        thlCntr[  _pixelReactiveTHL[*i] ] = 0;
    }

    // build the histogram
    for (i = _fineTuningPixelsEqualized.begin() ; i != iE ; i++ ) {
        thlCntr[  _pixelReactiveTHL[*i] ]++;
    }

    // Draw
    map<int, int>::iterator im = thlCntr.begin();
    map<int, int>::iterator imE = thlCntr.end();
    for ( ; im != imE ; im++ ) {
        _equalization->GetBarChart( devId )->SetValueInSet( setId , im->first, im->second );
    }
}

/**
 *  Once a mask is prepared only a subset of the rework pixels fall in.
 *  Get that set.
 */
set<int> ThlScan::GetReworkSubset(set<int> reworkSet, int spacing, int offset_x, int offset_y) {

    set<int> subset;

    for (int i = 0 ; i < __array_size_x ; i++) {

        // For instance if spacing = 4, there should be calls with offset_x=0,1,2,3
        //  in order to cover the whole matrix.
        if ( (i + offset_x) % spacing == 0 ) { // This is the right column
            for (int j = 0 ; j < __array_size_y ; j++) {
                if( (j + offset_y) % spacing == 0 ) { // Unmasked
                    int pix = XYtoX(i,j, __array_size_x);
                    if( reworkSet.find( pix ) != reworkSet.end() ) subset.insert( pix );
                }
            }
        }
    }
    return subset;
}

/**
 * Create and apply the mask with a given spacing
 */
bool ThlScan::SetEqualizationMask(SpidrController * spidrcontrol, int devId, int spacing, int offset_x, int offset_y, int * nmasked) {

    // Clear previous mask.  Not sending the configuration yet !
    ClearMask(spidrcontrol, devId, false);

    //! Turn test pulse bit on for that chip
    spidrcontrol->setInternalTestPulse(devId, _testPulses);

    // Indexes in the masked set need the offset of the chipId.
    // The reason is that it needs to match with the _pixelCountsMap id structure.
    int chipIdOffset = __matrix_size * devId;

    for (int i = 0 ; i < __array_size_x ; i++) {

        // For instance if spacing = 4, there should be calls with offset_x=0,1,2,3
        //  in order to cover the whole matrix.
        if ( (i + offset_x) % spacing == 0 ) { // This is the right column

            for (int j = 0 ; j < __array_size_y ; j++) {

                if( (j + offset_y) % spacing != 0 ) { // This one should be masked
                    spidrcontrol->setPixelMaskMpx3rx(i, j, true);
                    _maskedSet.insert( XYtoX(i, j, __array_size_x ) + chipIdOffset );
                } // leaving unmasked (j + offset_x) % spacing == 0
            }
        } else { // mask the entire column
            for (int j = 0 ; j < __array_size_y ; j++) {
                spidrcontrol->setPixelMaskMpx3rx(i, j, true);
                _maskedSet.insert( XYtoX(i, j, __array_size_x ) + chipIdOffset );
            }
        }
    }

    if ( ! spidrcontrol->setPixelConfigMpx3rx( devId ) ) {
        return false;
    }

    *nmasked =  _maskedSet.size();

    return true; // success
}

/**
 * Leave reworkPixels unmasked.  Returns number of un-masked pixels.
 */
int ThlScan::SetEqualizationMask(SpidrController * spidrcontrol, set<int> reworkPixels) {

    // Clear previous mask.  Not sending the configuration yet !
    ClearMask(spidrcontrol, false);

    int cntr = 0, pix = 0;
    for (int i = 0 ; i < __array_size_x ; i++) {

        for (int j = 0 ; j < __array_size_y ; j++) {

            pix = XYtoX(i, j, __array_size_x);

            // The pixels NOT in the reworkPixels set, must be masked
            if ( reworkPixels.find( pix ) == reworkPixels.end() ) {
                spidrcontrol->setPixelMaskMpx3rx(i,j, true);
                _maskedSet.insert( pix ); // this is a set, entries are unique
            } else {
                spidrcontrol->setPixelMaskMpx3rx(i,j, false);
                cntr++; // count unmasked
            }

        }

    }

    // And send the configuration
    spidrcontrol->setPixelConfigMpx3rx( _equalization->GetDeviceIndex() );

    return cntr;
}


void ThlScan::ClearMask(SpidrController * spidrcontrol, int devId, bool sendToChip){

    //! Unmask everything
    for (int i = 0 ; i < __array_size_x ; i++) {
        for (int j = 0 ; j < __array_size_y ; j++) {
            spidrcontrol->setPixelMaskMpx3rx(i, j, false);
        }
    }

    // And send the configuration if requested
    if ( sendToChip ) spidrcontrol->setPixelConfigMpx3rx( devId );

    _maskedSet.clear();

};
