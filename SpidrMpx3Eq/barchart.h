/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#ifndef _BAR_CHART_H_
#define _BAR_CHART_H_

#include "qcustomplot.h"

#include <vector>
#include <string>
using namespace std;

class BarChartProperties {

public:
	vector<string> name;
	vector<int> min_x;
	vector<int> max_x;
	vector<int> nBins;
	vector<int> color_r;
	vector<int> color_g;
	vector<int> color_b;
};

class BarChart: public QCustomPlot
{

public:

	BarChart( QWidget * parent );
	~BarChart();

	QCPBars * GetDataSet(int id) { return _barSets.at(id); };
	void SetBarChartProperties(BarChartProperties * bp) { _bp = bp; };
	BarChartProperties * GetBarChartProperties(){ return _bp; };
	void PrepareSets();

	void SetValueInSet(unsigned int setId, double val, double weight = 1);
	//void PushBackToSet(unsigned int setId, double val, double weight = 1);
	//void DumpData();

private:

	QWidget * _parent;
	vector<QCPBars * > _barSets;
	// The first dimension is created as 'PrepareSets' is called
	QVector< QVector<double> * > * _dataKeys;
	QVector< QVector<double> * > * _dataVals;

	BarChartProperties * _bp;
	// extracted from the properties @ PrepareSets
	unsigned int _nSets;

};

#endif
