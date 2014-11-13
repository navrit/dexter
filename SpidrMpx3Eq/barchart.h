/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#ifndef _BAR_CHART_H_

#include "qcustomplot.h"

#include <vector>
#include <string>
using namespace std;

class BarChartProperties {

public:
	vector<string> name;
	vector<int> min;
	vector<int> max;
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
	void SetBarChartProperties(BarChartProperties * bp) { _bp = bp; };
	void PrepareSets();
	void PushBackToSet(unsigned int setId, double val, double weight = 1);

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
