#include "qcstmvisualization.h"
#include "ui_qcstmvisualization.h"

#include "SpidrController.h"
#include "SpidrDaq.h"
#include "mpx3equalization.h"

QCstmVisualization::QCstmVisualization(QWidget *parent) :
QWidget(parent),
ui(new Ui::QCstmVisualization)
{
	ui->setupUi(this);
	QList<int> defaultSizesMain; //The ratio of the splitters. Defaults to the golden ratio because "oh! fancy".
	defaultSizesMain.append(2971215);
	defaultSizesMain.append(1836312);
	for(int i = 0; i < ui->mainSplitter->count();i++){
		QWidget *child = ui->mainSplitter->widget(i);
		child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		child->setMinimumSize(1,1);
	}
	ui->mainSplitter->setSizes(defaultSizesMain);


}

QCstmVisualization::~QCstmVisualization()
{
	delete ui;
}

void QCstmVisualization::SignalsAndSlots(){

	connect( ui->startDataTaking, SIGNAL( clicked() ), this, SLOT( StartDataTaking() ) );

}
void QCstmVisualization::StartDataTaking(){

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	// Start the trigger as configured
	spidrcontrol->startAutoTrigger();
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

void QCstmVisualization::ConnectionStatusChanged(bool connected) {

	ui->startDataTaking->setEnabled(connected); //Enable or disable the button depending on the connection status.

	// TODO
	// Configure the chip, provided that the Adj mask is loaded
	Configuration( false );

}

void QCstmVisualization::on_data_cleared(){
  ui->layerSpinner->setMaximum(0);
  ui->layerSpinner->setValue(0);
  ui->heatmap->clear();
  ui->histogramPlot->clear();
}

void QCstmVisualization::Configuration(bool reset) {//TODO: should be part of parent?

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	int deviceIndex = 2;
	int nTriggers = 100;

	// Reset pixel configuration
	if ( reset ) spidrcontrol->resetPixelConfig();

	// All adjustment bits to zero
	//SetAllAdjustmentBits(0x0, 0x0);

	// OMR
	//_spidrcontrol->setPolarity( true );		// Holes collection
	//_spidrcontrol->setDiscCsmSpm( 0 );		// DiscL used
	//_spidrcontrol->setInternalTestPulse( true ); // Internal tests pulse
	spidrcontrol->setPixelDepth( deviceIndex, 12 );

	spidrcontrol->setColourMode( deviceIndex, false ); 	// Fine Pitch
	spidrcontrol->setCsmSpm( deviceIndex, 0 );			// Single Pixel mode
	spidrcontrol->setEqThreshH( deviceIndex, true );
	spidrcontrol->setDiscCsmSpm( deviceIndex, 0 );		// In Eq mode using 0: Selects DiscL, 1: Selects DiscH
	//_spidrcontrol->setGainMode( 1 );

	// Gain ?!
	// 00: SHGM  0
	// 10: HGM   2
	// 01: LGM   1
	// 11: SLGM  3
	spidrcontrol->setGainMode( deviceIndex, 1 );

	// Other OMR
	spidrdaq->setDecodeFrames( true );
	spidrcontrol->setPixelDepth( deviceIndex, 12 );
	spidrdaq->setPixelDepth( 12 );
	spidrcontrol->setMaxPacketSize( 1024 );

	// Write OMR ... i shouldn't call this here
	//_spidrcontrol->writeOmr( 0 );

	// Trigger config
	int trig_mode      = 4;     // Auto-trigger mode
	int trig_length_us = 5000;  // This time shouldn't be longer than the period defined by trig_freq_hz
	int trig_freq_hz   = (int) ( 1. / (2.*((double)trig_length_us/1000000.)) );   // Make the period double the trig_len
	cout << "Configured freq is " << trig_freq_hz << "Hz" << endl;
	int nr_of_triggers = nTriggers;    // This is the number of shutter open i get
	//int trig_pulse_count;
	spidrcontrol->setShutterTriggerConfig( trig_mode, trig_length_us,
			trig_freq_hz, nr_of_triggers );

}

void QCstmVisualization::on_openfileButton_clicked()
{
  _mpx3gui->generateFrame();
}

void QCstmVisualization::on_frame_added (){
  ui->histogramPlot->addHistogram(_mpx3gui->getHist(-1), ui->spinBox->value());
  //ui->histogramPlot->setActive(-1);
  ui->heatmap->addData(_mpx3gui->getFrame(-1), _mpx3gui->getX(), _mpx3gui->getY());
  ui->layerSpinner->setMaximum(_mpx3gui->getFrameCount()-1);
  ui->layerSpinner->setValue(_mpx3gui->getFrameCount()-1);
}

void QCstmVisualization::on_frame_changed(){
  ui->histogramPlot->swapHistogram(_mpx3gui->getHist(-1), ui->spinBox->value());
  ui->heatmap->setData(_mpx3gui->getFrame(-1), _mpx3gui->getX(), _mpx3gui->getY());
  on_active_frame_changed(ui->layerSpinner->value());
}

void QCstmVisualization::on_active_frame_changed(int active){
  int* frame = _mpx3gui->getFrame(active);
  int count = 0, sum = 0;
  for(int i = 0; i < _mpx3gui->getX()*_mpx3gui->getY();i++){//TODO: absorb this into frames themselves.
      if(frame[i]){
          sum += frame[i];
          count++;
        }
   }
  ui->statistics->setText(QString("Active Pixels: %1\t Total Charge: %2").arg(count).arg(sum));
}

void QCstmVisualization::on_availible_gradients_changed(QStringList gradients){
  ui->heatmapCombobox->addItems(gradients);
}

void QCstmVisualization::on_gradient_added(QString name){
  ui->heatmapCombobox->addItem(name);
}

void QCstmVisualization::set_gradient(QString name){
  ui->heatmapCombobox->setCurrentText(name);
  on_heatmapCombobox_activated(name);
}

void QCstmVisualization::on_heatmapCombobox_activated(const QString &arg1)
{
    ui->heatmap->setHeatmap(_mpx3gui->getGradient(arg1));
}

void QCstmVisualization::on_pixel_selected(QPoint pixel, QPoint position){
  QMenu contextMenu;
  QAction mask(QString("Mask pixel @ %1, %2").arg(pixel.x()).arg(pixel.y()), &contextMenu), unmask(QString("Unmask pixel @ %1, %2").arg(pixel.x()).arg(pixel.y()), &contextMenu);
  contextMenu.addAction(&mask);
  contextMenu.addAction(&unmask);
  QAction* selectedItem = contextMenu.exec(position);
  if(selectedItem == &mask)
      _mpx3gui->getEqualization()->GetEqualizationResults()->maskPixel(pixel.y()*_mpx3gui->getX()+pixel.x());
  else if(selectedItem == &unmask)
      _mpx3gui->getEqualization()->GetEqualizationResults()->unmaskPixel(pixel.y()*_mpx3gui->getX()+pixel.x());
  _mpx3gui->getEqualization()->SetAllAdjustmentBits( );
}
