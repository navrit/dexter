/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#ifndef _BAR_CHART_H_
#define _BAR_CHART_H_

#include <QObject>
#include <QWidget>
#include <QResizeEvent>

#include "qcustomplot.h"

#include <vector>
#include <string>
using namespace std;

#define __max_number_of_sets 100

class BarChartProperties {

public:
	string name;
	string xAxisLabel;
	string yAxisLabel;
	int min_x;
	int max_x;
	int nBins;
	int color_r;
	int color_g;
	int color_b;
};

class BarChart: public QCustomPlot
{

public:

    explicit BarChart( QWidget * parent );
	~BarChart();

	QCPBars * GetDataSet(int id) { return _barSets.at(id); };
	vector<BarChartProperties> GetBarChartProperties(){ return _bp; };
    void AppendSet(BarChartProperties, bool showlegend = true);
	unsigned int GetNsets(){return _nSets; };


	void SetLogY(bool setl);
	void SetLogX(bool setl);

	void SetValueInSet(unsigned int setId, double val, double weight = 1);
    void SetValueInSetNonAcc(unsigned int setId, double val, double weight = 1);

    //void PushBackToSet(unsigned int setId, double val, double weight = 1);
	//void DumpData();
	void Clean();

    //void mouseDoubleClickEvent(QMouseEvent *event);

    void fitToHeight(double min = 0.);

private:

	QWidget * _parent;
	vector<QCPBars * > _barSets;
	// The first dimension is created as 'PrepareSets' is called
	QVector< QVector<double> * > * _dataKeys;
	QVector< QVector<double> * > * _dataVals;
	vector<BarChartProperties> _bp;
	// extracted from the properties @ PrepareSets
	unsigned int _nSets;

};

#endif
