/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#ifndef THLSCAN_H
#define THLSCAN_H

#include <map>
#include <set>
#include <vector>
#include <QThread>

#include "mpx3gui.h"

using namespace std;

class SpidrController;
class SpidrDaq;
class BarChart;
class QCstmPlotHeatmap;
class QCstmEqualization;
class Mpx3EqualizationResults;

enum Thl_Status {
	__UNDEFINED = -1,
	__NOT_TESTED_YET = 0,
};

#define  __accelerationStartLimit  50

class ScanResults {
public:
	double weighted_arithmetic_mean;
	double sigma;
	int DAC_DISC_setting;
	int global_adj;
};


class ThlScan : public QThread {

	Q_OBJECT

public:

	//explicit ThlScan();
	explicit ThlScan(Mpx3GUI *, QCstmEqualization *);
	//~ThlScan();



	void ConnectToHardware(SpidrController * sc, SpidrDaq * sd);
	void RewindData();
	void DoScan(int dac_code, int setId, int DAC_Disc_code, int numberOfLoops = -1, bool blindScan = false);
	int SetEqualizationMask(SpidrController * sc, int spacing, int offset_x, int offset_y);
	void ClearMask(SpidrController * spidrcontrol, bool ClearMask = true);
	int ExtractScanInfo(int * data, int size_in_bytes, int thl);
	ScanResults GetScanResults() { return _results; };
	void ExtractStatsOnChart(int setId);
	int NumberOfNonReactingPixels();
	vector<int> GetNonReactingPixels();
	void SetConfigurationToScanResults(int DAC_DISC_setting, int global_adj);
	void SetStopWhenPlateau(bool b) { _stopWhenPlateau = b; };

	void DeliverPreliminaryEqualization(Mpx3EqualizationResults *, ScanResults);

	int XYtoX(int x, int y, int dimX) { return y * dimX + x; }
	pair<int, int> XtoXY(int X, int dimX) { return make_pair(X % dimX, X/dimX); }

	int GetDetectedLowScanBoundary() { return _detectedScanBoundary_L; };
	int GetDetectedHighScanBoundary() { return _detectedScanBoundary_H; };

	int ReAdjustPixelsOff(double Nsigma, int DAC_Disc_code);

	typedef enum {
		__adjust_to_global = 0,
		__adjust_to_equalizationMatrix
	} adj_type;

	void SetAdjustmentType(adj_type t){ _adjType = t; };


private:

	void run();

	Mpx3GUI * _mpx3gui;
	QCstmEqualization * _equalization;

	SpidrController * _spidrcontrol;
	SpidrDaq * _spidrdaq;
	BarChart * _chart;
	QCstmPlotHeatmap * _heatmap;
	ScanResults _results;

	// pixelId, counts map
	map<int, int> _pixelCountsMap;
	// pixelId, reactive thl
	map<int, int> _pixelReactiveTHL;
	int _nReactivePixels;

	// Last scan boundaries
	// This information could be useful for a next scan
	int _detectedScanBoundary_L;
	int _detectedScanBoundary_H;

	set<int> _maskedSet;
	bool _stopWhenPlateau;

	// Scan parameters
	adj_type _adjType;
	int _spacing;
	int _minScan;
	int _maxScan;
	int _stepScan;
	int _deviceIndex;
	int _dac_code;
	int _setId;
	int _numberOfLoops;
	bool _blindScan;
	int _DAC_Disc_code;
	// IP source address (SPIDR network interface)
	int _srcAddr;

	// For data taking
	int * _data;
	int _frameId;
	int _thlItr;
	int _pixelReactiveInScan;

	// control
	bool _stop;

	private slots:
	//void UpdateChart(int setId, int thlValue);
	void UpdateChart(int thlValue);
	void UpdateChart(int setId, int thlValue);
	void UpdateHeatMap(int sizex, int sizey);
	void on_stop_data_taking_thread();

	signals:
	void UpdateChartSignal(int setId, int thlValue);
	void UpdateHeatMapSignal(int sizex, int sizey);
	void fillText(QString);
	void slideAndSpin(int, int);

};

#endif

