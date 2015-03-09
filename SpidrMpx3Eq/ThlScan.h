/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#ifndef THLSCAN_H
#define THLSCAN_H

#include <map>
#include <set>
using namespace std;


class SpidrController;
class SpidrDaq;
class BarChart;
class QCstmPlotHeatmap;
class Mpx3Equalization;
class Mpx3EqualizationResults;

enum Thl_Status {
	__UNDEFINED = -1,
	__NOT_TESTED_YET = 0,
};

class ScanResults {
public:
	double weighted_arithmetic_mean;
	double sigma;
	int DAC_DISC_setting;
	int global_adj;
};



class ThlScan {

public:

	ThlScan();
	ThlScan(BarChart *, QCstmPlotHeatmap *, Mpx3Equalization *);
	~ThlScan();

	void ConnectToHardware(SpidrController * sc, SpidrDaq * sd);
	void RewindData();
	void DoScan(int dac_code, int setId, int numberOfLoops = -1, bool blindScan = false);
	int SetEqualizationMask(int spacing, int offset_x, int offset_y);
	void ClearMask(bool ClearMask = true);
	void ExtractScanInfo(int * data, int size_in_bytes, int thl);
	void UpdateChart(int setId, int thlValue);
	void ExtractStatsOnChart(int setId);
	ScanResults GetScanResults() { return _results; };
	int NumberOfNonReactingPixels();
	void SetConfigurationToScanResults(int DAC_DISC_setting, int global_adj);

	Mpx3EqualizationResults * DeliverPreliminaryEqualization(ScanResults);

	int XYtoX(int x, int y, int dimX) { return y * dimX + x; }
	pair<int, int> XtoXY(int X, int dimX) { return make_pair(X % dimX, X/dimX); }

	int GetDetectedLowScanBoundary() { return _detectedScanBoundary_L; };
	int GetDetectedHighScanBoundary() { return _detectedScanBoundary_H; };

	int ReadjustPixelsOff(double N);

private:

	SpidrController * _spidrcontrol;
	SpidrDaq * _spidrdaq;
	BarChart * _chart;
	QCstmPlotHeatmap * _heatmap;
	Mpx3Equalization * _equalization;
	ScanResults _results;

	// pixelId, counts map
	map<int, int> _pixelCountsMap;
	// pixelId, reactive thl
	map<int, int> _pixelReactiveTHL;

	// Last scan boundaries
	// This information could be useful for a next scan
	int _detectedScanBoundary_L;
	int _detectedScanBoundary_H;

	set<int> _maskedSet;

};

#endif

