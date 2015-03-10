#include "qcstmvisualization.h"
#include "ui_qcstmvisualization.h"

#include "SpidrController.h"
#include "SpidrDaq.h"

QCstmVisualization::QCstmVisualization(QWidget *parent) :
QWidget(parent),
ui(new Ui::QCstmVisualization)
{
	ui->setupUi(this);
	QList<int> defaultSizesMain = {2971215,1836312}; //The ratio of the splitters. Defaults to the golden ratio because "oh! fancy".

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

void QCstmVisualization::ConnectionStatusChanged() {

	SpidrController * spidrcontrol = _mpx3gui->GetModuleConnection()->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetModuleConnection()->GetSpidrDaq();

	// TODO
	// Configure the chip, provided that the Adj mask is loaded
	Configuration( false );

	// Start the trigger as configured
	spidrcontrol->startAutoTrigger();
	Sleep( 50 );

	// See if there is a frame available
	// I should get as many frames as triggers

	int * framedata;

	while ( spidrdaq->hasFrame() ) {

		int size_in_bytes = -1;
		framedata = spidrdaq->frameData(0, &size_in_bytes);
		_mpx3gui->addFrame(framedata, size_in_bytes);

		//ExtractScanInfo( data, size_in_bytes, i );

		spidrdaq->releaseFrame();
		Sleep( 10 ); // Allow time to get and decode the next frame, if any



		// Report to heatmap
		//_mpx3gui->

		//_heatmap->addData(data, 256, 256); // Add a new plot/frame.
		//_heatmap->setActive(frameId++); // Activate the last plot (the new one)

		//_heatmap->setData( data, 256, 256 );

		// Last scan boundaries

	}

}


void QCstmVisualization::Configuration(bool reset) {

	SpidrController * spidrcontrol = _mpx3gui->GetModuleConnection()->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetModuleConnection()->GetSpidrDaq();

	int deviceIndex = 2;
	int nTriggers = 1;

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
	spidrcontrol->setGainMode( deviceIndex, 3 );

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
	int trig_freq_hz   = 100;   // One trigger every 10ms
	int nr_of_triggers = nTriggers;    // This is the number of shutter open i get
	//int trig_pulse_count;
	spidrcontrol->setShutterTriggerConfig( trig_mode, trig_length_us,
			trig_freq_hz, nr_of_triggers );

}

void QCstmVisualization::on_openfileButton_clicked()
{/*
  QImage image;
  QStringList files = QFileDialog::getOpenFileNames(this, tr("Open File"),QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), tr("Images (*.png *.xpm *.jpg *.gif *.png)"));
  if(files.isEmpty())
          return;
  ui->layerCombobox->clear();
  ui->layerCombobox->addItems(files);
  ui->histogramPlot->clear();
  delete[] nx; delete[] ny;
  for(unsigned u = 0; u < nData; u++){
          delete[] data[u];
          delete hists[u];
  }
  delete[] data;
  delete[] hists;
  ui->heatmap->clear();
  nData = files.length();
  data = new int*[nData];
  hists = new histogram*[nData];
  nx = new unsigned[nData]; ny = new unsigned[nData];
  for(unsigned i = 0; i < nData; i++){
          image.load(files[i]);
          if (image.isNull()) {
                  QMessageBox::information(this, tr("Image Viewer"), tr("Cannot load %1.").arg(files[i]));
                  return;
          }
          nx[i] = image.width(); ny[i] = image.height();
          data[i] = new int[nx[i]*ny[i]];
          for(unsigned u = 0; u < ny[i]; u++)
                  for(unsigned w = 0; w < nx[i];w++){
                          QRgb pixel = image.pixel(w,u);
                          data[i][u*nx[i]+w] = qGray(pixel);
                  }
          hists[i] = new histogram(data[i],nx[i]*ny[i], 1);
          ui->histogramPlot->addHistogram(hists[i]);
          ui->heatmap->addData(data[i], nx[i], ny[i]);
  }
  ui->histogramPlot->setActive(0);
  ui->heatmap->setActive(0);
  //ui->histogramPlot->rescaleAxes();
  ui->heatmap->rescaleAxes();*/
}
