/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#include <QFont>
#include <QIntValidator>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>
#include <QVBoxLayout>
//#include <QtWidgets>
#include <QPen>
#include <QSignalMapper>
//#include <QVector>

#include "DACs.h"
#include "mpx3eq_common.h"
#include "ui_spidrmpx3eq.h"
#include "SpidrController.h"
#include "SpidrDaq.h"

#include "qcustomplot.h"

DACs::DACs(){

}

DACs::DACs(Ui::SpidrMpx3Eq * ui) {

	_spidrcontrol = 0;  // Assuming no connection yet
	_spidrdaq = 0;		// Assuming no connection yet
	_ui = ui;

	// Defaults
	_scanStep = 4;
	_deviceIndex = 2;
	_ui->deviceIdSpinBox->setMaximum(3);
	_ui->deviceIdSpinBox->setMinimum(0);
	_ui->deviceIdSpinBox->setValue( _deviceIndex );
	_ui->progressBar->setValue( 0 );

	// Order widgets in vectors
	FillWidgetVectors();

	// Set limits in widgets
	SetLimits();

	// Setup connection between Sliders and SpinBoxes
	SetupSignalsAndSlots();

	// Leave a few things unnactivated
	_ui->startScanButton->setDisabled( true );


	// Prepare plot
	// Prepare the plot
	_dacScanPlot = new QCustomPlot();
	_dacScanPlot->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );

	// Insert the plot in the dialog window
	_dacScanPlot->setParent( _ui->_DACScanFrame );

	QRect hrect = ui->_histoFrame->geometry();
	_dacScanPlot->resize( hrect.size().rwidth() , hrect.size().rheight() );

	// prepare x axis
	QVector<double> ticks;
	_dacScanPlot->xAxis->setAutoTicks(true);
	_dacScanPlot->xAxis->setAutoTickLabels(true);
	_dacScanPlot->xAxis->setSubTickCount(0);
	_dacScanPlot->xAxis->setTickLength(0, 4);
	_dacScanPlot->xAxis->grid()->setVisible(true);
	_dacScanPlot->xAxis->setRange(0, 512);

	// prepare y axis:
	_dacScanPlot->yAxis->setRange(0, 4096);
	_dacScanPlot->yAxis->setPadding(5); // a bit more space to the left border
	_dacScanPlot->yAxis->setLabel("entries");
	_dacScanPlot->yAxis->grid()->setSubGridVisible(true);

	QPen gridPen;
	gridPen.setStyle(Qt::SolidLine);
	gridPen.setColor(QColor(0, 0, 0, 25));
	_dacScanPlot->yAxis->grid()->setPen(gridPen);
	gridPen.setStyle(Qt::DotLine);
	_dacScanPlot->yAxis->grid()->setSubGridPen(gridPen);

	// setup legend:
	_dacScanPlot->legend->setVisible(true);
	_dacScanPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignHCenter);
	_dacScanPlot->legend->setBrush(QColor(255, 255, 255, 200));
	QPen legendPen;
	legendPen.setColor(QColor(130, 130, 130, 200));
	_dacScanPlot->legend->setBorderPen(legendPen);
	QWidget f;
	QFont legendFont = f.font();
	legendFont.setPointSize(10);
	_dacScanPlot->legend->setFont(legendFont);
	_dacScanPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);


	int _dacIndex = 0;
	// Next graph
	_dacScanPlot->addGraph();
	_graph = _dacScanPlot->graph( _dacIndex );
	//QPen pen( COLOR_TABLE[_dacIndex] );
	QPen pen( Qt::red );
	//pen.setWidth( _comboBoxPenWidth->currentIndex()+1 );
	_graph->setPen( pen );
	//_graph->setName( QString(TPX3_DAC_TABLE[_dacIndex].name) );
	_graph->setName( QString("hola") );
	_graph->addToLegend();


	_dacScanPlot->replot();

}

DACs::~DACs() {

}

void DACs::StartDACScan() {

	if ( !_spidrcontrol ) {
		QMessageBox::information(_ui->_DACScanFrame, tr("MPX3"), tr("Connect to hardware first.") );
	}

}

void DACs::ConnectToHardware(SpidrController * sc, SpidrDaq * sd) {

	_spidrcontrol = sc;
	_spidrdaq = sd;

	// Connected, activate what was not ready to operate
	_ui->startScanButton->setDisabled( false );

}

void DACs::PopulateDACValues(){

	// Here we set the default values hardcoded in MPX3RX_DAC_TABLE (mid range)
	//   OR if the DACs file is present we read the values from it.  The file has
	//   higher priority.
	string defaultDACsFn = __default_DACs_filename;

	if ( ReadDACsFile(defaultDACsFn) ) {

		cout << "[INFO] setting dacs from defult DACs file." << endl;

		for(int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {
			_spidrcontrol->setDac( _deviceIndex, MPX3RX_DAC_TABLE[i].code, _dacVals[i] );
			_dacSpinBoxes[i]->setValue( _dacVals[i] );
			_dacSliders[i]->setValue( _dacVals[i] );
		}
		//_spidrcontrol->writeDacs( _deviceIndex );


	} else { // Setting DACs at mid-range

		for (int i = 0 ; i < 1; i++) {

			_spidrcontrol->setDac( _deviceIndex, MPX3RX_DAC_TABLE[i].code, MPX3RX_DAC_TABLE[i].dflt );
			_dacSpinBoxes[i]->setValue( MPX3RX_DAC_TABLE[i].dflt );
			_dacSliders[i]->setValue( MPX3RX_DAC_TABLE[i].dflt );

		}

	}



}

void DACs::SenseDACs() {

	int dev_nr = _ui->deviceIdSpinBox->value();
	int adc_val = 0;

	_ui->progressBar->setValue( 0 );

	for (int i = 0 ; i < MPX3RX_DAC_COUNT ; i++) {

		// Sample DAC
		_spidrcontrol->setSenseDac( MPX3RX_DAC_TABLE[i].code );
		Sleep( 100 );
		_spidrcontrol->getAdc( dev_nr, &adc_val );
		//cout << adc_val << endl;
		QString dacOut = QString::number( (__voltage_DACS_MAX/__maxADCCounts) * adc_val, 'f', 2 );
		dacOut += " V";
		_dacVLabels[i]->setText( dacOut );

		// Simple filling of the Progress bar
		_ui->progressBar->setValue( floor( ( (double)i / MPX3RX_DAC_COUNT) * 100 ) );
		//_ui->tabDACs->update();
		//_ui->gridLayout2->update();

	}

	_ui->progressBar->setValue( 0 );

}


void DACs::FillWidgetVectors() {

	// Spin boxes for the DAC values
	_dacSpinBoxes[0] = _ui->dac0SpinBox;
	_dacSpinBoxes[1] = _ui->dac1SpinBox;
	_dacSpinBoxes[2] = _ui->dac2SpinBox;
	_dacSpinBoxes[3] = _ui->dac3SpinBox;
	_dacSpinBoxes[4] = _ui->dac4SpinBox;
	_dacSpinBoxes[5] = _ui->dac5SpinBox;
	_dacSpinBoxes[6] = _ui->dac6SpinBox;
	_dacSpinBoxes[7] = _ui->dac7SpinBox;
	_dacSpinBoxes[8] = _ui->dac8SpinBox;
	_dacSpinBoxes[9] = _ui->dac9SpinBox;
	_dacSpinBoxes[10] = _ui->dac10SpinBox;
	_dacSpinBoxes[11] = _ui->dac11SpinBox;
	_dacSpinBoxes[12] = _ui->dac12SpinBox;
	_dacSpinBoxes[13] = _ui->dac13SpinBox;
	_dacSpinBoxes[14] = _ui->dac14SpinBox;
	_dacSpinBoxes[15] = _ui->dac15SpinBox;
	_dacSpinBoxes[16] = _ui->dac16SpinBox;
	_dacSpinBoxes[17] = _ui->dac17SpinBox;
	_dacSpinBoxes[18] = _ui->dac18SpinBox;
	_dacSpinBoxes[19] = _ui->dac19SpinBox;
	_dacSpinBoxes[20] = _ui->dac20SpinBox;
	_dacSpinBoxes[21] = _ui->dac21SpinBox;
	_dacSpinBoxes[22] = _ui->dac22SpinBox;
	_dacSpinBoxes[23] = _ui->dac23SpinBox;
	_dacSpinBoxes[24] = _ui->dac24SpinBox;
	_dacSpinBoxes[25] = _ui->dac25SpinBox;
	_dacSpinBoxes[26] = _ui->dac26SpinBox;

	// Sliders
	_dacSliders[0] = _ui->dac0hSlider;
	_dacSliders[1] = _ui->dac1hSlider;
	_dacSliders[2] = _ui->dac2hSlider;
	_dacSliders[3] = _ui->dac3hSlider;
	_dacSliders[4] = _ui->dac4hSlider;
	_dacSliders[5] = _ui->dac5hSlider;
	_dacSliders[6] = _ui->dac6hSlider;
	_dacSliders[7] = _ui->dac7hSlider;
	_dacSliders[8] = _ui->dac8hSlider;
	_dacSliders[9] = _ui->dac9hSlider;
	_dacSliders[10] = _ui->dac10hSlider;
	_dacSliders[11] = _ui->dac11hSlider;
	_dacSliders[12] = _ui->dac12hSlider;
	_dacSliders[13] = _ui->dac13hSlider;
	_dacSliders[14] = _ui->dac14hSlider;
	_dacSliders[15] = _ui->dac15hSlider;
	_dacSliders[16] = _ui->dac16hSlider;
	_dacSliders[17] = _ui->dac17hSlider;
	_dacSliders[18] = _ui->dac18hSlider;
	_dacSliders[19] = _ui->dac19hSlider;
	_dacSliders[20] = _ui->dac20hSlider;
	_dacSliders[21] = _ui->dac21hSlider;
	_dacSliders[22] = _ui->dac22hSlider;
	_dacSliders[23] = _ui->dac23hSlider;
	_dacSliders[24] = _ui->dac24hSlider;
	_dacSliders[25] = _ui->dac25hSlider;
	_dacSliders[26] = _ui->dac26hSlider;

	// DAC sense labels
	_dacVLabels[0] = _ui->dac0VLabel;
	_dacVLabels[1] = _ui->dac1VLabel;
	_dacVLabels[2] = _ui->dac2VLabel;
	_dacVLabels[3] = _ui->dac3VLabel;
	_dacVLabels[4] = _ui->dac4VLabel;
	_dacVLabels[5] = _ui->dac5VLabel;
	_dacVLabels[6] = _ui->dac6VLabel;
	_dacVLabels[7] = _ui->dac7VLabel;
	_dacVLabels[8] = _ui->dac8VLabel;
	_dacVLabels[9] = _ui->dac9VLabel;
	_dacVLabels[10] = _ui->dac10VLabel;
	_dacVLabels[11] = _ui->dac11VLabel;
	_dacVLabels[12] = _ui->dac12VLabel;
	_dacVLabels[13] = _ui->dac13VLabel;
	_dacVLabels[14] = _ui->dac14VLabel;
	_dacVLabels[15] = _ui->dac15VLabel;
	_dacVLabels[16] = _ui->dac16VLabel;
	_dacVLabels[17] = _ui->dac17VLabel;
	_dacVLabels[18] = _ui->dac18VLabel;
	_dacVLabels[19] = _ui->dac19VLabel;
	_dacVLabels[20] = _ui->dac20VLabel;
	_dacVLabels[21] = _ui->dac21VLabel;
	_dacVLabels[22] = _ui->dac22VLabel;
	_dacVLabels[23] = _ui->dac23VLabel;
	_dacVLabels[24] = _ui->dac24VLabel;
	_dacVLabels[25] = _ui->dac25VLabel;
	_dacVLabels[26] = _ui->dac26VLabel;

	// Check boxes
	_dacCheckBoxes[0] = _ui->dac0CheckBox;
	_dacCheckBoxes[1] = _ui->dac1CheckBox;
	_dacCheckBoxes[2] = _ui->dac2CheckBox;
	_dacCheckBoxes[3] = _ui->dac3CheckBox;
	_dacCheckBoxes[4] = _ui->dac4CheckBox;
	_dacCheckBoxes[5] = _ui->dac5CheckBox;
	_dacCheckBoxes[6] = _ui->dac6CheckBox;
	_dacCheckBoxes[7] = _ui->dac7CheckBox;
	_dacCheckBoxes[8] = _ui->dac8CheckBox;
	_dacCheckBoxes[9] = _ui->dac9CheckBox;
	_dacCheckBoxes[10] = _ui->dac10CheckBox;
	_dacCheckBoxes[11] = _ui->dac11CheckBox;
	_dacCheckBoxes[12] = _ui->dac12CheckBox;
	_dacCheckBoxes[13] = _ui->dac13CheckBox;
	_dacCheckBoxes[14] = _ui->dac14CheckBox;
	_dacCheckBoxes[15] = _ui->dac15CheckBox;
	_dacCheckBoxes[16] = _ui->dac16CheckBox;
	_dacCheckBoxes[17] = _ui->dac17CheckBox;
	_dacCheckBoxes[18] = _ui->dac18CheckBox;
	_dacCheckBoxes[19] = _ui->dac19CheckBox;
	_dacCheckBoxes[20] = _ui->dac20CheckBox;
	_dacCheckBoxes[21] = _ui->dac21CheckBox;
	_dacCheckBoxes[22] = _ui->dac22CheckBox;
	_dacCheckBoxes[23] = _ui->dac23CheckBox;
	_dacCheckBoxes[24] = _ui->dac24CheckBox;
	_dacCheckBoxes[25] = _ui->dac25CheckBox;
	_dacCheckBoxes[26] = _ui->dac26CheckBox;

}

void DACs::SetLimits() {


	for (int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {

		// Min and max for spin boxes
		_dacSpinBoxes[i]->setMinimum( 0 );
		_dacSpinBoxes[i]->setMaximum( (MPX3RX_DAC_TABLE[i].dflt * 2) - 1 );
		// Min and max for sliders
		_dacSliders[i]->setMinimum( 0 );
		_dacSliders[i]->setMaximum( (MPX3RX_DAC_TABLE[i].dflt * 2) - 1 );
		// Check all for scan
		_dacCheckBoxes[i]->setChecked(true);

		// Tooltip
		QString tooltip = "Integer value between 0 and ";
		tooltip += QString::number( (MPX3RX_DAC_TABLE[i].dflt * 2) - 1 );
		_dacSpinBoxes[i]->setToolTip( tooltip );
		_dacSliders[i]->setToolTip( tooltip );

		// Step init value
		_ui->scanStepSpinBox->setValue( _scanStep );

	}

}

void DACs::UncheckAllDACs() {

	for (int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {
		// Deselect all
		_dacCheckBoxes[i]->setChecked( false );
	}

}

void DACs::CheckAllDACs() {

	for (int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {
		// Check all for scan
		_dacCheckBoxes[i]->setChecked(true);
	}

}

void DACs::SetupSignalsAndSlots() {

	// Buttons
	connect( _ui->clearAllPushButton, SIGNAL(clicked()), this, SLOT( UncheckAllDACs() ) );
	connect( _ui->selectAllPushButton, SIGNAL(clicked()), this, SLOT( CheckAllDACs() ) );
	connect( _ui->startScanButton, SIGNAL(clicked()), this, SLOT( StartDACScan() ) );
	connect( _ui->senseDACsPushButton, SIGNAL(clicked()), this, SLOT( SenseDACs() ) );
	connect( _ui->deviceIdSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeDeviceIndex(int) ) );

	// Sliders and SpinBoxes

	// I need the SignalMapper in order to handle custom slots with parameters
	QSignalMapper * signalMapperSlider = new QSignalMapper (this);
	QSignalMapper * signalMapperSpinBox = new QSignalMapper (this);

	for ( int i = 0 ; i < MPX3RX_DAC_COUNT; i++ ) {

		// Connect slides to Spin Boxes back and forth
		// When the sliders move the SpinBoxes actualizes
		QObject::connect( _dacSliders[i], SIGNAL(sliderMoved(int)),
				_dacSpinBoxes[i], SLOT(setValue(int)) );

		// When the slider released, talk to the hardware
		QObject::connect( _dacSliders[i], SIGNAL(sliderReleased()),
				signalMapperSlider, SLOT(map()) );

		// When a value is changed in the SpinBox the slider needs to move
		//  and talk to the hardware
		QObject::connect( _dacSpinBoxes[i], SIGNAL(editingFinished()), // WARNING, this (int) is the value, not the index !
				signalMapperSpinBox, SLOT(map()) );

		// map the index
		signalMapperSlider->setMapping( _dacSliders[i], i );

		// map the index
		signalMapperSpinBox->setMapping( _dacSpinBoxes[i], i );

	}

	QObject::connect( signalMapperSlider, SIGNAL(mapped(int)), this, SLOT( FromSliderUpdateSpinBox(int) ) );

	QObject::connect( signalMapperSpinBox, SIGNAL(mapped(int)), this, SLOT( FromSpinBoxUpdateSlider(int) ) ); // SLOT( UpdateSliders(int) ) );

}

void DACs::FromSpinBoxUpdateSlider(int i) {

	// Set the value
	int val = _dacSpinBoxes[i]->value();
	// Set the slider according to the new value in the Spin Box
	_dacSliders[i]->setValue( val );
	// Set DAC
	_spidrcontrol->setDac( _deviceIndex, MPX3RX_DAC_TABLE[ i ].code, val );

}

void DACs::FromSliderUpdateSpinBox(int i) {

	//
	int val = _dacSliders[ i ]->value();
	// Set the spin box according to the new value in the Slider
	//_dacSpinBoxes[i]->setValue( val );
	// Set DAC
	_spidrcontrol->setDac( _deviceIndex, MPX3RX_DAC_TABLE[ i ].code, val );

}

//void DACs::SetDAC(QObject * info) {
//	// Verify first if the value changed at all
//	int val = _dacSpinBoxes[ ((SignalSlotMapping *)info)->index ]->value();
//	_spidrcontrol->setDac( _deviceIndex, MPX3RX_DAC_TABLE[ ((SignalSlotMapping *)info)->index ].code, val );
//}


bool DACs::ReadDACsFile(string fn) {

	if(fn.empty()) { // default
		fn = __default_DACs_filename;
	}

	filebuf fb;
	char rc;
	string temp;
	bool clearToAppend = true;
	bool waitForNextLine = false;
	int dacValIndex = 0;

	if ( fb.open (fn.c_str(), ios::in) ) {

		cout << "[INFO] reading DACs file from " << fn << endl;

		istream is(&fb);

		while(is) {

			rc = char(is.get());

			if ( waitForNextLine && ( rc != 0xa && rc != 0xd ) ) { // Already in the next line
				waitForNextLine = false;
				clearToAppend = true;
				// previous line finished
				//cout << temp << endl;
				// Append the values to the dac vals internal array
				_dacVals[dacValIndex] = atoi( temp.c_str() );
				dacValIndex++;
				if ( dacValIndex > MPX3RX_DAC_COUNT ) { // off scale
					string messg = "ERROR loading the defaults DAC file: ";
					messg += fn;
					QMessageBox::information(_ui->_DACScanFrame, tr("MPX3 - default DACs"), tr( messg.c_str() ) );
					return false;
				}
				temp.clear();
			}

			if ( rc == '/' || rc == '#' ) { // A comment from here and on
				clearToAppend = false;
			}
			if (clearToAppend && !waitForNextLine) {
				temp.append(1, rc); // append the character
			}
			if ( rc == 0xa || rc == 0xd ) { // This is the end of a line
				clearToAppend = false;
				waitForNextLine = true;
			}

		}
		fb.close();

	} else {

		cout << "[WARNING] Couldn't find " << fn << endl;
		cout << "[WARNING] Setting default DACs at mid range ! These might not be the best settings." << endl;
		string messg = "Couldn't find: ";
		messg += fn;
		messg += "\nSetting default DACs at mid range ! These might not be the best settings.";
		QMessageBox::warning ( _ui->_DACScanFrame, tr("MPX3 - default DACs"), tr( messg.c_str() ) );

		return false;
	}


	return true;
}

void DACs::ChangeDeviceIndex( int index )
{
  if( index < 0 ) return; // can't really happen cause the SpinBox has been limited
  _deviceIndex = index;
}


