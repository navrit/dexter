#ifndef QCSTMEQUALIZATION_H
#define QCSTMEQUALIZATION_H

#include <QWidget>
#include <QMap>

#include "mpx3gui.h"
#include "mpx3defs.h"
#include <qcustomplot.h>


#include <iostream>
#include <vector>

using namespace std;

#include "histogram.h"
//#include "mpx3eq_common.h"
#include "ThlScan.h"

#define __matrix_size_x 		256
#define __matrix_size_y 		256
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

	typedef enum {
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

	void WriteAdjBinaryFile(QString fn, lowHighSel sel = __ADJ_L);
	void ReadAdjBinaryFile(QString fn, lowHighSel sel = __ADJ_L);
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
	void GetSlopeAndCut_Adj_THL(ScanResults, ScanResults, double &, double &);

	double EvalLinear(double eta, double cut, double x);

	// Equalization steps
	void DAC_Disc_Optimization_100(int DAC_DISC_testValue);
	void DAC_Disc_Optimization_150(int DAC_DISC_testValue);
	void DAC_Disc_Optimization(int devId, ScanResults * res_100, ScanResults * res_150);
	void PrepareInterpolation_0x0();
	void PrepareInterpolation_0x5();
	void CalculateInterpolation(ScanResults res_x0, ScanResults res_x5);
	void ScanOnInterpolation(int DAC_Disc_code);
	void Rewind();
	void InitEqualization(int chipId); //!< chipId = -1  will equalize all available chips at once

	void DAC_Disc_Optimization_DisplayResults(ScanResults * res);

	int FineTunning(int DAC_Disc_code);
//	int DetectStartEqualizationRange(int setId, int DAC_Disc_code);

	void DisplayStatsInTextBrowser(int adj, int dac_disc, ScanResults * res);

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
	int GetGlobalAdj(){ return _global_adj; };
	int GetNHits(){ return _nHits; };
	int GetFineTuningLoops() { return _fineTuningLoops; };

	int GetNChips() {return _nChips; };

	void SetMinScan(int);
	void SetMaxScan(int);
	bool isScanDescendant() { return _scanDescendant; }
	bool isBusy() { return _busy; };

	void StartEqualization(); //!<
	void SetDAC_propagateInGUI(SpidrController * spidrcontrol, int devId, int dac_code, int dac_val);

	Mpx3EqualizationResults * GetEqualizationResults(int chipIndex);
	void InitializeBarCharts();
	BarChart * GetBarChart(int chipIdx);
	QCheckBox * GetCheckBox(int chipIdx);

	string BuildChartName(int val, QString leg);

	void LoadEqualization();

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

	// Equalization info
	QMap<int, Mpx3EqualizationResults *> _eqMap;
	vector<BarChart * > _chart;			//<! charts for all chips
	vector<QCheckBox * > _checkBoxes;	//<! checkBoxes for all chips
	set<int> _chartShownIndx;

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
	typedef struct {
		int currentTHx;
		QString currentTHx_String;
		int currentDAC_DISC;
		QString currentDAC_DISC_String;
		int currentDAC_DISC_OptValue;
		double currentEta_THL_DAC_Disc;
		double currentCut_THL_DAC_Disc;
		int equalizationCombination;
		int equalizationType;
	} equalizationSteeringInfo;
	equalizationSteeringInfo _steeringInfo;

	// IP source address (SPIDR network interface)
	int _srcAddr;
	int _nChips;
	bool _scanDescendant;

	int **data = 0;
	unsigned *nx =0, *ny =0, nData =0;
//	histogram ** hists = 0;

	// Drawing object
	//QCustomPlot * _customPlot;
	//BarChart * _chart;

	// Object in charge of performing Thl scans
	QVector<ThlScan * > _scans;

	// Important Equalization values
	double _eta_THL_DAC_DiscL;
	double _cut_THL_DAC_DiscL;
	double _eta_THL_DAC_DiscH;
	double _cut_THL_DAC_DiscH;
	int _opt_MPX3RX_DAC_DISC_L;
	int _opt_MPX3RX_DAC_DISC_H;
	int _global_adj;
	double _eta_Adj_THL;
	double _cut_Adj_THL;

public slots:
void SaveEqualization( int chipId );
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
void setEqualizationTHLType(int);
void ShowEqualizationForChip(bool checked);

void on_heatmapCombobox_currentIndexChanged(const QString &arg1);
void on_openfileButton_clicked();

signals:
	void slideAndSpin(int, int);
	void stop_data_taking_thread();

};

#endif // QCSTMEQUALIZATION_H
