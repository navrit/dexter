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

#include "SpidrController.h"
#include "SpidrDaq.h"

#include "qcustomplot.h"

DACs::DACs(){

}

DACs::DACs(QApplication * coreApp, Ui::Mpx3GUI * ui) {
	ReadDACsFile(""); //read the default file
	_coreApp = coreApp;
	_spidrcontrol = 0;  // Assuming no connection yet
	_spidrdaq = 0;		// Assuming no connection yet
	_ui = ui;
	_senseThread = 0x0;
	_scanThread = 0x0;
	_signalMapperSliderSpinBoxConn = 0x0;
	_signalMapperSlider = 0x0;
	_signalMapperSpinBox = 0x0;

	// Number of plots added to the Scan
	_plotIdxCntr = 0;

	// Defaults
	_scanStep = 16;
	_ui->scanStepSpinBox->setMaximum( 256 );
	_ui->scanStepSpinBox->setMinimum( 1 );
	_ui->scanStepSpinBox->setValue( _scanStep );
	_deviceIndex = 2;
	_ui->deviceIdSpinBox->setMaximum(3);
	_ui->deviceIdSpinBox->setMinimum(0);
	_ui->deviceIdSpinBox->setValue( _deviceIndex );
	_nSamples = 1;
	_ui->samplesSpinBox->setMaximum(100);
	_ui->samplesSpinBox->setMinimum(0);
	_ui->samplesSpinBox->setValue( _nSamples );

	_ui->progressBar->setValue( 0 );

	// Order widgets in vectors
	FillWidgetVectors();

	// Set limits in widgets
	SetLimits();

	// Setup connection between Sliders and SpinBoxes
	SetupSignalsAndSlots();

	// Leave a few things unnactivated
	_ui->startScanButton->setDisabled( true );
	_ui->senseDACsPushButton->setDisabled( true );

	// Prepare plot
	// Prepare the plot
	_dacScanPlot = new QCustomPlot();
	_dacScanPlot->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );
	// The legend
	_dacScanPlot->legend->setVisible( false ); // Nothing there yet...
	QFont f = _ui->_DACScanFrame->font();
	f.setPointSize( 7 ); // and make a bit smaller for legend
	_dacScanPlot->legend->setFont( f );
	_dacScanPlot->legend->setBrush( QBrush(QColor(255,255,255,230)) );
	// The axes
	_dacScanPlot->xAxis->setRange( 0, __max_DAC_range ); // Maximum DAC range
	_dacScanPlot->yAxis->setRange( 0, __voltage_DACS_MAX );
	f = _ui->_DACScanFrame->font();  // Start out with Dialog's font..
	f.setBold( true );
	_dacScanPlot->xAxis->setLabelFont( f );
	_dacScanPlot->yAxis->setLabelFont( f );
	// The labels:
	_dacScanPlot->xAxis->setLabel("DAC setting");
	_dacScanPlot->yAxis->setLabel("DAC out [V]");

	// Insert the plot in the dialog window
	_dacScanPlot->setParent( _ui->_DACScanFrame );

	QRect hrect = ui->_DACScanFrame->geometry();
	_dacScanPlot->resize( hrect.size().rwidth() , hrect.size().rheight() );

	//ReadDACsFile("asda"); //TODO: shouldn't exist, doesn't throw an error.
	//PopulateDACValues();

}

DACs::~DACs() {

}

void DACs::StartDACScan() {

	if ( !_spidrcontrol ) {
		QMessageBox::information(_ui->_DACScanFrame, tr("MPX3"), tr("Connect to hardware first.") );
		return;
	}

	// Replot to start
	_dacScanPlot->clearGraphs();
	_dacScanPlot->legend->setVisible( false );
	_dacScanPlot->replot();

	// Threads
	if ( _scanThread ) {
		if ( _scanThread->isRunning() ) {
			return;
		}
		//disconnect(_senseThread, SIGNAL( progress(int) ), _ui->progressBar, SLOT( setValue(int)) );
		delete _scanThread;
		_scanThread = 0x0;
	}

	// Create the thread
	_scanThread = new ScanDACsThread(this, _spidrcontrol);
	// Connect to the progress bar
	connect( _scanThread, SIGNAL( progress(int) ), _ui->progressBar, SLOT( setValue(int)) );

	_scanThread->start();

}

void DACs::ConnectToHardware(SpidrController * sc, SpidrDaq * sd) {

	_spidrcontrol = sc;
	_spidrdaq = sd;

	// Connected, activate what was not ready to operate
	_ui->startScanButton->setDisabled( false );
	_ui->senseDACsPushButton->setDisabled( false );

}

void DACs::PopulateDACValues() {

	// Here we set the default values hardcoded in MPX3RX_DAC_TABLE (mid range)
	//   OR if the DACs file is present we read the values from it.  The file has
	//   higher priority.
	string defaultDACsFn = __default_DACs_filename;

	if ( ReadDACsFile(defaultDACsFn) ) {

		cout << "[INFO] setting dacs from defult DACs file." << endl;

		for(int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {
			_dacVals[i] = configJson[MPX3RX_DAC_TABLE[i].name].toInt();
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

	if ( !_spidrcontrol ) {
		QMessageBox::information(_ui->_DACScanFrame, tr("MPX3"), tr("Connect to hardware first.") );
		return;
	}

	// Threads
	if ( _senseThread ) {
		if ( _senseThread->isRunning() ) {
			return;
		}
		//disconnect(_senseThread, SIGNAL( progress(int) ), _ui->progressBar, SLOT( setValue(int)) );
		delete _senseThread;
		_senseThread = 0x0;
	}

	// Create the thread
	_senseThread = new SenseDACsThread(this, _spidrcontrol);
	// Connect to the progress bar
	connect( _senseThread, SIGNAL( progress(int) ), _ui->progressBar, SLOT( setValue(int)) );

	_senseThread->start();

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

	// DAC labels
	_dacLabels[0] = _ui->dac0Label;
	_dacLabels[1] = _ui->dac1Label;
	_dacLabels[2] = _ui->dac2Label;
	_dacLabels[3] = _ui->dac3Label;
	_dacLabels[4] = _ui->dac4Label;
	_dacLabels[5] = _ui->dac5Label;
	_dacLabels[6] = _ui->dac6Label;
	_dacLabels[7] = _ui->dac7Label;
	_dacLabels[8] = _ui->dac8Label;
	_dacLabels[9] = _ui->dac9Label;
	_dacLabels[10] = _ui->dac10Label;
	_dacLabels[11] = _ui->dac11Label;
	_dacLabels[12] = _ui->dac12Label;
	_dacLabels[13] = _ui->dac13Label;
	_dacLabels[14] = _ui->dac14Label;
	_dacLabels[15] = _ui->dac15Label;
	_dacLabels[16] = _ui->dac16Label;
	_dacLabels[17] = _ui->dac17Label;
	_dacLabels[18] = _ui->dac18Label;
	_dacLabels[19] = _ui->dac19Label;
	_dacLabels[20] = _ui->dac20Label;
	_dacLabels[21] = _ui->dac21Label;
	_dacLabels[22] = _ui->dac22Label;
	_dacLabels[23] = _ui->dac23Label;
	_dacLabels[24] = _ui->dac24Label;
	_dacLabels[25] = _ui->dac25Label;
	_dacLabels[26] = _ui->dac26Label;

	// DAC sense labels. Where voltage is shown.
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

	// Tooltips
	for (int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {

		QString tooltip = "Integer value between 0 and ";
		tooltip += QString::number( (MPX3RX_DAC_TABLE[i].dflt * 2) - 1 );
		_dacSpinBoxes[i]->setToolTip( tooltip );
		_dacSliders[i]->setToolTip( tooltip );

		//
		QString tooltipLabel = "[";
		tooltipLabel += MPX3RX_DAC_TABLE[i].name;
		tooltipLabel += "] index:";
		tooltipLabel += QString::number( i );
		tooltipLabel += ",code:";
		tooltipLabel += QString::number( MPX3RX_DAC_TABLE[i].code );
		_dacLabels[i]->setToolTip( tooltipLabel );
		_dacCheckBoxes[i]->setToolTip( tooltipLabel );

		QString tooltipLabelV = "16bits AD conversion (V)";
		_dacVLabels[i]->setToolTip( tooltipLabelV );

	}

	// Text in the DAC labels
	for (int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {
		QString DACname = MPX3RX_DAC_TABLE[i].name;
		_dacLabels[i]->setText( DACname );
	}

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

	}

	// Leave by default the THL2 o THL7 uncheked by default
	for (int i = 2 ; i <= 7 ; i++ ) {
		_dacCheckBoxes[i]->setChecked( false );
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
	connect( _ui->samplesSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeNSamples(int) ) );
	connect( _ui->scanStepSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeScanStep(int) ) );

	// Sliders and SpinBoxes

	// I need the SignalMapper in order to handle custom slots with parameters
	if(!_signalMapperSliderSpinBoxConn) _signalMapperSliderSpinBoxConn = new QSignalMapper (this);
	if(!_signalMapperSlider) _signalMapperSlider = new QSignalMapper (this);
	if(!_signalMapperSpinBox) _signalMapperSpinBox = new QSignalMapper (this);

	for ( int i = 0 ; i < MPX3RX_DAC_COUNT; i++ ) {

		// Connect slides to Spin Boxes back and forth
		// When the sliders move the SpinBoxes actualizes
		//QObject::connect( _dacSliders[i], SIGNAL(sliderMoved(int)),
		//_dacSpinBoxes[i], SLOT(setValueDAC(int)) );

		QObject::connect( _dacSliders[i], SIGNAL(sliderMoved(int)),
				_signalMapperSliderSpinBoxConn, SLOT(map()) );

		// When the slider released, talk to the hardware
		QObject::connect( _dacSliders[i], SIGNAL(sliderReleased()),
				_signalMapperSlider, SLOT(map()) );

		// When a value is changed in the SpinBox the slider needs to move
		//  and talk to the hardware
		QObject::connect( _dacSpinBoxes[i], SIGNAL(valueChanged(int)), // SIGNAL(valueChanged(int)), // SIGNAL(editingFinished()),
				_signalMapperSpinBox, SLOT(map()) );

		// map the index
		_signalMapperSliderSpinBoxConn->setMapping( _dacSliders[i], i ); // SLOT(setValueDAC(int))

		// map the index
		_signalMapperSlider->setMapping( _dacSliders[i], i );

		// map the index
		_signalMapperSpinBox->setMapping( _dacSpinBoxes[i], i );

	}

	QObject::connect( _signalMapperSliderSpinBoxConn,  SIGNAL(mapped(int)), this, SLOT(setValueDAC(int)) );

	QObject::connect( _signalMapperSlider, SIGNAL(mapped(int)), this, SLOT( FromSliderUpdateSpinBox(int) ) );

	QObject::connect( _signalMapperSpinBox, SIGNAL(mapped(int)), this, SLOT( FromSpinBoxUpdateSlider(int) ) ); // SLOT( UpdateSliders(int) ) );

}

/**
 * While the slider moves I want to see the spinBox running but
 *  only want to talk to the hardware when it is released.
 */
void DACs::setValueDAC(int i) {

	// Temporarily disconnect the signal that triggers the message to the hardware
	QObject::disconnect( _signalMapperSpinBox, SIGNAL(mapped(int)), this, SLOT( FromSpinBoxUpdateSlider(int) ) );

	// Set the value in the spinBox so the user sees it running
	int val = _dacSliders[ i ]->value();
	// Change the value in the spinBox
	_dacSpinBoxes[i]->setValue( val );

	// Connect it back
	QObject::connect( _signalMapperSpinBox, SIGNAL(mapped(int)), this, SLOT( FromSpinBoxUpdateSlider(int) ) );

}

void DACs::FromSpinBoxUpdateSlider(int i) {

	// Set the value
	int val = _dacSpinBoxes[i]->value();
	// Set the slider according to the new value in the Spin Box
	_dacSliders[i]->setValue( val );
	// Set DAC
	_spidrcontrol->setDac( _deviceIndex, MPX3RX_DAC_TABLE[ i ].code, val );
	// Clean up the corresponding labelV.  The dacOut won't be read right away.
	// Only under user request
	_dacVals[i] = val;
	GetLabelsList()[i]->setText("");

}

void DACs::FromSliderUpdateSpinBox(int i) {

	//
	int val = _dacSliders[ i ]->value();
	// Set the spin box according to the new value in the Slider
	//_dacSpinBoxes[i]->setValue( val );
	// Set DAC
	_spidrcontrol->setDac( _deviceIndex, MPX3RX_DAC_TABLE[ i ].code, val );
	_dacVals[i] = val;
	// Clean up the corresponding labelV.  The dacOut won't be read right away.
	// Only under user request
	GetLabelsList()[i]->setText("");

}

//void DACs::SetDAC(QObject * info) {
//	// Verify first if the value changed at all
//	int val = _dacSpinBoxes[ ((SignalSlotMapping *)info)->index ]->value();
//	_spidrcontrol->setDac( _deviceIndex, MPX3RX_DAC_TABLE[ ((SignalSlotMapping *)info)->index ].code, val );
//}


bool DACs::ReadDACsFile(string fn) {//TODO: should use QString instead of std::string, for one: it handles unicode better and interfaces with the rest of Qt better.
	if(fn.empty()) { // default
		fn = __default_DACs_filename;
	}
	QFile saveFile(QString(fn.c_str()));
	if (!saveFile.open(QIODevice::ReadOnly)) {
	    cout << "[WARNING] Couldn't find " << fn << endl;
	    cout << "[WARNING] Setting default DACs at hardcoded values ! These might not be the best settings." << endl;
	    string messg = "Couldn't open: ";
	    messg += fn;
	    messg += "\nSetting default DACs at hardcoded values ! These might not be the best settings.";
	    QMessageBox::warning ( _ui->_DACScanFrame, tr("MPX3 - default DACs"), tr( messg.c_str() ) );
	    return false;
	 }

	QJsonDocument jsDoc(QJsonDocument::fromJson(saveFile.readAll()));
	configJson = jsDoc.object();
	getConfig();
	return true;
}

bool DACs::WriteDACsFile(string fn){
  setConfig();
  if(fn.empty()) { // default
          fn = __default_DACs_filename;
  }
  QFile saveFile(QString(fn.c_str()));
  if (!saveFile.open(QIODevice::WriteOnly)) {
      string messg = "Couldn't open: ";
      messg += fn;
      messg += "\nNo output written!";
      QMessageBox::warning ( _ui->_DACScanFrame, tr("MPX3 - default DACs"), tr( messg.c_str() ) );
      return false;
   }
  QFileInfo fInfo(saveFile.fileName());
  std::cout << "Opened " <<fInfo.absoluteFilePath().toStdString() << std::endl;
  QJsonDocument jsDoc(configJson);
  if(-1 == saveFile.write(jsDoc.toJson())){
      std::cout << "Write error!";
      return false;
  }
  return true;
}

void DACs::ChangeDeviceIndex( int index )
{
	if( index < 0 ) return; // can't really happen cause the SpinBox has been limited
	_deviceIndex = index;
}

void DACs::ChangeNSamples( int index )
{
	if( index < 0 ) return; // can't really happen cause the SpinBox has been limited
	_nSamples = index;
}

void DACs::ChangeScanStep( int index )
{
	if( index < 0 ) return; // can't really happen cause the SpinBox has been limited
	_scanStep = index;
}

QCPGraph * DACs::GetGraph(int idx) {
	return _dacScanPlot->graph( idx );
}

void DACs::scanFinished() {

	_plotIdxCntr = 0;
	_plotIdxMap.clear();

	// clear plots
	//_dacScanPlot->clearGraphs();
	//_dacScanPlot->legend->setVisible( false );

}

void DACs::addData(int dacIdx, int dacVal, double adcVal ) {

	_graph = _dacScanPlot->graph( _plotIdxMap[dacIdx] ); // the right vector index
	_graph->addData( dacVal, adcVal );
	_dacScanPlot->replot();

	// Actualize Slider, SpinBoxes, and Labels
	//_dacSpinBoxes[dacIdx]->setValue( dacVal );
	//_dacSliders[dacIdx]->setValue( dacVal );
	// Labels are updated already through a SIGNAL/SLOT from the thread.

}

void DACs::setTextWithIdx(QString s, int i) {

	// Get the color from the Color table
	// Create the style sheet string
	QPalette palette = GetLabelsList()[i]->palette();
	//palette.setColor(GetLabelsList()[i]->backgroundRole(), COLOR_TABLE[i]);
	palette.setColor(GetLabelsList()[i]->foregroundRole(), COLOR_TABLE[i]);

	GetLabelsList()[i]->setPalette(palette);

	// The StyleSheet is the crossplatform solution but I am not using it here
	//GetLabelsList()[i]->setStyleSheet("QLabel { background-color : Qt::red; color : Qt::black; }");

	// Finally set the text
	GetLabelsList()[i]->setText(s);

}

void DACs::slideAndSpin(int i, int val) {

	// // Temporarily disconnect the signal that triggers the message to the hardware
	QObject::disconnect( _signalMapperSpinBox, SIGNAL(mapped(int)), this, SLOT( FromSpinBoxUpdateSlider(int) ) );

	// Slide n' Spin
	GetSpinBoxList()[i]->setValue( val );
	GetSliderList()[i]->setValue( val );

	// Connect it back
	QObject::connect( _signalMapperSpinBox, SIGNAL(mapped(int)), this, SLOT( FromSpinBoxUpdateSlider(int) ) );

}

void DACs::addData(int dacIdx) {

	_plotIdxMap[dacIdx] = _plotIdxCntr;
	_plotIdxCntr++;

	// If starting a plot, create it first
	_dacScanPlot->addGraph();
	_graph = _dacScanPlot->graph( _plotIdxMap[dacIdx] ); // the right vector index

	QPen pen( COLOR_TABLE[ dacIdx ] );
	pen.setWidth( 0.1 );
	_graph->setPen( pen );
	_graph->setName( QString(MPX3RX_DAC_TABLE[dacIdx].name) ); // the dac index

	// Do not use legend.  I will color the text boxes instead.
	//_graph->addToLegend();
	//_dacScanPlot->legend->setVisible( true );

}


void SenseDACsThread::run() {

	// Open a new temporary connection to the spider to avoid collisions to the main one
	SpidrController * spidrcontrol = new SpidrController( 192, 168, 1, 10 );

	if ( !spidrcontrol || !spidrcontrol->isConnected() ) {
		cout << "Device not connected !" << endl;
		return;
	}

	// Make it update the Tab so the drawing is smooth
	connect( this, SIGNAL( fillText(QString) ), _dacs->GetUI()->tabDACs, SLOT( update() ) );

	// Clean up Labels first
	for (int i = 0 ; i < MPX3RX_DAC_COUNT ; i++) {

		connect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );
		fillText("");
		disconnect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );

	}

	int adc_val = 0;

	progress( 0 );

	for (int i = 0 ; i < MPX3RX_DAC_COUNT ; i++) {

		if ( ! _dacs->GetCheckBoxList()[i]->isChecked() ) continue;

		if ( !spidrcontrol->setSenseDac( _dacs->GetDeviceIndex(), MPX3RX_DAC_TABLE[i].code ) ) {

			cout << "setSenseDac[" << i << "] | " << spidrcontrol->errorString() << endl;

		} else {

			adc_val = 0;

			if ( !spidrcontrol->getDacOut( _dacs->GetDeviceIndex(), &adc_val, _dacs->GetNSamples() ) ) {

				cout << "getDacOut : " << i << " | " << spidrcontrol->errorString() << endl;

			} else {

				adc_val /= _dacs->GetNSamples();
				QString dacOut;
				if ( adc_val > __maxADCCounts || adc_val < 0 ) { // FIXME .. handle the clipping properly
					dacOut = "clip'ng";
				} else {
					dacOut = QString::number( (__voltage_DACS_MAX/(double)__maxADCCounts) * (double)adc_val, 'f', 2 );
					dacOut += "V";
				}

				// Send signal to Labels.  Making connections one by one.
				connect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );
				fillText( dacOut );
				disconnect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );

				// Send signal to progress bar
				progress( floor( ( (double)i / MPX3RX_DAC_COUNT) * 100 ) );
				//cout << i << " --> " << adc_val << endl;


			}

		}

	}

	progress( 100 );

	disconnect( this, SIGNAL( fillText(QString) ), _dacs->GetUI()->tabDACs, SLOT( update() ) );

	// Disconnect the progress bar
	//disconnect( this, SIGNAL( progress(int) ), _ui->progressBar, SLOT( setValue(int)) );


	delete spidrcontrol;
}

void ScanDACsThread::run() {

	// Open a new temporary connection to the spider to avoid collisions to the main one
	SpidrController * spidrcontrol = new SpidrController( 192, 168, 1, 10 );

	if ( !spidrcontrol || !spidrcontrol->isConnected() ) {
		cout << "Device not connected !" << endl;
		return;
	}

	// Store starting values in order to replace them later
	vector<int> currentDACs;
	int dac_val = 0;
	for (int i = 0 ; i < MPX3RX_DAC_COUNT ; i++) {
		spidrcontrol->getDac(  _dacs->GetDeviceIndex(), MPX3RX_DAC_TABLE[i].code, &dac_val);
		currentDACs.push_back( dac_val );
	}

	// Make it update the Tab so the drawing is smooth
	connect( this, SIGNAL( fillText(QString) ), _dacs->GetUI()->tabDACs, SLOT( update() ) );

	// Connect the plots on DACs
	connect( this, SIGNAL( addData(int, int, double) ), _dacs, SLOT( addData(int, int, double) ) );
	// This SLOT will be taking care of creating the new plot
	connect( this, SIGNAL( addData(int) ), _dacs, SLOT( addData(int) ) );
	connect( this, SIGNAL( scanFinished() ), _dacs, SLOT( scanFinished() ) );

	// For the progress bar verify how many DACs are scheduled for scanning
	// and calculate the number of steps.
	int nStepsToScan = 0;
	for (int i = 0 ; i < MPX3RX_DAC_COUNT ; i++) {
		if ( _dacs->GetCheckBoxList()[i]->isChecked() ) {
			nStepsToScan += (MPX3RX_DAC_TABLE[i].dflt * 2) / _dacs->GetScanStep();
		}
	}

	int adc_val = 0;
	progress( 0 );
	int progressBarCntr = 0;

	for (int i = 0 ; i < MPX3RX_DAC_COUNT ; i++) {

		if ( ! _dacs->GetCheckBoxList()[i]->isChecked() ) continue;

		// Create the plot coming
		addData(i);

		// Clean up the corresponding label
		connect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );
		fillText("");
		disconnect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );

		// Scan !
		for ( int dacValI = 0 ; dacValI < MPX3RX_DAC_TABLE[i].dflt * 2 - 1 ; dacValI += _dacs->GetScanStep() ) {


			if( !spidrcontrol->setDac( _dacs->GetDeviceIndex(), MPX3RX_DAC_TABLE[i].code, dacValI ) ) {

				cout << "setDac[" << i << "] | " << spidrcontrol->errorString() << endl;


			} else {

				// Adjust the sliders and the SpinBoxes to the new value
				connect( this, SIGNAL( slideAndSpin(int, int) ), _dacs, SLOT( slideAndSpin(int, int) ) );
				slideAndSpin( i, dacValI );
				disconnect( this, SIGNAL( slideAndSpin(int, int) ), _dacs, SLOT( slideAndSpin(int, int) ) );

				if ( !spidrcontrol->setSenseDac( _dacs->GetDeviceIndex(), MPX3RX_DAC_TABLE[i].code ) ) {

					cout << "setSenseDac[" << i << "] | " << spidrcontrol->errorString() << endl;

				} else {

					adc_val = 0;

					if ( !spidrcontrol->getDacOut( _dacs->GetDeviceIndex(), &adc_val, _dacs->GetNSamples() ) ) {

						cout << "getDacOut : " << i << " | " << spidrcontrol->errorString() << endl;

					} else {

						// Here we completed the chain: setDac --> setSenseDac --> getDacOut

						adc_val /= _dacs->GetNSamples();
						QString dacOut;
						double adc_volt = 0.;
						if ( adc_val > __maxADCCounts || adc_val < 0 ) { // FIXME .. handle the clipping properly
							dacOut = "clip'ng";
						} else {

							adc_volt = (__voltage_DACS_MAX/(double)__maxADCCounts) * (double)adc_val;
							dacOut = QString::number( adc_volt, 'f', 2 );
							dacOut += "V";
							addData(i, dacValI, adc_volt);

						}

						// Send signal to Labels.  Making connections one by one.
						connect( this, SIGNAL( fillTextWithIdx(QString, int) ), _dacs, SLOT(setTextWithIdx(QString,int)) );
						fillTextWithIdx( dacOut, i );
						disconnect( this, SIGNAL( fillTextWithIdx(QString, int) ), _dacs, SLOT(setTextWithIdx(QString,int) ) );
						//connect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );
						//fillText( dacOut );
						//disconnect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );

						// Send signal to progress bar
						progress( floor( ( (double)progressBarCntr / (double)nStepsToScan) * 100 ) );
						progressBarCntr++;


					}

				}

			}

		}

		//////////////////////////////////////////////////////////////////////////////
		// And bring back the DAC to it's original value
		int retryCntr = __N_RETRY_ORIGINAL_SETTING;

		if( !spidrcontrol->setDac( _dacs->GetDeviceIndex(), MPX3RX_DAC_TABLE[i].code, currentDACs[i] ) ) {

			if( retryCntr == 0 ) {
				retryCntr = __N_RETRY_ORIGINAL_SETTING;
				cout << "setDac[" << i << "] | " << spidrcontrol->errorString() << " ... tried 3 times, giving up." << endl;
				continue;
			}
			cout << "setDac[" << i << "] | " << spidrcontrol->errorString() << " ... retry " << __N_RETRY_ORIGINAL_SETTING - retryCntr + 1 << endl;
			retryCntr--;

		} else {

			// setDac successful
			// Bring the slider and spinBox to the right position
			// Adjust the sliders and the SpinBoxes to the new value
			connect( this, SIGNAL( slideAndSpin(int, int) ), _dacs, SLOT( slideAndSpin(int, int) ) );
			slideAndSpin( i, currentDACs[i] );
			disconnect( this, SIGNAL( slideAndSpin(int, int) ), _dacs, SLOT( slideAndSpin(int, int) ) );

			//////////////////////////////////////////////////////////////////////////////
			// And sense it in the original position
			if ( !spidrcontrol->setSenseDac( _dacs->GetDeviceIndex(), MPX3RX_DAC_TABLE[i].code ) ) {

				cout << "setSenseDac[" << i << "] | " << spidrcontrol->errorString() << endl;

			} else {

				adc_val = 0;

				if ( !spidrcontrol->getDacOut( _dacs->GetDeviceIndex(), &adc_val, _dacs->GetNSamples() ) ) {

					cout << "getDacOut : " << i << " | " << spidrcontrol->errorString() << endl;

				} else {

					adc_val /= _dacs->GetNSamples();
					QString dacOut;
					if ( adc_val > __maxADCCounts || adc_val < 0 ) { // FIXME .. handle the clipping properly
						dacOut = "clip'ng";
					} else {
						dacOut = QString::number( (__voltage_DACS_MAX/(double)__maxADCCounts) * (double)adc_val, 'f', 2 );
						dacOut += "V";
					}

					// Send signal to Labels.  Making connections one by one.
					connect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );
					fillText( dacOut );
					disconnect( this, SIGNAL( fillText(QString) ), _dacs->GetLabelsList()[i], SLOT( setText(QString)) );

				}

			} //////////////////////////////////////////////////////////////////////////////

		} //////////////////////////////////////////////////////////////////////////////

	}

	progress( 100 );

	scanFinished();

	disconnect( this, SIGNAL( fillText(QString) ), _dacs->GetUI()->tabDACs, SLOT( update() ) );
	disconnect( this, SIGNAL( addData(int, int, double) ), _dacs, SLOT( addData(int, int, double) ) );
	disconnect( this, SIGNAL( addData(int) ), _dacs, SLOT( addData(int) ) );
	disconnect( this, SIGNAL( scanFinished() ), _dacs, SLOT( scanFinished() ) );

	// Disconnect the progress bar
	//disconnect( this, SIGNAL( progress(int) ), _ui->progressBar, SLOT( setValue(int)) );


	delete spidrcontrol;
}

void DACs::setConfig(){
  for(int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {
          configJson[MPX3RX_DAC_TABLE[i].name] = _dacVals[i];
    }
  /*for(int i = 0; i < Thresholds.length();i++){
      configJson[QString("Threshold%1").arg(i)] = Thresholds[i];
   }
  configJson["I_Preamp"] = I_Preamp;
  configJson["I_Ikrum"] = I_Ikrum;
  configJson["I_Shaper"] = I_Shaper;
  configJson["I_Disc"] = I_Disc;
  configJson["I_Disc_LS"] = I_Disc_LS;
  configJson["I_Shaper_test"] = I_Shaper_test;
  configJson["I_DAC_DiscL"] = I_DAC_DiscL;
  configJson["I_DAC_test"] = I_DAC_test;
  configJson["I_DAC_DiscH"] = I_DAC_DiscH;
  configJson["I_Delay"] = I_Delay;
  configJson["I_TP_BufferIn"] = I_TP_BufferIn;
  configJson["I_TP_BufferOut"] = I_TP_BufferOut;
  configJson["V_Rpz"] = V_Rpz;
  configJson["V_Gnd"] = V_Gnd;
  configJson["V_Tp_ref"] = V_Tp_ref;
  configJson["V_Fbk"] = V_Fbk;
  configJson["V_Cas"] = V_Cas;
  configJson["V_Tp_refA"] = V_Tp_refA;
  configJson["V_Tp_refB"] = V_Tp_refB;*/
}

void DACs::getConfig(){
  for(int i = 0 ; i < MPX3RX_DAC_COUNT; i++){
    _dacVals[i] = configJson[MPX3RX_DAC_TABLE[i].name].toInt();
    }
  /*for(int i = 0; i < Thresholds.length();i++){
      Thresholds[i] = configJson[QString("Threshold%1").arg(i)].toInt();
   }
  I_Preamp = configJson["I_Preamp"].toInt();
  I_Ikrum = configJson["I_Ikrum"].toInt();
  I_Shaper = configJson["I_Shaper"].toInt();
  I_Disc = configJson["I_Disc"].toInt();
  I_Disc_LS = configJson["I_Disc_LS"].toInt();
  I_Shaper_test = configJson["I_Shaper_test"].toInt();
  I_DAC_DiscL = configJson["I_DAC_DiscL"].toInt();
  I_DAC_test =configJson["I_DAC_test"].toInt();;
  I_DAC_DiscH = configJson["I_DAC_DiscH"].toInt();
  I_Delay = configJson["I_Delay"].toInt();
  I_TP_BufferIn = configJson["I_TP_BufferIn"].toInt();
  I_TP_BufferOut = configJson["I_TP_BufferOut"].toInt();
  V_Rpz= configJson["V_Rpz"].toInt();
  V_Gnd = configJson["V_Gnd"].toInt();
  V_Tp_ref = configJson["V_Tp_ref"].toInt();
  V_Fbk = configJson["V_Fbk"].toInt();
  V_Cas = configJson["V_Cas"].toInt();
  V_Tp_refA = configJson["V_Tp_refA"].toInt();
  V_Tp_refB = configJson["V_Tp_refB"].toInt();*/
}

void DACs::openWriteMenu(){
  std::cout << "Openwritemenu called!" << std::endl;
  QString fileName = QFileDialog::getSaveFileName(this->_ui->widget, tr("Save Config"), tr("."), tr("json Files (*.json)"));
  WriteDACsFile(fileName.toStdString());
  //this->WriteDACsFile()
}
