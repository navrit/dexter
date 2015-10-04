#ifndef QCSTMEQUALIZATION_H
#define QCSTMEQUALIZATION_H

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

#define __equalization_target	10
#define __default_step_scan		2

#define EQ_NEXT_STEP(x) ( _eqStatus == x && ! _stepDone[x] )

class QCustomPlot;
class SpidrController;
class SpidrDaq;
//class DACs;
class QCstmDacs;
class ThlScan;
class BarChart;
class BarChartProperties;
class ModuleConnection;



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
	void maskPixel(int pixId){
		maskedPixels.insert(pixId);
	}
	void unmaskPixel(int pixId){
		if(maskedPixels.contains(pixId)) maskedPixels.remove(pixId);
	}
	int GetNMaskedPixels(){
		return maskedPixels.size();
	}
	QSet<int> GetMaskedPixels(){
		return maskedPixels;
	}
	int * GetAdjustementMatrix(lowHighSel sel = __ADJ_L);

	void ExtrapolateAdjToTarget(int target, double eta_Adj_THL, lowHighSel sel = __ADJ_L);

	void WriteAdjBinaryFile(QString fn);
	void ReadAdjBinaryFile(QString fn);
	void WriteMaskBinaryFile(QString fn);
	void ReadMaskBinaryFile(QString fn);

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

	// pixel Id, reactive thlValue
	map<int, int> _pixId_Thl_L;
	map<int, int> _pixId_Thl_H;

	// status
	map<int, eq_status> _eqStatus_L;
	map<int, eq_status> _eqStatus_H;

};


class equalizationSteeringInfo {

public:

	equalizationSteeringInfo(){};
	~equalizationSteeringInfo(){};

	void SetCurrentEta_Adj_THx(double v) { currentEta_Adj_THx = v; }

	int equalizationCombination;
	int equalizationType;
	int globalAdj;
	int currentTHx;
	QString currentTHx_String;
	int currentDAC_DISC;
	QString currentDAC_DISC_String;
	int currentDAC_DISC_OptValue;	// the optimized value
	double currentEta_THx_DAC_Disc; //<! Eta and Cut for the THx Vs DAC_DISC_x function (DAC_DISC Optimization)
	double currentCut_THx_DAC_Disc;
	double currentEta_Adj_THx; //<! Eta and Cut for the Adj Vs. THx function (Adj extrapolation)
	double currentCut_Adj_THx;


};


namespace Ui {
class QCstmEqualization;
}

class QCstmEqualization : public QWidget
{
	Q_OBJECT

public:

	explicit QCstmEqualization(QWidget *parent = 0);
	~QCstmEqualization();
	void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; };
	Ui::QCstmEqualization * GetUI();

	void timerEvent( QTimerEvent * );
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
	int * CalculateInterpolation(int devId, ThlScan * scan_x0, ThlScan * scan_x5);// ScanResults * res_x0, ScanResults * res_x5);
	void ScanOnInterpolation();
	void Rewind();
	void InitEqualization(int chipId); //<! chipId = -1  will equalize all available chips at once
	void NewRunInitEqualization(); //<! partial initialization
	bool pixelInScheduledChips(int);

	void DAC_Disc_Optimization_DisplayResults(ScanResults * res);

	int FineTunning();
	//	int DetectStartEqualizationRange(int setId, int DAC_Disc_code);

	void DisplayStatsInTextBrowser(int adj, int dac_disc, ScanResults * res);
	void KeepOtherChipsQuiet();

	pair<int, int> XtoXY(int X, int dimX);
	void SetupSignalsAndSlots();
	void SetLimits();
	void Configuration(int devId, int THx, bool reset);
	void SetAllAdjustmentBits(SpidrController * spidrcontrol, int devId, int val_L, int val_H);
	void SetAllAdjustmentBits(SpidrController * spidrcontrol, int deviceId);
	void SetAllAdjustmentBits(SpidrController * spidrcontrol);
	//void SetAllAdjustmentBits(SpidrController * spidrcontrol, int deviceId );
	void SetAllAdjustmentBits();
	void ClearAllAdjustmentBits();

	void AppendToTextBrowser(QString s);
	void ClearTextBrowser();
	int GetDeviceIndex(){ return _deviceIndex; };
	int GetNTriggers(){ return _nTriggers; };
	int GetSpacing(){ return _spacing; };
	int GetMinScan(){ return _minScanTHL; };
	int GetMaxScan(){ return _maxScanTHL; };
	int GetStepScan(){ return _stepScan; };
	int GetNHits(){ return _nHits; };
	int GetFineTuningLoops() { return _fineTuningLoops; };

	int GetNChips() {return _nChips; };

	void SetMinScan(int val = -1);
	void SetMaxScan(int val = -1);
	bool isScanDescendant() { return _scanDescendant; }
	bool isBusy() { return _busy; };
	bool scanningAllChips() { return _scanAllChips; };

	void StartEqualization(); //!<
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

	void LoadEqualization();
	void ShowEqualization(Mpx3EqualizationResults::lowHighSel sel);

	void InitializeEqualizationStructure(); //<! on a normal run, when the user load the equalization after connecting
	void RewindEqualizationStructure();

	typedef enum {
		__INIT = 0,
		__DAC_Disc_Optimization_100,
		__DAC_Disc_Optimization_150,
		__PrepareInterpolation_0x0,
		__PrepareInterpolation_0x5,
		__ScanOnInterpolation,
		__FineTunning,
		__EQStatus_Count
	} eqStatus;
	bool * _stepDone;

private:

	Ui::QCstmEqualization * _ui;
	bool _busy;
	Dataset * _resdataset;

	// Equalization info
	QMap<int, Mpx3EqualizationResults *> _eqMap;
	vector<BarChart * > _chart;			//<! charts for all chips
	vector<QCheckBox * > _checkBoxes;	//<! checkBoxes for all chips
	vector<BarChart * > _adjchart_L;			//<! adjustment charts
	vector<BarChart * > _adjchart_H;			//<! adjustment charts
	QGridLayout * _gridLayoutHistograms;

	// Connectivity between modules
	Mpx3GUI * _mpx3gui;

	QStringList files;

	QApplication * _coreApp;

	int _setId;
	int _deviceIndex;
	int _nTriggers;
	int _spacing;
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
	unsigned int _eqStatus;
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
		__nEQTypes
	};
	int _equalizationType;
	Mpx3EqualizationResults::lowHighSel _equalizationShow;
	vector<equalizationSteeringInfo *> _steeringInfo;

	// IP source address (SPIDR network interface)
	int _srcAddr;
	int _nChips;
	bool _scanDescendant;

	int **data = 0;
	unsigned *nx =0, *ny =0, nData =0;

	// Object in charge of performing Thl scans
	QVector<ThlScan * > _scans;

public slots:
void SaveEqualization();
void on_logYCheckBox_toggled(bool checked);

private slots:

void setFineTuningLoops(int);
void setNHits(int);
void ScanThreadFinished();
void StartEqualizationSingleChip();
void StartEqualizationAllChips();
void ChangeNTriggers(int);
void ChangeDeviceIndex(int);
void ChangeSpacing(int);
void ChangeMin(int);
void ChangeMax(int);
void ChangeStep(int);
void ConnectionStatusChanged();
void StopEqualization();
void CleanEqualization();
void setEqualizationTHLTHH(int);
void setEqualizationShowTHLTHH(int);
void setEqualizationTHLType(int);
void ShowEqualizationForChip(bool checked);

void on_heatmapCombobox_currentIndexChanged(const QString &arg1);
void on_openfileButton_clicked();

signals:
void slideAndSpin(int, int);
void stop_data_taking_thread();

};

#endif // QCSTMEQUALIZATION_H
