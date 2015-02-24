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

enum Thl_Status {
	__NOT_TESTED_YET = 0,
};

class ThlScan {

public:

	ThlScan();
	ThlScan(BarChart *, QCstmPlotHeatmap *);
	~ThlScan();

	void ConnectToHardware(SpidrController * sc, SpidrDaq * sd);
	void RewindData();
	void DoScan();
	int SetEqualizationMask(int spacing, int offset);
	void ClearMask(){ _maskedSet.clear(); };
	void Configuration();
	void ExtractScanInfo(int * data, int size_in_bytes);
	void UpdateChart(int thlValue);

	int XYtoX(int x, int y, int dimX) { return y * dimX + x; }
	pair<int, int> XtoXY(int X, int dimX) { return make_pair(X % dimX, X/dimX); }

private:

	SpidrController * _spidrcontrol;
	SpidrDaq * _spidrdaq;
	BarChart * _chart;
	QCstmPlotHeatmap * _heatmap;

	map<int, int> _pixelCountsMap;
	set<int> _maskedSet;
	int _nTriggers;

	// Current device Id
	int _deviceIndex;

};

#endif

