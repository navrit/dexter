/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#ifndef MPX3EQUALIZATION_H
#define MPX3EQUALIZATION_H

#include "ui_mpx3gui.h"
#include "mpx3gui.h"
#include "mpx3defs.h"

//#include <QImage>
#include <QMainWindow>

#include <iostream>
#include <vector>

#include <qcustomplot.h>

using namespace std;

#include "histogram.h"
#include "mpx3eq_common.h"
#include "ThlScan.h"

#define __matrix_size_x 		256
#define __matrix_size_y 		256
#define __equalization_target	10

class QCustomPlot;
class SpidrController;
class SpidrDaq;
class DACs;
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
	int * GetAdjustementMatrix();

	void ExtrapolateAdjToTarget(int target, double eta_Adj_THL);

private:
	// pixel Id, adjustment
	map<int, int> _pixId_Adj;
	// pixel Id, reactive thlValue
	map<int, int> _pixId_Thl;

};

class Mpx3Equalization : public QWidget {
	Q_OBJECT

public:
	explicit Mpx3Equalization();
	explicit Mpx3Equalization(QApplication * coreApp, Ui::Mpx3GUI * );
	~Mpx3Equalization();

	void timerEvent( QTimerEvent * );
	void PrintFraction(int * buffer, int size, int first_last);
	int GetNPixelsActive(int * buffer, int size, verblev verbose);
	void GetSlopeAndCut_THL_IDAC_DISC(ScanResults, ScanResults, double &, double &);
	void GetSlopeAndCut_Adj_THL(ScanResults, ScanResults, double &, double &);

	double EvalLinear(double eta, double cut, double x);

	// Equalization steps
	int DAC_Disc_Optimization(int setId, int DAC_Disc_code);
	int PrepareInterpolation(int setId, int DAC_Disc_code);
	int FineTunning(int setId, int DAC_Disc_code);
	int DetectStartEqualizationRange(int setId, int DAC_Disc_code);

	void DisplayStatsInTextBrowser(int adj, int dac_disc, ScanResults res);

	pair<int, int> XtoXY(int X, int dimX);
	void SetupSignalsAndSlots();
	void SetLimits();
	void SetModuleConnection(ModuleConnection * p) { _moduleConn = p; };
	void Configuration(bool reset);
	void SetAllAdjustmentBits(int val_L, int val_H);
	void SetAllAdjustmentBits(Mpx3EqualizationResults *);
	void AppendToTextBrowser(QString s);
	void ClearTextBrowser();
	int GetDeviceIndex(){ return _deviceIndex; };
	int GetNTriggers(){ return _nTriggers; };
	int GetSpacing(){ return _spacing; };
	int GetMinScan(){ return _minScanTHL; };
	int GetMaxScan(){ return _maxScanTHL; };
	int GetStepScan(){ return _stepScan; };

	void SetMinScan(int);
	void SetMaxScan(int);

	Mpx3EqualizationResults * GetEqualizationResults() { return _eqresults; };

private:

	// Connectivity between modules
	ModuleConnection * _moduleConn;

	QStringList files;
	QMap<QString, QCPColorGradient> heatmapMap;
	SpidrController * _spidrcontrol;
	SpidrDaq * _spidrdaq;

	QApplication * _coreApp;
	Ui::Mpx3GUI * _ui;

	int _deviceIndex;
	int _nTriggers;
	int _spacing;
	int _minScanTHL;
	int _maxScanTHL;
	int _stepScan;

	int **data = 0;
	unsigned *nx =0, *ny =0, nData =0;
	histogram ** hists = 0;

	// Drawing object
	//QCustomPlot * _customPlot;
	//BarChart * _chart;

	// Object in charge of performing Thl scans
	vector<ThlScan * > _scans;

	// DACs
	DACs * _dacs;

	//
	Mpx3EqualizationResults * _eqresults;

	// Important Equalization values
	double _eta_THL_DAC_DiscL;
	double _cut_THL_DAC_DiscL;
	double _eta_THL_DAC_DiscH;
	double _cut_THL_DAC_DiscH;
	int _opt_MPX3RX_DAC_DISC_L;
	int _opt_MPX3RX_DAC_DISC_H;
	double _eta_Adj_THL;
	double _cut_Adj_THL;

private slots:

	void StartEqualization();
	void Connect();
	void ChangeNTriggers(int);
	void ChangeDeviceIndex(int);
	void ChangeSpacing(int);
	void ChangeMin(int);
	void ChangeMax(int);
	void ChangeStep(int);

	void on_heatmapCombobox_currentIndexChanged(const QString &arg1);
	void on_openfileButton_clicked();
};


#endif // MPX3EQUALIZATION_H
