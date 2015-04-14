#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"
#include "mpx3equalization.h"
#include "SpidrController.h"
#include "SpidrDaq.h"

#include <stdio.h>
QCstmGLVisualization::QCstmGLVisualization(QWidget *parent) :  QWidget(parent),  ui(new Ui::QCstmGLVisualization)
{
  ui->setupUi(this);
}

QCstmGLVisualization::~QCstmGLVisualization()
{
  delete ui;
}

void QCstmGLVisualization::StartDataTaking(){
	Configuration( false );

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	cout << "Acquiring ... ";

	// Start the trigger as configured
	spidrcontrol->startAutoTrigger();//getNTriggers
	Sleep( 50 );
	// See if there is a frame available
	// I should get as many frames as triggers
	int * framedata;
	while ( spidrdaq->hasFrame() ) {
		//cout << "capture ..." << endl;
		int size_in_bytes = -1;
		framedata = spidrdaq->frameData(0, &size_in_bytes);

		_mpx3gui->addFrame( framedata );

		spidrdaq->releaseFrame();
		Sleep( 10 ); // Allow time to get and decode the next frame, if any
	}

	cout << "done." << endl;

}
void QCstmGLVisualization::ConnectionStatusChanged() {

	ui->startButton->setEnabled(true); //Enable or disable the button depending on the connection status.

	// TODO
	// Configure the chip, provided that the Adj mask is loaded
	//Configuration( false );

}

void QCstmGLVisualization::Configuration(bool reset) {//TODO: should be part of parent?

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	int deviceIndex = 1;
	int nTriggers = 100;

	// Reset pixel configuration
	if ( reset ) spidrcontrol->resetPixelConfig();

	// All adjustment bits to zero
	//SetAllAdjustmentBits(0x0, 0x0);

	// OMR
	//spidrcontrol->setPolarity( true );		// Holes collection
	//_spidrcontrol->setDiscCsmSpm( 0 );		// DiscL used
	//_spidrcontrol->setInternalTestPulse( true ); // Internal tests pulse

	spidrcontrol->setColourMode( deviceIndex, false ); 	// Fine Pitch
	spidrcontrol->setCsmSpm( deviceIndex, 0 );			// Single Pixel mode
	//spidrcontrol->setEqThreshH( deviceIndex, true );
	spidrcontrol->setDiscCsmSpm( deviceIndex, 0 );		// In Eq mode using 0: Selects DiscL, 1: Selects DiscH
	//_spidrcontrol->setGainMode( 1 );

	// Gain ?!
	// 00: SHGM  0
	// 10: HGM   2
	// 01: LGM   1
	// 11: SLGM  3
	spidrcontrol->setGainMode( deviceIndex, 2 );

	// Other OMR
	spidrdaq->setDecodeFrames( true );
	spidrcontrol->setPixelDepth( deviceIndex, 12 );
	spidrdaq->setPixelDepth( 12 );
	spidrcontrol->setMaxPacketSize( 1024 );

	// Write OMR ... i shouldn't call this here
	//_spidrcontrol->writeOmr( 0 );

	// Trigger config
	int trig_mode      = 4;     // Auto-trigger mode = 4
	int trig_length_us = 500;  // This time shouldn't be longer than the period defined by trig_freq_hz
	int trig_freq_hz   = (int) ( 1. / (2.*((double)trig_length_us/1000000.)) );   // Make the period double the trig_len
	cout << "[INFO] Configured freq is " << trig_freq_hz << "Hz" << endl;
	int nr_of_triggers = nTriggers;    // This is the number of shutter open i get
	//int trig_pulse_count;
	spidrcontrol->setShutterTriggerConfig( trig_mode, trig_length_us,
			trig_freq_hz, nr_of_triggers );

}

void QCstmGLVisualization::setGradient(int index){
  ui->gradientDisplay->setGradient(_mpx3gui->getGradient(index));
  ui->glPlot->setGradient(_mpx3gui->getGradient(index));
}

void QCstmGLVisualization::SetMpx3GUI(Mpx3GUI *p){
  _mpx3gui = p;
  setGradient(0);
  connect(_mpx3gui, SIGNAL(ConnectionStatusChanged(bool)), ui->startButton, SLOT(setEnabled(bool))); //enable the button on connection
  connect(ui->startButton, SIGNAL(clicked(bool)), this, SLOT(StartDataTaking()));
  connect(ui->summingCheckbox, SIGNAL(clicked(bool)), _mpx3gui, SLOT(set_summing(bool)));
  connect(_mpx3gui, SIGNAL(summing_set(bool)), ui->summingCheckbox, SLOT(setChecked(bool)));
  connect(ui->gradientSelector, SIGNAL(activated(int)), this, SLOT(setGradient(int)));
  connect(ui->generateDataButton, SIGNAL(clicked()), _mpx3gui, SLOT(generateFrame()));
  connect(_mpx3gui, SIGNAL(data_cleared()), this, SLOT(on_clear()));
  connect(_mpx3gui, SIGNAL(frame_added()), this, SLOT(on_frame_added()));
  connect(_mpx3gui, SIGNAL(hist_added()), this, SLOT(on_hist_added()));
  connect(_mpx3gui, SIGNAL(frames_reload()),this, SLOT(on_frame_updated()));
  connect(_mpx3gui, SIGNAL(availible_gradients_changed(QStringList)), this, SLOT(on_availible_gradients_changed(QStringList)));
  connect(ui->histPlot, SIGNAL(rangeChanged(QCPRange)),ui->glPlot, SLOT(setRange(QCPRange)));
  connect(ui->histPlot, SIGNAL(rangeChanged(QCPRange)), ui->gradientDisplay, SLOT(set_range(QCPRange)));
  connect(_mpx3gui, SIGNAL(active_frame_changed(int)), ui->glPlot, SLOT(setActive(int)));
  connect(_mpx3gui, SIGNAL(active_frame_changed(int)), ui->histPlot, SLOT(setActive(int)));
  connect(ui->binWidthSpinner, SIGNAL(valueChanged(int)), ui->histPlot, SLOT(rebinHistograms(int)));
  connect(ui->glPlot, SIGNAL(hovered_pixel_changed(QPoint)),this, SLOT(on_hover_changed(QPoint)));
  connect(ui->glPlot, SIGNAL(pixel_selected(QPoint,QPoint)), this, SLOT(on_pixel_selected(QPoint,QPoint)));
  connect(ui->layerSpinner, SIGNAL(valueChanged(int)), _mpx3gui, SLOT(set_active_frame(int)));
  connect(this, SIGNAL(change_hover_text(QString)), ui->mouseOverLabel, SLOT(setText(QString)));
}

void QCstmGLVisualization::on_clear(){
  ui->histPlot->clear();
  ui->layerSpinner->setMaximum(0);
}

void QCstmGLVisualization::on_availible_gradients_changed(QStringList gradients){
  ui->gradientSelector->clear();
  ui->gradientSelector->addItems(gradients);
}

void QCstmGLVisualization::on_frame_updated(){
  ui->glPlot->setData(_mpx3gui->getDataset()->getFrames());
  ui->histPlot->changeBinSize(ui->binWidthSpinner->value(), ui->layerSpinner->value());
}

void QCstmGLVisualization::on_hover_changed(QPoint pixel){
  emit(change_hover_text(QString("%1 @ (%2, %3)").arg(_mpx3gui->getPixelAt(pixel.x(), pixel.y(),ui->layerSpinner->value())).arg(pixel.x()).arg(pixel.y())));
}

void QCstmGLVisualization::on_hist_added(){
  ui->histPlot->addHistogram(_mpx3gui->getHist(-1), ui->binWidthSpinner->value());
  ui->layerSpinner->setMaximum(_mpx3gui->getFrameCount()-1);
}

void QCstmGLVisualization::on_frame_added(){
  ui->layerSpinner->setValue(_mpx3gui->getFrameCount()-1);
  ui->glPlot->setData(_mpx3gui->getDataset()->getFrames());
}

void QCstmGLVisualization::on_pixel_selected(QPoint pixel, QPoint position){
  QMenu contextMenu;
  QAction mask(QString("Mask pixel @ %1, %2").arg(pixel.x()).arg(pixel.y()), &contextMenu), unmask(QString("Unmask pixel @ %1, %2").arg(pixel.x()).arg(pixel.y()), &contextMenu);
  contextMenu.addAction(&mask);
  contextMenu.addAction(&unmask);
  QAction* selectedItem = contextMenu.exec(position);
  if(selectedItem == &mask)
      //_mpx3gui->getDataset()->addMask(pixel);
     _mpx3gui->getEqualization()->GetEqualizationResults()->maskPixel(pixel.y()*_mpx3gui->getX()+pixel.x());
  else if(selectedItem == &unmask)
      //_mpx3gui->getDataset()->removeMask(pixel);
      _mpx3gui->getEqualization()->GetEqualizationResults()->unmaskPixel(pixel.y()*_mpx3gui->getX()+pixel.x());
  //_mpx3gui->getDataset()->loadAdjustments();
  _mpx3gui->getEqualization()->SetAllAdjustmentBits( );//TODO: integrate into dataset
}
