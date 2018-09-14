#ifndef QCSTMEQUALIZATION_H
#define QCSTMEQUALIZATION_H

/**
 * Original author:
 *   John Idarraga <idarraga@cern.ch>
 *   Nikhef, 2014
 *
 * Extended/modified by:
 *   Navrit Bal <navrit.bal@cern.ch> <navritb@nikhef.nl>
 *   Nikhef, ASI, CERN: 2016+
 */

#include <QWidget>
#include <QMap>

#include "mpx3gui.h"
#include "mpx3defs.h"
#include "mpx3eq_common.h"
#include <qcustomplot.h>


#include <iostream>
#include <vector>

using namespace std;

#include "histogram.h"
#include "ThlScan.h"

#include "testpulseequalisation.h"

#define __default_step_scan		1
#define __low_but_above_noise_threshold 100      //! This is for an equalised chip, should be valid for all gain modes

#define EQ_NEXT_STEP(x) ( _eqStatus == x && ! _stepDone[x] )

class QCustomPlot;
class SpidrController;
class SpidrDaq;
class QCstmDacs;
class ThlScan;
class BarChart;
class BarChartProperties;
class ModuleConnection;
class testPulseEqualisation;

class Mpx3EqualizationResults {

public:

    Mpx3EqualizationResults();
    ~Mpx3EqualizationResults();

    typedef enum {
        __NOT_EQUALIZED = 0,
        __SCHEDULED_FOR_FINETUNING,				// Scheduled for Fine Tuning
        __EQUALIZED,							// DONE
        __EQUALIZATION_FAILED_ADJ_OUTOFBOUNDS,  // EQ FAILED
        __EQUALIZATION_FAILED_NONREACTIVE		// EQ FAILED
    } eq_status;

    typedef enum  {
        __ADJ_L = 0,
        __ADJ_H
    } lowHighSel;

    void SetPixelAdj(int pixId, int adj, lowHighSel sel = __ADJ_L);
    void SetPixelReactiveThl(int pixId, int thl, lowHighSel = __ADJ_L);
    int GetPixelAdj(int pixId, lowHighSel = __ADJ_L);
    int GetPixelReactiveThl(int pixId, lowHighSel = __ADJ_L);
    void maskPixel(int pixId) {
        maskedPixels.insert(pixId);
    }
    void maskPixel2D(QPair<int,int> pixId){
        maskedPixels2D.insert(pixId);
    }

    void unmaskPixel(int pixId) {
        if(maskedPixels.contains(pixId)) maskedPixels.remove(pixId);

    }
    void unmaskPixel2D( QPair<int,int> pixId) {
        if(maskedPixels2D.contains(pixId)) maskedPixels2D.remove(pixId);

    }

    int GetNMaskedPixels() {
        return maskedPixels.size();
    }

    int GetNMaskedPixels2D() {
        return maskedPixels2D.size();
    }

    QSet<int> GetMaskedPixels() {
        return maskedPixels;
    }

    QSet<QPair<int,int>> GetMaskedPixels2D() {
        return maskedPixels2D;
    }

    int * GetAdjustementMatrix(lowHighSel sel = __ADJ_L);

    void ExtrapolateAdjToTarget(int target, double eta_Adj_THL, lowHighSel sel = __ADJ_L);

    bool WriteAdjBinaryFile(QString fn);
    bool ReadAdjBinaryFile(QString fn);
    bool WriteMaskBinaryFile(QString fn);
    bool ReadMaskBinaryFile(QString fn);

    void ClearAdj();
    void ClearMasked();
    void ClearReactiveThresholds();
    void Clear();

    void SetStatus(int pixId, eq_status st, lowHighSel sel = __ADJ_L);
    eq_status GetStatus(int pixId, lowHighSel = __ADJ_L);


private:

    // pixel Id, adjustment
    QByteArray _pixId_Adj_L;
    QByteArray _pixId_Adj_H;

    // Masked pixels
    QSet<int> maskedPixels;
    //masked pixels 2D
    QSet< QPair<int,int> > maskedPixels2D;

    // pixel Id, reactive thlValue
    map<int, int> _pixId_Thl_L;
    map<int, int> _pixId_Thl_H;

    // status
    map<int, eq_status> _eqStatus_L;
    map<int, eq_status> _eqStatus_H;



};


class equalizationSteeringInfo {

public:
    equalizationSteeringInfo(){}
    ~equalizationSteeringInfo(){}

    void SetCurrentEta_Adj_THx(double v) { currentEta_Adj_THx = v; }

    //! Eta is the gradient (ie. m in y = mx +c)
    //! Cut is the y intercept (ie. c in y = mx +c)

    int equalizationCombination;
    int equalizationType;
    int globalAdj;
    int currentTHx;
    QString currentTHx_String;
    int currentDAC_DISC;
    QString currentDAC_DISC_String;
    int currentDAC_DISC_OptValue;	//! optimised value
    double currentEta_THx_DAC_Disc; //! Eta and Cut for the THx Vs DAC_DISC_x function (DAC_DISC Optimization)
    double currentCut_THx_DAC_Disc;
    double currentEta_Adj_THx; //! Eta and Cut for the Adj Vs. THx function (Adj extrapolation)
    double currentCut_Adj_THx;

    int equalisationTarget = 10;
};


namespace Ui {
class QCstmEqualization;
}

class QCstmEqualization : public QWidget
{
    Q_OBJECT

public:
    explicit QCstmEqualization(QWidget *parent = nullptr);
    ~QCstmEqualization();
    void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; }
    Ui::QCstmEqualization * GetUI();

    void PrintFraction(int * buffer, int size, int first_last);
    int GetNPixelsActive(int * buffer, int size, verblev verbose);
    void GetSlopeAndCut_IDAC_DISC_THL(ScanResults *, ScanResults *, double &, double &);
    void GetSlopeAndCut_Adj_THL(ScanResults *, ScanResults *, double &, double &);

    double EvalLinear(double eta, double cut, double x);

    // Equalization steps
    void DAC_Disc_Optimization_100();
    void DAC_Disc_Optimization_150();
    void DAC_Disc_Optimization(int devId, ScanResults * res_100, ScanResults * res_150);
    void PrepareInterpolation_0x0();
    void PrepareInterpolation_0x5();
    int * CalculateInterpolation(int devId, ThlScan * scan_x0, ThlScan * scan_x5); // ScanResults * res_x0, ScanResults * res_x5);
    void ScanOnInterpolation();
    void Rewind();
    bool InitEqualization(int chipId);      //! chipId = -1  will equalize all available chips at once
    void NewRunInitEqualization();          //! partial initialization
    bool pixelInScheduledChips(int);

    void DAC_Disc_Optimization_DisplayResults(ScanResults * res);

    int FineTuning();

    void DisplayStatsInTextBrowser(int adj, int dac_disc, ScanResults * res);
    void KeepOtherChipsQuiet();
    void resetThresholds();

    pair<int, int> XtoXY(int X, int dimX);
    void SetupSignalsAndSlots();
    void SetLimits();
    void Configuration(int devId, int THx, bool reset);
    void SetAllAdjustmentBits(SpidrController * spidrcontrol, int devId, int val_L, int val_H);
    void SetAllAdjustmentBits(SpidrController * spidrcontrol, int deviceId, bool applymask = false, bool testbit = false);
    void SetAllAdjustmentBits(SpidrController * spidrcontrol);
    void SetAllAdjustmentBits();
    void ClearAllAdjustmentBits(int devId = 0);

    void AppendToTextBrowser(QString s);
    void ClearTextBrowser();
    int GetDeviceIndex(){ return _deviceIndex; }
    int GetNTriggers(){ return _nTriggers; }
    int GetSpacing(){ return _spacing; }
    int GetMinScan(){ return _minScanTHL; }
    int GetMaxScan(){ return _maxScanTHL; }
    int GetStepScan(){ return _stepScan; }
    int GetNHits(){ return _nHits; }
    int GetFineTuningLoops() { return _fineTuningLoops; }

    int GetNChips() {return _nChips; }

    void SetMinScan(int val = -1);
    void SetMaxScan(int val = -1);
    bool isScanDescendant() { return _scanDescendant; }
    bool isBusy() { return _busy; }
    bool scanningAllChips() { return _scanAllChips; }

    void StartEqualization();
    void SetDAC_propagateInGUI(SpidrController * spidrcontrol, int devId, int dac_code, int dac_val);

    Mpx3EqualizationResults * GetEqualizationResults(int chipIndex);
    void InitializeBarChartsEqualization();
    void InitializeBarChartsAdjustements();
    void DistributeAdjHistogramsInGridLayout();
    equalizationSteeringInfo * GetSteeringInfo(int chipIdx);
    BarChart * GetBarChart(int chipIdx);
    QCheckBox * GetCheckBox(int chipIdx);
    BarChart * GetAdjBarChart(int chipIdx, Mpx3EqualizationResults::lowHighSel sel);

    int XYtoX(int x, int y, int dimX) { return y * dimX + x; }
    void UpdateHeatMap(int * data, int sizex, int sizey);

    string BuildChartName(int val, QString leg);

    void LoadEqualization(bool getPath = false, QString path ="");
    void ShowEqualization(Mpx3EqualizationResults::lowHighSel sel);

    void InitializeEqualizationStructure(); //! on a normal run, when the user load the equalization after connecting

    typedef enum {
        __INIT = 0,
        __DAC_Disc_Optimization_100,
        __DAC_Disc_Optimization_150,
        __PrepareInterpolation_0x0,
        __PrepareInterpolation_0x5,
        __ScanOnInterpolation,
         __EstimateEqualisationTarget,
        __FineTuning,
        __EQStatus_Count
    } eqStatus;
    bool * _stepDone;

    void setWindowWidgetsStatus(win_status s = win_status::startup);

    bool equalizationHasBeenLoaded(){return _equalizationLoaded; }

    QMap<int, Mpx3EqualizationResults *> getEqMap(){ return _eqMap; }

    // -------------------------- Recent changes  ------------------------------
    testPulseEqualisation * testPulseEqualisationDialog = nullptr;
    void setTestPulseMode(bool arg) { testPulseMode = arg; }

    bool estimate_V_TP_REF_AB(uint electrons, bool makeDialog);      //! This should fail if requested charge cannot be injected.
    uint setDACToVoltage(int chipID, int dacCode, double V);
    bool initialiseTestPulses(SpidrController * spidrcontrol);
    bool activateTestPulses(SpidrController * spidrcontrol, int chipID, int offset_x, int offset_y, int *maskedPixels);

private:

    Ui::QCstmEqualization * _ui;
    bool _busy;
    Dataset * _resdataset;

    // Equalization info
    QMap<int, Mpx3EqualizationResults *> _eqMap;
    vector<BarChart * > _chart;                 //! charts for all chips
    vector<QCheckBox * > _checkBoxes;           //! checkBoxes for all chips
    vector<BarChart * > _adjchart_L;            //! adjustment low bits charts
    vector<BarChart * > _adjchart_H;            //! adjustment high bits charts
    QGridLayout * _gridLayoutHistograms;

    // Connectivity between modules
    Mpx3GUI * _mpx3gui;

    QStringList files;

    QApplication * _coreApp;

    // -------------------------- Recent changes  ------------------------------
    //! bits flipped below
    // b00: SHGM  0
    // b10: HGM   1
    // b01: LGM   2
    // b11: SLGM  3
    int gainMode = 3;
    bool testPulseMode = false;

    uint defaultNoiseEqualisationTarget = 10;
    uint DAC_DISC_1_value = 100;
    uint DAC_DISC_2_value = 150;

    void resetForNewEqualisation();

    void estimateEqualisationTarget();

    int _firstMinScanTHL; //! Set at the beginning of StartEqualisation
    // -------------------------------------------------------------------------

    bool _equalizationLoaded = false;
    int _setId;
    int _deviceIndex;
    int _nTriggers;
    int _spacing; //! Pixel spacing. Ie. a pixel spacing of 2 would be scannig 1/4 pixels
    int _minScanTHL;
    int _maxScanTHL;
    int _stepScan;
    int _nHits;
    int _fineTuningLoops;
    bool _threadFinished;
    bool _scanAllChips;
    int _nchipsX;
    int _nchipsY;
    int _fullsize_x;
    int _fullsize_y;
    vector<int> _workChipsIndx;
    unsigned int _eqStatus; //! Important state machine variable
    unsigned int _scanIndex;
    enum {
        __THLandTHH = 0,
        __OnlyTHL,
        __OnlyTHH,
        __nEQCombinations
    };
    int _equalizationCombination;
    enum {
        __NoiseEdge = 0,
        __NoiseCentroid,  // upper than this, fast equalizations
        __NoiseEdgeFAST,
        __NoiseCentroidFAST,
        __nEQTypes
    };
    int _equalizationType;
    Mpx3EqualizationResults::lowHighSel _equalizationShow;
    vector<equalizationSteeringInfo *> _steeringInfo;

    // IP source address (SPIDR network interface)
    int _srcAddr;
    int _nChips;
    bool _scanDescendant;

    int **data = nullptr;
    unsigned *nx = nullptr, *ny = nullptr, nData = 0;

    // Object in charge of performing Thl scans
    QVector<ThlScan * > _scans;

    bool maskMatrix[512][512] = {{false}};    //false = unmasked, true = masked
    void resetMaskMatrix(int chipid){
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
    //convert chip index to preview index
    QPoint chipIndexToPreviewIndex(QPoint chipIndex,int chipId);
    bool makeTeaCoffeeDialog();

public slots:
    void SaveEqualization(QString path="");
    void on_logYCheckBox_toggled(bool checked);

private slots:

    void setFineTuningLoops(int);
    void setNHits(int);
    void ScanThreadFinished();
    void StartEqualizationSingleChip();
    void StartEqualizationAllChips();
    void StartEqualizationSequentialSingleChips(); //! Navrit: Added on 25/10/17 to address the bad
                                                   //! behaviour when equalising all chips
                                                   //! simultaneously
    void ChangeNTriggers(int);
    void ChangeDeviceIndex(int);
    void ChangeSpacing(int);
    void ChangeMin(int);
    void ChangeMax(int);
    void ChangeStep(int);
    void ConnectionStatusChanged(bool);
    void StopEqualization();
    void setEqualizationTHLTHH(int);
    void setEqualizationShowTHLTHH(int);
    void setEqualizationTHLType(int);
    void ShowEqualizationForChip(bool checked);

    void on_pushButton_testPulses_clicked();

signals:
    void slideAndSpin(int, int);
    void stop_data_taking_thread();

    //! Status bar signal functions
    void sig_statusBarClean();
    void sig_statusBarAppend(QString mess, QString colorString);


    void equalizationPathExported(QString path);
    void pixelsMasked(int devId,QSet<int> pixelSet);

};

#endif // QCSTMEQUALIZATION_H
