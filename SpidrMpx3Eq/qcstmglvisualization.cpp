#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"
#include "qcstmequalization.h"
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
		_mpx3gui->addLayer(framedata);
		//_mpx3gui->getDataset()->setLayer(framedata,0);
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

	spidrcontrol->setColourMode( deviceIndex, _mpx3gui->getConfig()->getColourMode() ); // false 	// Fine Pitch
	spidrcontrol->setCsmSpm( deviceIndex, _mpx3gui->getConfig()->getCsmSpm() ); // 0 );				// Single Pixel mode

	// Particular for Equalization
	//spidrcontrol->setEqThreshH( deviceIndex, true );
	//spidrcontrol->setDiscCsmSpm( deviceIndex, 0 );		// In Eq mode using 0: Selects DiscL, 1: Selects DiscH
	//_spidrcontrol->setGainMode( 1 );

	// Gain ?!
	// 00: SHGM  0
	// 10: HGM   2
	// 01: LGM   1
	// 11: SLGM  3
	spidrcontrol->setGainMode( deviceIndex, _mpx3gui->getConfig()->getGainMode() ); // 2 );

	// Other OMR
	spidrdaq->setDecodeFrames(  _mpx3gui->getConfig()->getDecodeFrames() ); //  true );
	spidrcontrol->setPixelDepth( deviceIndex,  _mpx3gui->getConfig()->getPixelDepth() );
	spidrdaq->setPixelDepth( _mpx3gui->getConfig()->getPixelDepth() );
	spidrcontrol->setMaxPacketSize( _mpx3gui->getConfig()->getMaxPacketSize() );

	// Write OMR ... i shouldn't call this here
	//_spidrcontrol->writeOmr( 0 );

	// Trigger config
	int trig_mode      = _mpx3gui->getConfig()->getTriggerMode();     // Auto-trigger mode = 4
	int trig_length_us = _mpx3gui->getConfig()->getTriggerLength();  // This time shouldn't be longer than the period defined by trig_freq_hz
	int trig_freq_hz   = (int) ( 1. / (2.*((double)trig_length_us/1000000.)) );   // Make the period double the trig_len
	cout << "[INFO] Configured freq is " << trig_freq_hz << "Hz" << endl;
	int nr_of_triggers = _mpx3gui->getConfig()->getNTriggers();    // This is the number of shutter open i get
	//int trig_pulse_count;
	spidrcontrol->setShutterTriggerConfig( trig_mode, trig_length_us,
			trig_freq_hz, nr_of_triggers );

}

void QCstmGLVisualization::setGradient(int index){
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
  connect(ui->histPlot, SIGNAL(rangeChanged(QCPRange)), ui->glPlot, SLOT(set_range(QCPRange)));
  connect(ui->layerSpinner, SIGNAL(valueChanged(int)), ui->glPlot->getPlot(), SLOT(setActive(int)));
  connect(ui->layerSpinner, SIGNAL(valueChanged(int)), ui->histPlot, SLOT(setActive(int)));
  connect(ui->layerSpinner, SIGNAL(valueChanged(int)), _mpx3gui, SLOT(set_active_frame(int)));
  connect(ui->layerSpinner, SIGNAL(valueChanged(int)), this, SLOT(on_active_frame_changed(int)));
  connect(ui->binWidthSpinner, SIGNAL(valueChanged(int)), ui->histPlot, SLOT(rebinHistograms(int)));
  connect(ui->glPlot->getPlot(), SIGNAL(hovered_pixel_changed(QPoint)),this, SLOT(on_hover_changed(QPoint)));
  connect(ui->glPlot->getPlot(), SIGNAL(pixel_selected(QPoint,QPoint)), this, SLOT(on_pixel_selected(QPoint,QPoint)));

  connect(this, SIGNAL(change_hover_text(QString)), ui->mouseOverLabel, SLOT(setText(QString)));
  //connect(ui->fullRangeRadio, SIGNAL(pressed()), ui->histPlot, SLOT(set_scale_full()));
  connect(ui->histPlot, SIGNAL(new_range_dragged(QCPRange)), this, SLOT(on_new_range_dragged(QCPRange)));
}

void QCstmGLVisualization::on_new_range_dragged(QCPRange newRange){
  ui->lowerManualSpin->setValue(newRange.lower);
  ui->upperManualSpin->setValue(newRange.upper);
  if(ui->manualRangeRadio->isChecked())
    this->on_manualRangeRadio_toggled(true);
  else
    ui->manualRangeRadio->setChecked(true);

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
  ui->glPlot->getPlot()->readData(*_mpx3gui->getDataset());
  ui->histPlot->changeBinSize(ui->binWidthSpinner->value(), ui->layerSpinner->value());
  if(ui->percentileRangeRadio->isChecked())
    on_percentileRangeRadio_toggled(true);
  else if(ui->fullRangeRadio->isChecked())
    on_fullRangeRadio_toggled(true);
}

void QCstmGLVisualization::on_hover_changed(QPoint pixel){
  emit(change_hover_text(QString("%1 @ (%2, %3)").arg(_mpx3gui->getPixelAt(pixel.x(), pixel.y(),ui->layerSpinner->value())).arg(pixel.x()).arg(pixel.y())));
}

void QCstmGLVisualization::on_hist_added(){
  ui->histPlot->addHistogram(_mpx3gui->getHist(-1), ui->binWidthSpinner->value());
  ui->layerSpinner->setMaximum(_mpx3gui->getFrameCount()-1);
}

void QCstmGLVisualization::on_frame_added(){
  ui->glPlot->getPlot()->readData(*_mpx3gui->getDataset());
  //ui->glPlot->getPlot()->setData(_mpx3gui->getDataset()->getFrames());
  on_active_frame_changed(_mpx3gui->getFrameCount()-1);
}

void QCstmGLVisualization::on_active_frame_changed(int active){
  ui->layerSpinner->setValue(active);
  if(ui->percentileRangeRadio->isChecked())
    on_percentileRangeRadio_toggled(true);
  else if(ui->fullRangeRadio->isChecked())
    on_fullRangeRadio_toggled(true);
}

void QCstmGLVisualization::on_pixel_selected(QPoint pixel, QPoint position){
  QMenu contextMenu;
  QAction mask(QString("Mask pixel @ %1, %2").arg(pixel.x()).arg(pixel.y()), &contextMenu), unmask(QString("Unmask pixel @ %1, %2").arg(pixel.x()).arg(pixel.y()), &contextMenu);
  contextMenu.addAction(&mask);
  contextMenu.addAction(&unmask);
  QAction* selectedItem = contextMenu.exec(position);
  if(!_mpx3gui->getConfig()->isConnected())
    return;
  if(selectedItem == &mask)
      //_mpx3gui->getDataset()->addMask(pixel);
     _mpx3gui->getEqualization()->GetEqualizationResults()->maskPixel(pixel.y()*_mpx3gui->getX()+pixel.x());
  else if(selectedItem == &unmask)
      //_mpx3gui->getDataset()->removeMask(pixel);
      _mpx3gui->getEqualization()->GetEqualizationResults()->unmaskPixel(pixel.y()*_mpx3gui->getX()+pixel.x());
  //_mpx3gui->getDataset()->loadAdjustments();
  _mpx3gui->getEqualization()->SetAllAdjustmentBits( );//TODO: integrate into dataset
}

void QCstmGLVisualization::on_percentileRangeRadio_toggled(bool checked)
{
    if(checked){
      int nPoints = ui->glPlot->getPlot()->getNx()*ui->glPlot->getPlot()->getNy()/ui->binWidthSpinner->value();
      ui->histPlot->set_scale_percentile(ui->lowerPercentileSpin->value()*nPoints, ui->upperPercentileSpin->value()*nPoints);
      }
}

void QCstmGLVisualization::on_lowerPercentileSpin_editingFinished()
{
    if(ui->lowerPercentileSpin->value() > ui->upperPercentileSpin->value())
      ui->upperPercentileSpin->setValue(ui->lowerPercentileSpin->value());
    if(ui->percentileRangeRadio->isChecked())
      on_percentileRangeRadio_toggled(ui->percentileRangeRadio->isChecked());
}

void QCstmGLVisualization::on_upperPercentileSpin_editingFinished()
{
    if(ui->upperPercentileSpin->value() < ui->lowerPercentileSpin->value())
      ui->lowerPercentileSpin->setValue(ui->upperPercentileSpin->value());
    if(ui->percentileRangeRadio->isChecked())
      on_percentileRangeRadio_toggled(ui->percentileRangeRadio->isChecked());
}

void QCstmGLVisualization::on_lowerManualSpin_editingFinished()
{
    if(ui->upperManualSpin->value() < ui->lowerManualSpin->value())
      ui->upperManualSpin->setValue(ui->lowerManualSpin->value());
    if(ui->manualRangeRadio->isChecked())
      on_manualRangeRadio_toggled(ui->manualRangeRadio->isChecked());

}

void QCstmGLVisualization::on_upperManualSpin_editingFinished()
{
  if(ui->lowerManualSpin->value() > ui->upperManualSpin->value())
    ui->lowerManualSpin->setValue(ui->upperManualSpin->value());
  if(ui->manualRangeRadio->isChecked())
    on_manualRangeRadio_toggled(ui->manualRangeRadio->isChecked());
}

void QCstmGLVisualization::on_manualRangeRadio_toggled(bool checked)
{
      if(checked)
        ui->histPlot->changeRange(QCPRange(ui->lowerManualSpin->value(), ui->upperManualSpin->value()));
}

void QCstmGLVisualization::on_fullRangeRadio_toggled(bool checked)
{
    if(checked)
      ui->histPlot->set_scale_full();
}
