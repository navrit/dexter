#ifndef QCSTMEQUALIZATION_H
#define QCSTMEQUALIZATION_H

#include <QWidget>

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
	void SetPixelAdj(int pixId, int adj);
	void SetPixelReactiveThl(int pixId, int thl);
	int GetPixelAdj(int pixId);
	int GetPixelReactiveThl(int pixId);
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
	int * GetAdjustementMatrix();

	void ExtrapolateAdjToTarget(int target, double eta_Adj_THL);

	void WriteAdjBinaryFile(QString fn);
	void ReadAdjBinaryFile(QString fn);
	void WriteMaskBinaryFile(QString fn);
	void ReadMaskBinaryFile(QString fn);

	void ClearAdj();
	void ClearMasked();
	void ClearReactiveThresholds();
	void Clear();

private:
	// pixel Id, adjustment
	/*map<int, int> */
	QByteArray _pixId_Adj;
	QSet<int> maskedPixels;
	// pixel Id, reactive thlValue
	map<int, int> _pixId_Thl;

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
	void GetSlopeAndCut_IDAC_DISC_THL(ScanResults, ScanResults, double &, double &);
	void GetSlopeAndCut_Adj_THL(ScanResults, ScanResults, double &, double &);

	double EvalLinear(double eta, double cut, double x);

	// Equalization steps
	void DAC_Disc_Optimization_100(int DAC_Disc_code, int DAC_DISC_testValue);
	void DAC_Disc_Optimization_150(int DAC_Disc_code, int DAC_DISC_testValue);
	void DAC_Disc_Optimization(ScanResults res_100, ScanResults res_150);
	void PrepareInterpolation_0x0(int DAC_Disc_code);
	void PrepareInterpolation_0x5(int DAC_Disc_code);
	void CalculateInterpolation(ScanResults res_x0, ScanResults res_x5);
	void ScanOnInterpolation(int DAC_Disc_code);
	void Rewind();
	void InitEqualization();

	void DAC_Disc_Optimization_DisplayResults(ScanResults res);

	int FineTunning(int setId, int DAC_Disc_code);
//	int DetectStartEqualizationRange(int setId, int DAC_Disc_code);

	void DisplayStatsInTextBrowser(int adj, int dac_disc, ScanResults res);

	pair<int, int> XtoXY(int X, int dimX);
	void SetupSignalsAndSlots();
	void SetLimits();
	void Configuration(bool reset);
	void SetAllAdjustmentBits(SpidrController * spidrcontrol, int val_L, int val_H);
	void SetAllAdjustmentBits(SpidrController * spidrcontrol);
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

	void SetMinScan(int);
	void SetMaxScan(int);

	void StartEqualization(int chipId);

	Mpx3EqualizationResults * GetEqualizationResults(int chipIndex);

	void LoadEqualization();

	typedef enum {
		__INIT = 0,
		__DAC_Disc_Optimization_100,
		__DAC_Disc_Optimization_150,
		__PrepareInterpolation_0x0,
		__PrepareInterpolation_0x5,
		__ScanOnInterpolation,
		__EQStatus_Count
	} eqStatus;
	bool * _stepDone;

private:

	Ui::QCstmEqualization * _ui;

	// Equalization info
	Mpx3EqualizationResults * _eqresults;
	QVector<Mpx3EqualizationResults *> _eqVector;

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
	bool _threadFinished;
	unsigned int _eqStatus;
	// IP source address (SPIDR network interface)
	int _srcAddr;
	int _nChips;

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

private slots:

void ScanThreadFinished();
void StartEqualization();
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

void on_heatmapCombobox_currentIndexChanged(const QString &arg1);
void on_openfileButton_clicked();

signals:
	void slideAndSpin(int, int);


};

#endif // QCSTMEQUALIZATION_H
