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
}

BarChart::~BarChart() {

}

void BarChart::PrepareSets() {

	unsigned int nSets = (unsigned int)_bp->name.size();
	_nSets = nSets;
	for (unsigned int iSet = 0 ; iSet < nSets ; iSet++ ) {
		// Create the set
		_barSets.push_back( new QCPBars(this->xAxis, this->yAxis) );
		// And add it to the plot
		this->addPlottable( _barSets[iSet] );
		// Name it
		QPen pen;
		pen.setWidthF(1.2);
		_barSets[iSet]->setName(_bp->name[iSet].c_str());
		// Give it a color
		QColor penC = QColor(_bp->color_r[iSet], _bp->color_r[iSet], _bp->color_r[iSet] );
		pen.setColor( penC );
		_barSets[iSet]->setPen(pen);
		QColor brushC = QColor(_bp->color_r[iSet], _bp->color_r[iSet], _bp->color_r[iSet], 70 );
		_barSets[iSet]->setBrush( brushC );
		// stack bars ontop of each other:
		//nuclear->moveAbove(fossil);
		//regen->moveAbove(nuclear);
	}

	// prepare x axis
	QVector<double> ticks;
	this->xAxis->setAutoTicks(true);
	this->xAxis->setAutoTickLabels(true);
	this->xAxis->setSubTickCount(0);
	this->xAxis->setTickLength(0, 4);
	this->xAxis->grid()->setVisible(true);
	this->xAxis->setRange(0, 511);

	// prepare y axis:
	this->yAxis->setRange(0, 100);
	this->yAxis->setPadding(5); // a bit more space to the left border
	this->yAxis->setLabel("entries");
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


	// Prepare data.  As many vectors as sets.
	_dataKeys = new QVector< QVector<double> * >(_nSets, new QVector<double>() );
	_dataVals = new QVector< QVector<double> * >(_nSets, new QVector<double>() );

	for (unsigned int iSet = 0 ; iSet < nSets ; iSet++ ) {
		// Prepare x axis for this set, and fill with zeroes all bins
		for(int i = 0 ; i < _bp->max_x[iSet] - _bp->min_x[iSet] ; i++) {
			_dataKeys->at(iSet)->push_back( i );
			_dataVals->at(iSet)->push_back( 0 );
		}
		// Connect data
		_barSets[iSet]->setData( *(_dataKeys->at(iSet)), *(_dataVals->at(iSet)) );
	}

}

void BarChart::SetValueInSet(unsigned int setId, double val, double weight) {

	QCPBarData bin_content = (*(GetDataSet(setId)->data()))[val];
	GetDataSet(setId)->addData( bin_content.key , bin_content.value + weight );

	// Revisit limits
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

