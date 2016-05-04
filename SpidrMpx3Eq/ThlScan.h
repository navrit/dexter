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
#include <QtWidgets>

#include "mpx3gui.h"


using namespace std;

class SpidrController;
class SpidrDaq;
class BarChart;
class QCstmPlotHeatmap;
class QCstmEqualization;
class Mpx3EqualizationResults;
class equalizationSteeringInfo;

enum Thl_Status {
	__UNDEFINED = -1,
	__NOT_TESTED_YET = 0,
};


#define  __accelerationStartLimit	50
#define __step_scan_boostfactor		10

class ScanResults {
public:
	double weighted_arithmetic_mean;
	double sigma;
	int DAC_DISC_setting;
	int global_adj;
	int chipIndx;
};


class ThlScan : public QThread {

	Q_OBJECT

public:

	//explicit ThlScan();
	explicit ThlScan(Mpx3GUI *, QCstmEqualization *);
	//~ThlScan();

	typedef enum {
		__adjust_to_global = 0,
		__adjust_to_equalizationMatrix
	} adj_type;

	void SetAdjustmentType(adj_type t){ _adjType = t; };

	typedef enum {
		__BASIC_SCAN = 0,
		__FINE_TUNNING1_SCAN
	} scan_type;

	void ConnectToHardware(SpidrController * sc, SpidrDaq * sd);
	void RewindData(int full_sizex, int full_sizey);
	void DoScan(int dac_code, int setId, int DAC_Disc_code, int numberOfLoops = -1, bool blindScan = false);
    bool SetEqualizationMask(SpidrController * sc, int devId, int spacing, int offset_x, int offset_y, int * nmasked);
	int SetEqualizationMask(SpidrController * sc, set<int> reworkPixels);
	set<int> GetReworkSubset(set<int> reworkSet, int spacing, int offset_x, int offset_y);

	void ClearMask(SpidrController * spidrcontrol, int devId, bool ClearMask = true);
	int ExtractScanInfo(int * data, int size_in_bytes, int thl);
	int ExtractScanInfo(int * data, int size_in_bytes, int thl, int);
	bool OutsideTargetRegion(int devId, int pix, double Nsigma);
	void SetMinScan(int val = -1);
	void SetMaxScan(int val = -1);

	void ExtractStatsOnChart(int devId, int setId);
	int NumberOfNonReactingPixels();
	vector<int> GetNonReactingPixels();
	void SetConfigurationToScanResults(int DAC_DISC_setting, int global_adj);
	void SetStopWhenPlateau(bool b) { _stopWhenPlateau = b; };

	void DeliverPreliminaryEqualization(int devId, int currentDAC_DISC, Mpx3EqualizationResults *, int global_adj);

	int XYtoX(int x, int y, int dimX) { return y * dimX + x; }
	pair<int, int> XtoXY(int X, int dimX) { return make_pair(X % dimX, X/dimX); }

	int GetDetectedLowScanBoundary() { return _detectedScanBoundary_L; };
	int GetDetectedHighScanBoundary() { return _detectedScanBoundary_H; };

	void FineTuning();
	void EqualizationScan();
	void SetDAC_propagateInGUI(SpidrController * spidrcontrol, int devId, int dac_code, int dac_val );

	void SetScanType(scan_type st) { _scanType = st; };
	scan_type GetScanType() { return _scanType; };
	set<int> ExtractPixelsNotOnTarget();
	int ExtractReworkSubsetSpacingAware(set<int> & reworkPixelsSet, set<int> & reworkSubset, int spacing);
	bool TwoPixelsRespectMinimumSpacing(int pix1, int pix2, int spacing);
	void SelectBestAdjFromHistory(int showHeadAndTail);
	int ShiftAdjustments(SpidrController * spidrcontrol, set<int> reworkSubset, set<int> activeMask);
	bool AdjScanCompleted(set<int> reworkSubset, set<int> activeMask);
	void TagPixelsEqualizationStatus(set<int> vetoList);
	void RewindReactionCounters(set<int> reworkPixelsSet);
	void DumpSet(set<int> reworkSubset, QString name, int max = 100);
	void FillAdjReactTHLHistory();
	void DumpAdjReactTHLHistory(int showHeadAndTail);
	int PixelBelonsToChip(int pix);
	bool ThereIsAFalse(vector<bool> v);

	void SetSetId(int si) { _setId = si; };
	int GetSetId() { return _setId; };

	void SetWorkChipIndexes(vector<int> v, vector<equalizationSteeringInfo *> st);
	vector<int> GetWorkChipIndexes() { return _workChipsIndx; };

	void InitializeScanResults(vector<equalizationSteeringInfo *> st);
	ScanResults * GetScanResults(int chipIdx);

private:

	void run();

	Mpx3GUI * _mpx3gui;
	QCstmEqualization * _equalization;

	SpidrController * _spidrcontrol;
	SpidrDaq * _spidrdaq;
	QCstmPlotHeatmap * _heatmap;

	vector<ScanResults *> _results;		//<! results for all chips
	vector<int> _workChipsIndx;
	Dataset * _dataset;

	// pixelId, counts map
	map<int, int> _pixelCountsMap;
	// pixelId, reactive thl
	map<int, int> _pixelReactiveTHL;
	set<int> _maskedSet;
	// Holder for pixels ready.  Used at the beginning of the fine tuning procedure
	set<int> _fineTunningPixelsEqualized;
	set<int> _scheduledForFineTuning;

	// Dedicated to Fine Tuning
	// Keeping track of the reactive threshold for every adj value
	// pixId ---> < (adj,reactTHL) ... >
	map<int, vector< pair<int, int> > > _adjReactiveTHLFineTuning;

	int _nReactivePixels;
	scan_type _scanType;

	// Last scan boundaries
	// This information could be useful for a next scan
	int _detectedScanBoundary_L;
	int _detectedScanBoundary_H;

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
	int _nchipsX;
	int _nchipsY;
	// IP source address (SPIDR network interface)
	int _srcAddr;
	int _fullsize_x;
	int _fullsize_y;

	// For data taking
	int * _data;
	int * _plotdata;
	int _frameId;
	int _thlItr;
	int _pixelReactiveInScan;

	// control
	bool _stop;

	private slots:
	//void UpdateChart(int setId, int thlValue);
	void UpdateChart(int devId, int setId, int thlValue);
	void UpdateChartPixelsReady(int devId, int setId);
	void UpdateHeatMap(int sizex, int sizey);
	void on_stop_data_taking_thread();

	signals:
	void UpdateChartSignal(int devId, int setId, int thlValue);
	void UpdateChartPixelsReadySignal(int devId, int setId);
	void UpdateHeatMapSignal(int sizex, int sizey);
	void fillText(QString);
	void slideAndSpin(int, int);

};

#endif

