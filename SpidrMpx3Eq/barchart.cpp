/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#include "barchart.h"

#include <iostream>
using namespace std;

BarChart::BarChart( QWidget * parent ):
																		QCustomPlot(),
																		_nSets(0)
{
	_parent = parent;
	Clean();

}

BarChart::~BarChart() {

}

void BarChart::SetLogY(bool setl) {
	if ( setl ) yAxis->setScaleType( QCPAxis::stLogarithmic );
	else yAxis->setScaleType( QCPAxis::stLinear );
}

void BarChart::SetLogX(bool setl) {
	if ( setl ) xAxis->setScaleType( QCPAxis::stLogarithmic );
	else xAxis->setScaleType( QCPAxis::stLinear );
}

void BarChart::Clean() {

	int nCleared = this->clearPlottables();
    //cout << "[INFO] Number of plots cleared : " << nCleared << endl;

	_nSets = 0;
	_barSets.clear();
	_bp.clear();

	replot();

}

void BarChart::AppendSet(BarChartProperties bp) {

	// Create the set and push it back in the vector of sets
	_barSets.push_back( new QCPBars(this->xAxis, this->yAxis) );
	// And add it to the plot
	this->addPlottable( _barSets[_nSets] );
	// Name it
	_barSets[_nSets]->setName(bp.name.c_str());

	QPen pen;
	pen.setWidthF(1.2);
	// Give it a color
	QColor penC = QColor(bp.color_r, bp.color_g, bp.color_b );
	pen.setColor( penC );
	_barSets[_nSets]->setPen(pen);
	QColor brushC = QColor(bp.color_r, bp.color_g, bp.color_b, 70 );
	_barSets[_nSets]->setBrush( brushC );

	// Only when creating the first set work out the axis and legend
	if( _nSets == 0 ) {
		// prepare x axis
		QVector<double> ticks;
		this->xAxis->setAutoTicks(true);
		this->xAxis->setAutoTickLabels(true);
		this->xAxis->setSubTickCount(0);
		this->xAxis->setTickLength(0, 4);
		this->xAxis->grid()->setVisible(true);
		this->xAxis->setRange(0, bp.max_x);
		this->xAxis->setLabel(bp.xAxisLabel.c_str());

		// prepare y axis:
		this->yAxis->setRange(0, 100);
		this->yAxis->setPadding(5); // a bit more space to the left border
		this->yAxis->setLabel(bp.yAxisLabel.c_str());
		this->yAxis->grid()->setSubGridVisible(true);

		QPen gridPen;
		gridPen.setStyle(Qt::SolidLine);
		gridPen.setColor(QColor(0, 0, 0, 25));
		this->yAxis->grid()->setPen(gridPen);
		gridPen.setStyle(Qt::DotLine);
		this->yAxis->grid()->setSubGridPen(gridPen);

		// setup legend:
		this->legend->setVisible(true);
		this->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignHCenter);
		this->legend->setBrush(QColor(255, 255, 255, 200));
		QPen legendPen;
		legendPen.setColor(QColor(130, 130, 130, 200));
		this->legend->setBorderPen(legendPen);
		QFont legendFont = font();
		legendFont.setPointSize(10);
		this->legend->setFont(legendFont);
		this->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

		// Prepare data.  Already a total of __max_number_of_sets if needed
		_dataKeys = new QVector< QVector<double> * >(__max_number_of_sets, new QVector<double>() );
		_dataVals = new QVector< QVector<double> * >(__max_number_of_sets, new QVector<double>() );

	}

	// Prepare x axis for this set, and fill with zeroes all bins
	for(int i = 0 ; i <= bp.max_x - bp.min_x ; i++) {
		_dataKeys->at(_nSets)->push_back( i );
		_dataVals->at(_nSets)->push_back( 0 );
	}

	// Connect data
	_barSets[_nSets]->setData( *(_dataKeys->at(_nSets)), *(_dataVals->at(_nSets)) );

	// Keep a quick track of the BarChartProperties using this member vector
	_bp.push_back( bp );
	// Ready for next set
	_nSets++;

}

void BarChart::SetValueInSet(unsigned int setId, double val, double weight) {

	// Extract the content
	QCPBarData bin_content = (*(GetDataSet(setId)->data()))[val];
	// GetDataSet(setId)->addData( bin_content.key , bin_content.value + weight );
	// Add the extra counts to this Thl value
	bin_content.value = bin_content.value + weight;
	// And put it back in the bin
	(*(GetDataSet(setId)->data()))[val] = bin_content;

	// Revisit limits // TODO .. not working yet
	QCPRange rangeY = this->yAxis->range();
	if ( rangeY.upper < bin_content.value + weight ) {
		this->yAxis->setRange(0, ( bin_content.value + weight) * 1.1 );
	}

	replot();
}

/*
void BarChart::PushBackToSet(unsigned int setId, double val, double weight) {

	if ( setId >= _nSets ) {
		cout << "[ERRO] Trying to fit a set out of range (BarChart::PushBackToSet)" << endl;
		return;
	}

	// Increase the value at the particular location
	double a = _dataVals->at(setId)->at( val );
	a += weight;
	(*(_dataVals->at(setId)))[val] = a;

}

void BarChart::DumpData() {

	QVector< QVector<double> * >::iterator itr = _dataVals->begin();
	QVector< QVector<double> * >::iterator itrE = _dataVals->end();

	QVector<double>::iterator i;
	QVector<double>::iterator iE;

	// Set
	for ( ; itr != itrE ; itr++ ) {
		// Data
		i = (*itr)->begin();
		iE = (*itr)->end();
		if ( !(*itr)->empty() ) { cout << "< "; }
		else { cout << "empty" << endl; }
		int cntr = 0;
		int sizeV = (*itr)->size();
		bool dotsFlg = false;
		for ( ; i != iE ; i++ ) {
			// only print first 10 and last 10 elements
			if ( cntr < 10 || cntr > sizeV - 10 ) {
				cout << (*i);
				if( i+1 != iE ) cout << ", ";
				else cout << " >" << endl;
			} else {
				if ( !dotsFlg ) { cout << " ... "; dotsFlg = true; }
			}
			cntr++;
		}
	}

}
 */

