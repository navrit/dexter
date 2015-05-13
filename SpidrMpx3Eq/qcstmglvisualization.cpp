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
  SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
  SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

  cout << "Acquiring ... ";

  int nChips = _mpx3gui->getFrameCount();

  // Start the trigger as configured
  spidrcontrol->startAutoTrigger();
  Sleep( 50 );

  // See if there is a frame available
  int * framedata;
  while ( spidrdaq->hasFrame() ) {
      int size_in_bytes = -1;
      QVector<int> activeDevices = _mpx3gui->getConfig()->getActiveDevices();

      for(int i = 0 ; i < activeDevices.size() ; i++) {

          framedata = spidrdaq->frameData(i, &size_in_bytes);

          if ( size_in_bytes == 0 ) continue; // this may happen

          // In color mode the separation of thresholds needs to be done
          if( _mpx3gui->getConfig()->getColourMode() ) {

              int size = size_in_bytes / 4;
              int sizeReduced = size / 4;    // 4 thresholds per 110um pixel

              QVector<int> *th0 = new QVector<int>(sizeReduced);
              QVector<int> *th2 = new QVector<int>(sizeReduced);
              QVector<int> *th4 = new QVector<int>(sizeReduced);
              QVector<int> *th6 = new QVector<int>(sizeReduced);

              SeparateThresholds(framedata, size, th0, th2, th4, th6, sizeReduced);

              _mpx3gui->addFrame(th0->data(), i, 0);
              delete th0;

              _mpx3gui->addFrame(th2->data(), i, 2);
              delete th2;

              _mpx3gui->addFrame(th4->data(), i, 4);
              delete th4;

              _mpx3gui->addFrame(th6->data(), i, 6);
              delete th6;

            } else {
              _mpx3gui->addFrame(framedata, i, 0);
            }

        }
       if( _mpx3gui->getConfig()->getColourMode() )
         on_reload_all_layers();
       else
         on_reload_layer(0);
      //_mpx3gui->getDataset()->setLayer(framedata,0);
      spidrdaq->releaseFrame();
      Sleep( 10 ); // Allow time to get and decode the next frame, if any
    }

  cout << "done." << endl;

}

void QCstmGLVisualization::SeparateThresholds(int * data, int size, QVector<int> * th0, QVector<int> * th2, QVector<int> * th4, QVector<int> * th6, int sizeReduced) {

  // Layout of 110um pixel
  //  -------------
  //  | P3  |  P1 |
  //	-------------
  //  | P4  |  P2 |
  //  -------------
  //  Where:
  //  	P1 --> TH0, TH1
  //		P2 --> TH2, TH3
  //		P3 --> TH4, TH5
  //		P4 --> TH6, TH7

  int indx = 0, indxRed = 0, redi = 0, redj = 0;

  for (int j = 0 ; j < __matrix_size_y ; j++) {

      redi = 0;
      for (int i = 0 ; i < __matrix_size_x  ; i++) {

          indx = XYtoX( i, j, __matrix_size_x);
          indxRed = XYtoX( redi, redj, __matrix_size_x / 2); // This index should go up to 128*128

          //if(indxRed > 16380 ) cout << "indx " << indx << ", indxRed = " << indxRed << endl;

          if( i % 2 == 0 && j % 2 == 0) {
              (*th6)[indxRed] = data[indx]; // P4
            }
          if( i % 2 == 0 && j % 2 == 1) {
              (*th4)[indxRed] = data[indx]; // P3
            }
          if( i % 2 == 1 && j % 2 == 0) {
              (*th2)[indxRed] = data[indx]; // P2
            }
          if( i % 2 == 1 && j % 2 == 1) {
              (*th0)[indxRed] = data[indx]; // P1
            }

          if (i % 2 == 1) redi++;

        }

      if (j % 2 == 1) redj++;

    }

}

pair<int, int> QCstmGLVisualization::XtoXY(int X, int dimX){
  return make_pair(X % dimX, X/dimX);
}


void QCstmGLVisualization::ConnectionStatusChanged() {

  ui->startButton->setEnabled(true); //Enable or disable the button depending on the connection status.

  // TODO
  // Configure the chip, provided that the Adj mask is loaded
  //Configuration( false );

}


void QCstmGLVisualization::Configuration(bool reset, int deviceIndex) {//TODO: should be part of parent?

  SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
  SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

  int nTriggers = _mpx3gui->getConfig()->getNTriggers();

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
  connect(_mpx3gui, SIGNAL(frame_added(int)), this, SLOT(on_frame_added(int)));//TODO specify which layer.
  //connect(_mpx3gui, SIGNAL(hist_added(int)), this, SLOT(on_hist_added(int)));
  //connect(_mpx3gui, SIGNAL(hist_changed(int)),this, SLOT(on_hist_changed(int)));
  connect(_mpx3gui, SIGNAL(reload_layer(int)), this, SLOT( on_reload_layer(int)));
  connect(_mpx3gui, SIGNAL(reload_all_layers()), this, SLOT(on_reload_all_layers()));
  //connect(_mpx3gui, SIGNAL(frames_reload()),this, SLOT(on_frame_updated()));
  connect(_mpx3gui, SIGNAL(availible_gradients_changed(QStringList)), this, SLOT(on_availible_gradients_changed(QStringList)));
  connect(ui->histPlot, SIGNAL(rangeChanged(QCPRange)), this, SLOT(on_range_changed(QCPRange)));
  //connect(ui->histPlot, SIGNAL(rangeChanged(QCPRange)), this, SLOT(on_new_range_dragged(QCPRange)));
  // connect(ui->layerSelector, SIGNAL(activated(QString)), ui->glPlot->getPlot(), SLOT()

  connect(ui->binWidthSpinner, SIGNAL(valueChanged(int)), ui->histPlot, SLOT(changeBinSize(int)));
  connect(ui->glPlot->getPlot(), SIGNAL(hovered_pixel_changed(QPoint)),this, SLOT(on_hover_changed(QPoint)));
  connect(ui->glPlot->getPlot(), SIGNAL(pixel_selected(QPoint,QPoint)), this, SLOT(on_pixel_selected(QPoint,QPoint)));

  connect(this, SIGNAL(change_hover_text(QString)), ui->mouseOverLabel, SLOT(setText(QString)));
  //connect(ui->fullRangeRadio, SIGNAL(pressed()), ui->histPlot, SLOT(set_scale_full()));
  connect(ui->histPlot, SIGNAL(new_range_dragged(QCPRange)), this, SLOT(on_new_range_dragged(QCPRange)));
}

void QCstmGLVisualization::on_range_changed(QCPRange newRange){
  ui->lowerManualSpin->setValue(newRange.lower);
  ui->upperManualSpin->setValue(newRange.upper);
  ui->glPlot->set_range(newRange);
}

void QCstmGLVisualization::on_new_range_dragged(QCPRange newRange){
  on_range_changed(newRange);
  if(!ui->manualRangeRadio->isChecked())
    ui->manualRangeRadio->setChecked(true);
}

void QCstmGLVisualization::on_clear(){
  layerNames.clear();
  ui->layerSelector->clear();
  ui->histPlot->clear();
  ui->layerSelector->clear();
}

void QCstmGLVisualization::on_availible_gradients_changed(QStringList gradients){
  ui->gradientSelector->clear();
  ui->gradientSelector->addItems(gradients);
}

void QCstmGLVisualization::on_hover_changed(QPoint pixel){
  emit(change_hover_text(QString("%1 @ (%2, %3)").arg(_mpx3gui->getPixelAt(pixel.x(), pixel.y(),getActiveThreshold())).arg(pixel.x()).arg(pixel.y())));
}

void QCstmGLVisualization::on_reload_layer(int threshold){
  int layer = _mpx3gui->getDataset()->thresholdToIndex(threshold);
  ui->glPlot->getPlot()->readData(*_mpx3gui->getDataset()); //TODO: only read specific layer.
  ui->histPlot->setHistogram(threshold, _mpx3gui->getDataset()->getLayer(threshold), _mpx3gui->getDataset()->getPixelsPerLayer());
  setThreshold(threshold);
  on_active_frame_changed();
}

void QCstmGLVisualization::on_reload_all_layers(){
  ui->glPlot->getPlot()->readData(*_mpx3gui->getDataset()); //TODO: only read specific layer.
  QList<int> thresholds = _mpx3gui->getDataset()->getThresholds();
  for(int i = 0; i < thresholds.size(); i++){
      addThresholdToSelector(thresholds[i]);
      ui->histPlot->setHistogram(thresholds[i], _mpx3gui->getDataset()->getLayer(thresholds[i]), _mpx3gui->getDataset()->getPixelsPerLayer());
    }
  setThreshold(thresholds[0]);
  on_active_frame_changed();
}

void QCstmGLVisualization::addThresholdToSelector(int threshold){
  QString label = QString("Threshold %1").arg(threshold);
  if(!layerNames.contains(threshold)){
      layerNames[threshold] =label ;
      ui->layerSelector->clear();
      ui->layerSelector->addItems(QStringList(layerNames.values()));
    }
}

void QCstmGLVisualization::setThreshold(int threshold){
  addThresholdToSelector(threshold);
  ui->layerSelector->setCurrentIndex(_mpx3gui->getDataset()->thresholdToIndex(threshold));
}

int QCstmGLVisualization::getActiveThreshold(){
  QStringList list = ui->layerSelector->currentText().split(" ");
  if(list.size() < 2)
    return -1;
  return list[1].toInt();
}

void QCstmGLVisualization::on_active_frame_changed(){
  //ui->layerSelector->addItem(QString("%1").arg(threshold));
  int layer = _mpx3gui->getDataset()->thresholdToIndex(this->getActiveThreshold());
  ui->glPlot->getPlot()->setActive(layer);
  ui->histPlot->setActive(layer);
  ui->chargeLabel->setText(QString("Total Charge: %1").arg(ui->histPlot->getTotal(getActiveThreshold())));
  ui->countsLabel->setText(QString("Total Fired: %1").arg(_mpx3gui->getDataset()->getPixelsPerLayer() - ui->histPlot->getBin(getActiveThreshold(),0)));
  if(ui->percentileRangeRadio->isChecked())
    on_percentileRangeRadio_toggled(true);
  else if(ui->fullRangeRadio->isChecked())
    on_fullRangeRadio_toggled(true);
}

void QCstmGLVisualization::on_pixel_selected(QPoint pixel, QPoint position){
  if(!_mpx3gui->getConfig()->isConnected())
    return;
  int frameIndex = _mpx3gui->getDataset()->getContainingFrame(pixel);
  if(frameIndex == -1)
    return;
  QPoint naturalCoords = _mpx3gui->getDataset()->getNaturalCoordinates(pixel, frameIndex);
  int naturalFlatCoord = naturalCoords.y()*_mpx3gui->getDataset()->x()+naturalCoords.x();
  int deviceID = _mpx3gui->getConfig()->getActiveDevices()[frameIndex];
  QMenu contextMenu;
  QAction mask(QString("Mask pixel @ %1, %2").arg(pixel.x()).arg(pixel.y()), &contextMenu), unmask(QString("Unmask pixel @ %1, %2").arg(pixel.x()).arg(pixel.y()), &contextMenu);
  contextMenu.addAction(&mask);
  contextMenu.addAction(&unmask);
  QAction* selectedItem = contextMenu.exec(position);
  if(!_mpx3gui->getConfig()->isConnected())
    return;
  if(selectedItem == &mask){
       _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->maskPixel(naturalFlatCoord);
   }
  else if(selectedItem == &unmask){
        _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->unmaskPixel(naturalFlatCoord);
    }
  _mpx3gui->getEqualization()->SetAllAdjustmentBits( _mpx3gui->getConfig()->getController(), deviceID);
}

void QCstmGLVisualization::on_percentileRangeRadio_toggled(bool checked)
{
  if(checked){
      ui->histPlot->set_scale_percentile(getActiveThreshold(),ui->lowerPercentileSpin->value(), ui->upperPercentileSpin->value());
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
    ui->histPlot->set_scale_full(getActiveThreshold());
}

void QCstmGLVisualization::on_outOfBoundsCheckbox_toggled(bool checked)
{
  ui->glPlot->getPlot()->setAlphaBlending(checked);
}

void QCstmGLVisualization::on_layerSelector_activated(const QString &arg1)
{
  QStringList split = arg1.split(' ');
  int threshold = split.last().toInt();
  int layer = _mpx3gui->getDataset()->thresholdToIndex(threshold);
  ui->glPlot->getPlot()->setActive(layer);
  ui->histPlot->setActive(layer);
  //_mpx3gui->set_active_frame(threshold);
  this->on_active_frame_changed();
}
