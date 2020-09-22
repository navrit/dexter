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
#include "qcustomplot.h"

#include <iostream>
#include <vector>
using namespace std;

#include "histogram.h"
#include "ThlScan.h"
#include "testpulseequalisation.h"
#include "ServerStatus.h"

#define __default_step_scan		1
#define __DAC_Disc_Optimisation_step    2
#define __above_noise_threshold 256      //! This is for an equalised chip, should be valid for all gain modes

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
    int *GetAdjustmentMatrix(lowHighSel sel = __ADJ_L);
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

#define __default_equalisationtarget 10

class equalizationSteeringInfo {

public:

    equalizationSteeringInfo(){}
    ~equalizationSteeringInfo(){}

    void SetCurrentEta_Adj_THx(double v) { currentEta_Adj_THx = v; }

    int  GetEqualizationTarget( ) { return _equalisationTarget; }
    void SetEqualizationTarget( int et ) { _equalisationTarget = et; }

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
    double currentEta_THx_DAC_Disc; //! Eta and Cut for the THx Vs DAC_DISC_x function (DAC_DISC Optimisation)
    double currentCut_THx_DAC_Disc;
    double currentEta_Adj_THx; //! Eta and Cut for the Adj Vs. THx function (Adj extrapolation)
    double currentCut_Adj_THx;

private:
    // Target 10 is the recomended value from the CERN team.
    // It is a good target for noise equalization
    // The best target can be calculated if needed.
    int _equalisationTarget = 10;
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

    typedef enum {
        __INIT = 0,
        __DAC_Disc_Optimisation_100,
        __DAC_Disc_Optimisation_150,
        __PrepareInterpolation_0x0,
        __PrepareInterpolation_0x5,
        __ScanOnInterpolation,
        //        __EstimateEqualisationTarget,
        __FineTuning,
        __EQStatus_Count
    } eqStatus;

    // int GetNPixelsActive(int * buffer, int size, verblev verbose); // Unused function

    //! Core equalisation functions
    void StartEqualization();
    bool InitEqualization(int chipId);      //! chipId = -1  will equalize all available chips at once
    void NewRunInitEqualization();          //! partial initialisation
    void Configuration(int devId, int THx, bool reset);
    void DAC_Disc_Optimisation_100();
    void DAC_Disc_Optimisation_150();
    void DAC_Disc_Optimisation(int devId, ScanResults * res_100, ScanResults * res_150);
    void PrepareInterpolation_0x0();
    void PrepareInterpolation_0x5();
    void ScanOnInterpolation();
    int * CalculateInterpolation(int devId, ThlScan * scan_x0, ThlScan * scan_x5);
    int  FineTuning();


    //! Core helper functions
    void LoadEqualisation(bool getPath = false, bool remotely = false, QString path ="");
    void SetDAC_propagateInGUI(SpidrController * spidrcontrol, int devId, int dac_code, int dac_val);
    void KeepOtherChipsQuiet();
    void FullEqRewind();
    void GetSlopeAndCut_IDAC_DISC_THL(ScanResults *, ScanResults *, double &, double &);
    void GetSlopeAndCut_Adj_THL(ScanResults *, ScanResults *, double &, double &);
    void resetThresholds();
    void InitialiseEqualizationStructure();
    void SetupSignalsAndSlots();
    void SetLimits();
    void stopEqualizationRemotely();
    void temporarilyOverrideUserChosenSpacing();
    void restoreOveriddenUserChosenSpacing();
    void clearPreviousData(uint chipListSize);
    void printNonReactiveWarning(uint chipListSize);
    void updateTestpulseVariables();
    bool pixelInScheduledChips(int pixels);
    double EvalLinear(double eta, double cut, double x);

    void SetAllAdjustmentBits(SpidrController *spidrcontrol, int deviceId, int val_L, int val_H);
    void SetAllAdjustmentBits(SpidrController *spidrcontrol, int deviceId, bool applymask);
    void SetAllAdjustmentBits(SpidrController *spidrcontrol);
    void SetAllAdjustmentBits();
    void ClearAllAdjustmentBits(int chip = 0);

    Mpx3EqualizationResults * GetEqualizationResults(int chipIndex);
    inline pair<int, int> XtoXY(int X, int dimX);
    int XYtoX(int x, int y, int dimX) { return y * dimX + x; }


    //! Test pulse equalisation
    testPulseEqualisation * testPulseEqualisationDialog = nullptr;
    void setTestPulseMode(bool arg) { testPulseMode = arg; }

    bool estimate_V_TP_REF_AB(uint electrons, bool makeDialog);      //! This should fail if requested charge cannot be injected.
    uint setDACToVoltage(int chipID, int dacCode, double V);
    //    bool initialiseTestPulses(SpidrController * spidrcontrol);

    //! GUI functions
    void ShowEqualization(Mpx3EqualizationResults::lowHighSel sel);
    void DAC_Disc_Optimisation_DisplayResults(ScanResults * res);
    void DisplayStatsInTextBrowser(int adj, int dac_disc, ScanResults * res);
    void AppendToTextBrowser(QString s);
    void ClearTextBrowser();
    void InitializeBarChartsEqualization();
    void InitialiseBarChartsAdjustments();
    void DistributeAdjHistogramsInGridLayout();
    void UpdateHeatMap(int * _data, int sizex, int sizey);
    std::string BuildChartName(int val, const QString &leg);


    //! Getters and setters etc.
    void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; }
    Ui::QCstmEqualization * GetUI();
    static QCstmEqualization* getInstance();

    QMap<int, Mpx3EqualizationResults *> getEqMap(){ return _eqMap; }
    equalizationSteeringInfo * GetSteeringInfo(int chipIdx);
    BarChart * GetAdjBarChart(int chipIdx, Mpx3EqualizationResults::lowHighSel sel);
    BarChart * GetBarChart(int chipIdx);
    QCheckBox * GetCheckBox(int chipIdx);

    bool isScanDescendant() { return _scanDescendant; }
    bool isBusy() { return _busy; }
    bool getScanningAllChips() { return _scanAllChips; }
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
    void setWindowWidgetsStatus(win_status s = win_status::startup);
    bool getEqualisationHasBeenLoaded(){ return _equalisationLoaded; }


private:

    Ui::QCstmEqualization * _ui = nullptr;
    Mpx3GUI * _mpx3gui = nullptr;
    QApplication * _coreApp = nullptr;

    // Equalization info
    QMap<int, Mpx3EqualizationResults *> _eqMap;
    vector<BarChart * > _chart = {nullptr};                 //! charts for all chips
    vector<QCheckBox * > _checkBoxes = {nullptr};           //! checkBoxes for all chips
    vector<BarChart * > _adjchart_L = {nullptr};            //! adjustment low bits charts
    vector<BarChart * > _adjchart_H = {nullptr};            //! adjustment high bits charts
    QGridLayout * _gridLayoutHistograms = nullptr;
    Dataset * _resdataset = nullptr;

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

    bool _turn_on_CSM_for_THH = false;
    // -------------------------------------------------------------------------

    QStringList files = {};
    bool _busy = false;
    bool _isRemotePath = false;
    QString _remotePath = "";

    bool _equalisationLoaded = false;
    bool _isSequentialAllChipsEqualization = false;
    int _setId;
    int _deviceIndex;
    int _nTriggers; //! There really is no need to sample the noise multiple times
    int _spacing; //! Pixel spacing. Ie. a pixel spacing of 2 would be scannig 1/4 pixels
    int _userChosenSpacing = -1;
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
    vector<uint> _workChipsIndx;
    uint _eqStatus; //! Important state machine variable
    uint _scanIndex;
    enum {
        __THLandTHH = 0,
        __OnlyTHL,
        __OnlyTHH,
        __nEQCombinations
    };
    int _equalisationCombination;
    int _prevEqualizationCombination;
    QString _tempEqSaveDir;

    enum {
        __NoiseEdge = 0,
        __NoiseCentroid,  // above this, fast equalizations
        __NoiseEdgeFAST,
        __NoiseCentroidFAST,
        __nEQTypes
    };
    int _equalisationType;
    Mpx3EqualizationResults::lowHighSel _equalisationShow;
    vector<equalizationSteeringInfo *> _steeringInfo;
    std::array<bool, __EQStatus_Count> _stepDone;

    // Object in charge of performing Thl scans
    QVector<ThlScan * > _scans  = {nullptr};

    // IP source address (SPIDR network interface)
    int _srcAddr;
    int _nChips;
    bool _scanDescendant;
    bool _stopEq = false;

    bool makeTeaCoffeeDialog();

    void safeCopy(QString copyFrom, QString copyTo, QString files);

public slots:
    void SaveEqualization(const QString &path = "", bool toTempDir = false, bool fetchfromtempdir = false);
    void on_h1LogyCheckBox_toggled(bool checked);

private slots:

    void setFineTuningLoops(int);
    void setNHits(int);
    void ScanThreadFinished();
    void StartEqualizationSingleChip();
    void StartEqualizationAllChips();

    //! Addresses the weird behaviour when equalising all chips simultaneously
    void StartEqualizationSequentialSingleChips();
    void StartEqualizationSequentialSingleChipsRemotely(const QString &path);
    void setNTriggers(int);
    void setDeviceIndex(int, bool uisetting = false);
    void setSpacing(int);
    void setMin(int, bool uisetting = false);
    void setMax(int, bool uisetting = false);
    void setStep(int, bool uisetting = false);
    void ConnectionStatusChanged(bool);
    void StopEqualization();
    void setEqualizationTHLTHH(int);
    void setEqualizationShowTHLTHH(int);
    void setEqualizationTHLType(int);
    void ShowEqualizationForChip();

    void on_pushButton_testPulses_clicked();

signals:
    void slideAndSpin(int, int);
    void stop_data_taking_thread();

    //! Status bar signal functions
    void sig_statusBarClean();
    void sig_statusBarAppend(QString mess, QString colorString);

    void busy(SERVER_BUSY_TYPE);
    void equalizationPathExported(QString path);
};

#endif // QCSTMEQUALIZATION_H
