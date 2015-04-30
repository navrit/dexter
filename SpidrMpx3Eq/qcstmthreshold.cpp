#include "qcstmthreshold.h"
#include "ui_qcstmthreshold.h"
#include  "qcstmdacs.h"

#include "SpidrController.h"
#include "SpidrDaq.h"

#include "mpx3gui.h"
#include "mpx3defs.h"
#include "mpx3eq_common.h"
#include "ui_mpx3gui.h"

QCstmThreshold::QCstmThreshold(QWidget *parent) :  QWidget(parent),  ui(new Ui::QCstmThreshold)
{
	ui->setupUi(this);
	QList<int> defaultSizesMain; //The ratio of the splitters. Defaults to the golden ratio because "oh! fancy".
	defaultSizesMain.append(2971215);
	defaultSizesMain.append(1836312);
	for(int i = 0; i < ui->splitter->count();i++){
		QWidget *child = ui->splitter->widget(i);
		child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		child->setMinimumSize(1,1);
	}
	ui->splitter->setSizes(defaultSizesMain);

	// Signals & Slots
	SetupSignalsAndSlots();

	// GUI defaults
	GUIDefaults();

	ui->framePlot->axisRect()->setupFullAxesBox(true);
	QCPColorScale *colorScale = new QCPColorScale(ui->framePlot);
	ui->framePlot->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
	colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
  for(int i = 0; i < MPX3RX_DAC_COUNT;i++){
      ui->scanTargetComboBox->addItem(MPX3RX_DAC_TABLE[i].name);
    }
}

QCstmThreshold::~QCstmThreshold()
{
	delete ui;
}

int QCstmThreshold::getActiveTargetCode(){
  return MPX3RX_DAC_TABLE[ui->scanTargetComboBox->currentIndex()-1].code;
}

void QCstmThreshold::StartCalibration() {

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	connect( this, SIGNAL( UpdateChartSignal(int, int) ), this, SLOT( UpdateChart(int, int) ) );
	connect( this, SIGNAL( UpdateHeatMapSignal() ), this, SLOT( UpdateHeatMap() ) );

	int dacCodeToScan = 1;
	int minScan = 0;
	int maxScan = 100;
	int stepScan = 2;
	int deviceIndex = 2;

	int pixelsReactive = 0;

	for(int itr = minScan ; itr <= maxScan ; itr += stepScan ) {

		cout << itr << endl;

		// Set Dac
		spidrcontrol->setDac( deviceIndex, dacCodeToScan, itr );
		// Adjust the sliders and the SpinBoxes to the new value
		connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
		// Get the DAC back just to be sure and then slide&spin
		int dacVal = 0;
		spidrcontrol->getDac( deviceIndex, dacCodeToScan, &dacVal);
		// SlideAndSpin works with the DAC index, no the code.
		int dacIndex = _mpx3gui->GetUI()->DACsWidget->GetDACIndex( dacCodeToScan );
		slideAndSpin( dacIndex,  dacVal );
		disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );

		// Measure
		// Start the trigger as configured
		spidrcontrol->startAutoTrigger();
		Sleep( 50 );

		// See if there is a frame available
		// I should get as many frames as triggers

		while ( spidrdaq->hasFrame() ) {

			int size_in_bytes = -1;
			_data = spidrdaq->frameData(0, &size_in_bytes);

			pixelsReactive += ExtractScanInfo( _data, size_in_bytes, itr );

			// Report to heatmap
			UpdateHeatMapSignal();

			//
			spidrdaq->releaseFrame();
			Sleep(10);

		}


	}


	disconnect( this, SIGNAL( UpdateChartSignal(int, int) ), this, SLOT( UpdateChart(int, int) ) );
	disconnect( this, SIGNAL( UpdateHeatMapSignal() ), this, SLOT( UpdateHeatMap() ) );

}

void QCstmThreshold::SetupSignalsAndSlots() {

	std::cout << "[QCstmThreshold] Connecting signals and slots" << std::endl;
	connect( ui->thlCalibStart, SIGNAL(clicked()), this, SLOT( StartCalibration() ) );

}

void QCstmThreshold::GUIDefaults() {

	//ui->thlCalibStart->
}

int QCstmThreshold::ExtractScanInfo(int * data, int size_in_bytes, int thl) {

	int nPixels = size_in_bytes/4;
	int pixelsActive = 0;
	// Each 32 bits corresponds to the counts in each pixel already
	// in 'int' representation as the decoding has been requested
	for(int i = 0 ; i < nPixels ; i++) {
		if ( data[i] != 0 ) {
			pixelsActive++;
		}
	}

	return pixelsActive;
}

void QCstmThreshold::UpdateChart(int setId, int thlValue) {
/*
	map<int, int>::iterator itr = _pixelCountsMap.begin();
	map<int, int>::iterator itrE = _pixelCountsMap.end();

	// I am going to plot for this threshold the number of
	//  pixels which reached _nTriggers counts.  The next time
	//  they won't be considered.
	int cntr = 0;
	for( ; itr != itrE ; itr++ ) {

		if( (*itr).second ==  _equalization->GetNTriggers() ) {
			cntr++;
			(*itr).second++; // This way we avoid re-ploting next time. The value _nTriggers+1 identifies these pixels
		}

	}

	_chart->SetValueInSet( setId , thlValue, cntr );
*/
}

void QCstmThreshold::UpdateHeatMap() {

	addFrame(QPoint(0,0), 0, _data);

}

/**
 * 		offset: corner of the quad
 * 		layer: threhold layer
 * 		data: the actual data
 */

void QCstmThreshold::addFrame(QPoint offset, int layer, int* data){

  while(layer >= ui->framePlot->plottableCount())
    ui->framePlot->addPlottable(new QCPColorMap(ui->framePlot->xAxis, ui->framePlot->yAxis));
  int nx = _mpx3gui->getDataset()->x(),  ny =_mpx3gui->getDataset()->y(); //TODO: grab from config.
  for(int i = 0; i < ny; i++)
    for(int j = 0; j < nx; j++)
      ((QCPColorMap*)ui->framePlot->plottable(layer))->data()->setCell(j+offset.x()*nx,ny-1-i+offset.y()*ny, data[i*nx+j]);
  ui->framePlot->rescaleAxes();
}

