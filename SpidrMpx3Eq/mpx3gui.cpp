/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#include "mpx3gui.h"
#include "qcstmequalization.h"
#include "qcstmglvisualization.h"
#include "ui_mpx3gui.h"

#include "qcustomplot.h"
#include "mpx3eq_common.h"
//#include "DACs.h"
#include "mpx3defs.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "barchart.h"
#include "ThlScan.h"
#include "gradient.h"

#include <QMessageBox>

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

Mpx3GUI::Mpx3GUI(QApplication * coreApp, QWidget * parent) :	QMainWindow(parent), _coreApp(coreApp), _ui(new Ui::Mpx3GUI)
{
  // Instantiate everything in the UI
  _ui->setupUi(this);
  config = new Mpx3Config;
  config->SetMpx3GUI( this );

  // FIXME
  workingSet = new Dataset(8,8, 4);

  //workingSet->setFramesPerGroup(1,1)

  workingSet->setOrientation(0, _MPX3RX_ORIENTATION[0]);
  workingSet->setOrientation(1, _MPX3RX_ORIENTATION[1]);
  workingSet->setOrientation(2, _MPX3RX_ORIENTATION[2]);
  workingSet->setOrientation(3,_MPX3RX_ORIENTATION[3]);
  workingSet->setLayout(0,  _MPX3RX_LAYOUT[0] );
  workingSet->setLayout(1,  _MPX3RX_LAYOUT[1] );
  workingSet->setLayout(2,  _MPX3RX_LAYOUT[2]);
  workingSet->setLayout(3,  _MPX3RX_LAYOUT[3]);

  gradients = Gradient::fromJsonFile("./config/heatmaps.json");
  QStringList gradientNames;
  for(int i = 0; i < gradients.length();i++)
    gradientNames.append(gradients[i]->getName());

  // Prepare DACs panel
  //_dacs = new DACs(_coreApp, _ui);
  //_dacs->SetMpx3GUI( this );

  _ui->DACsWidget->SetMpx3GUI( this );

  // Prepare Equalization
  //_equalization = new Mpx3Equalization(_coreApp, _ui);
  //_equalization->SetMpx3GUI( this );
  _ui->equalizationWidget->SetMpx3GUI( this );


  // Prepare Visualization
  _ui->visualizationGL->SetMpx3GUI(this);
  //_ui->CTTab->SetMpx3GUI(this);
  emit availible_gradients_changed(gradientNames);

  // Prepare THL Calibration
  _ui->ThresholdTab->SetMpx3GUI(this);

  //Config & monitoring
  _ui->CnMWidget->SetMpx3GUI(this);
  // Read the configuration
  QString configFile = "./config/mpx3.json";
  if ( ! config->fromJsonFile( configFile ) ) {
      QMessageBox::critical(this, "Loading configuration error",
                            QString("Couldn't load the configuration file: %1").arg(configFile));
    }
  // Signals and slots for this part
  SetupSignalsAndSlots();
  //emit frame_added();
}

Mpx3GUI::~Mpx3GUI()
{
  delete config;
  delete workingSet;
  delete _ui;
}

void Mpx3GUI::addLayer(int *data){
  return addLayer(data, -1);
}

/*void Mpx3GUI::updateHistogram(int layer){
  if(mode == 1){
      if(workingSet->thresholdToIndex(layer) >= hists.size()){
          hists.append(new histogram(workingSet->getLayer(layer),workingSet->getFrameCount()*workingSet->x()*workingSet->y(),  1));
          emit hist_added(layer);
        }
      else{
          histogram *old = hists[workingSet->thresholdToIndex(layer)];//TODO: do this better
          delete old;
          old = new histogram(workingSet->getLayer(layer),workingSet->getFrameCount()*workingSet->x()*workingSet->y(),  1);
          emit hist_changed(layer);
        }
    }
  else{
      hists.append(new histogram(workingSet->getLayer(layer),workingSet->getFrameCount()*workingSet->x()*workingSet->y(),  1));
      emit hist_added(layer);
    }
}*/

void Mpx3GUI::addFrame(int *frame, int index, int layer){
  if(mode == 1){
      workingSet->sumFrame(frame,index, layer);
    }
  else{
      workingSet->setFrame(frame, index, layer);
    }
}

void Mpx3GUI::addLayer(int *data, int layer){
  if(mode == 1){
      workingSet->addLayer(data, layer);
    }
  else{
      workingSet->setLayer(data, layer);
    }
  //updateHistogram(layer);
  emit frame_added(layer);
}

void Mpx3GUI::reloadLayer(int layer){
  //updateHistogram(layer);
  emit frame_added(layer);
}

Gradient* Mpx3GUI::getGradient(int index){
  return gradients.at(index);
}

SpidrController * Mpx3GUI::GetSpidrController(){
  return config->getController();
}

void Mpx3GUI::LoadEqualization(){
  _ui->equalizationWidget->LoadEqualization();
}

void Mpx3GUI::SetupSignalsAndSlots(){

  std::cout << "[Mpx3GUI] Connecting signals and slots" << std::endl;
  connect( _ui->actionLoad_Equalization, SIGNAL(triggered()), this, SLOT( LoadEqualization() ) );
  connect( _ui->actionSave_DACs, SIGNAL(triggered()), this, SLOT( save_config()) );
  connect( _ui->actionLoad_DACs, SIGNAL(triggered()), this, SLOT( load_config()) );
  connect( _ui->actionConnect, SIGNAL(triggered()), this, SLOT( establish_connection() ) );

  connect(_ui->actionSumming, SIGNAL(triggered()), this, SLOT(set_mode_integral()));
  connect(_ui->actionDiscrete, SIGNAL(triggered()), this, SLOT(set_mode_normal()));

  connect(_ui->actionSave_data, SIGNAL(triggered()), this, SLOT(save_data()));
  connect(_ui->actionSave_Equalization, SIGNAL(triggered()), _ui->equalizationWidget, SLOT(SaveEqualization()));
  connect(_ui->actionOpen_data, SIGNAL(triggered()), this, SLOT(open_data()));
  connect(_ui->actionClear_data, SIGNAL(triggered()), this, SLOT(clear_data()));
  connect(_ui->actionClear_configuration, SIGNAL(triggered()), this, SLOT(clear_configuration()) );

  // Inform every module of changes in connection status
  connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->DACsWidget, SLOT( ConnectionStatusChanged() ) );
  connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->equalizationWidget, SLOT( ConnectionStatusChanged() ) );
  connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->visualizationGL, SLOT( ConnectionStatusChanged() ) );

}
Mpx3Config* Mpx3GUI::getConfig(){return config;}
void Mpx3GUI::on_openfileButton_clicked() {

}

void Mpx3GUI::set_summing(bool shouldSum){
  if(shouldSum)
    set_mode_integral();
  else
    set_mode_normal();
}

void Mpx3GUI::establish_connection() {
  cout << "Connecting ..." << endl;
  SpidrController * spidrcontrol = config->establishConnection();

  // Check if we are properly connected to the SPIDR module
  if ( spidrcontrol->isConnected() ) {

      cout << "Connected to SPIDR: " << spidrcontrol->ipAddressString() << "[" << config->getNDevicesPresent();
      if(config->getNDevicesPresent() > 1) cout << " chips found] ";
      else cout << " chip found] ";

      int ipaddr;
      // This call takes device number 0 'cause it is not really addressed to a chip in particular
      if( spidrcontrol->getIpAddrDest( 0, &ipaddr ) )
        cout << ", IP dest: "
             << ((ipaddr>>24) & 0xFF) << "."
             << ((ipaddr>>16) & 0xFF) << "."
             << ((ipaddr>> 8) & 0xFF) << "."
             << ((ipaddr>> 0) & 0xFF);
      cout <<  endl;
      //_ui->_statusLabel->setText("Connected");
      //_ui->_statusLabel->setStyleSheet("QLabel { background-color : blue; color : white; }");

    } else {
      cout << spidrcontrol->connectionStateString() << ": "
           << spidrcontrol->connectionErrString() << endl;
      //_ui->_statusLabel->setText("Connection failed.");
      //_ui->_statusLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
      QMessageBox::critical(this, "Connection error",
                            QString("Couldn't establish a connection to the Spidr controller at %1, %2").arg(QString::fromStdString(spidrcontrol->ipAddressString())).arg(QString::fromStdString(spidrcontrol->connectionErrString())));
      emit ConnectionStatusChanged(false);
      return; //No use in continuing if we can't connect.
    }

  // Get version numbers
  cout << "SpidrController class: "
       << spidrcontrol->versionToString( spidrcontrol->classVersion() ) << endl;
  int version;
  if( spidrcontrol->getFirmwVersion( &version ) )
    cout << "SPIDR firmware  : " << spidrcontrol->versionToString( version ) << endl;
  if( spidrcontrol->getSoftwVersion( &version ) )
    cout << "SPIDR software  : " << spidrcontrol->versionToString( version ) << endl;

  // SpidrDaq
  _spidrdaq = new SpidrDaq( spidrcontrol );
  cout << "SpidrDaq: ";

  for( int i=0; i<4; ++i ) cout << _spidrdaq->ipAddressString( i ) << " ";
  cout << endl;
  Sleep( 1000 );
  cout << _spidrdaq->errorString() << endl;

  // Here the chips can be configured
  getConfig()->SendConfiguration();

  // Emmit
  emit ConnectionStatusChanged(true);
  delete workingSet;

  workingSet = new Dataset(256, 256,config->getNActiveDevices());//TODO: get framesize from config, load offsets & orientation from config
  //emit something here?
  clear_data();
  QVector<int> activeDevices = config->getActiveDevices();
  for(int i = 0; i < activeDevices.size();i++){
      workingSet->setLayout(i, _MPX3RX_LAYOUT[activeDevices[i]]);
      workingSet->setOrientation(i, _MPX3RX_ORIENTATION[activeDevices[i]]);
    }
 /*for(int i = 0; i < workingSet->getLayerCount();i++)
    updateHistogram(i);*/
  //emit frames_reload();
}


void Mpx3GUI::generateFrame(){//TODO: put into Dataset
  //int thresholds[] = {0,1,2,3};
  QVector<int> data(workingSet->x()*workingSet->y()*workingSet->getFrameCount());
  for(int k = 0; k < workingSet->getFrameCount();k++){
      for(int t = 0; t < 4;t++){
          double fx = ((double)8*rand()/RAND_MAX)/(workingSet->x()), fy = (8*(double)rand()/RAND_MAX)/workingSet->y();
          for(int i = 0; i < workingSet->y(); i++)
            for(int j = 0; j < workingSet->x(); j++)
              //data[k*workingSet->x()*workingSet->y()+i*workingSet->x()+j] = (int)((1<<14)*sin(fx*j)*(cos(fy*i)));
              data[i*workingSet->x()+j] = (int)((1<<0)*(i*j));//sin(fx*j)*(cos(fy*i)));
          addFrame(data.data(), k, t);
        }
    }
  reloadLayer(0);reloadLayer(1);reloadLayer(2);reloadLayer(3);
}

int Mpx3GUI::getPixelAt(int x, int y, int layer){
  return workingSet->sample(x,y, layer);
  //if(layer >= data.length() || x >= nx || y >= ny)
  // return 0;
  //return data[layer][y*nx+x];
}

QPoint Mpx3GUI::getSize(){
  return workingSet->getSize();
}

void Mpx3GUI::getSize(int *x, int *y){
  QPoint dataSize = workingSet->getSize();
  *x = dataSize.x();
  *y = dataSize.y();
}
int Mpx3GUI::getX(){
  return workingSet->getSize().x();
}
int Mpx3GUI::getY(){
  return workingSet->getSize().y();
}

int Mpx3GUI::getFrameCount(){
  return workingSet->getFrameCount();
}


void Mpx3GUI::save_data(){//TODO: REIMPLEMENT
  QString filename = QFileDialog::getSaveFileName(this, tr("Save Data"), tr("."), tr("binary files (*.bin)"));
  QFile saveFile(filename);
  if (!saveFile.open(QIODevice::WriteOnly)) {
      string messg = "Couldn't open: ";
      messg += filename.toStdString();
      messg += "\nNo output written!";
      QMessageBox::warning ( this, tr("Error saving data"), tr( messg.c_str() ) );
      return;
    }
  saveFile.write(workingSet->toByteArray());
  saveFile.close();
  return;
}

void Mpx3GUI::save_config(){
  QString filename = QFileDialog::getSaveFileName(this, tr("Save config"), tr("."), tr("json files (*.json)"));
  config->toJsonFile(filename);
  return;
}

void Mpx3GUI::load_config(){
  QString filename = QFileDialog::getOpenFileName(this, tr("Save config"), tr("."), tr("json files (*.json)"));
  config->fromJsonFile(filename);

  // update the dacs
  _ui->DACsWidget->PopulateDACValues();

  return;
}

void Mpx3GUI::open_data(){
  QString filename = QFileDialog::getOpenFileName(this, tr("Read Data"), tr("."), tr("binary files (*.bin)"));
  QFile saveFile(filename);
  if (!saveFile.open(QIODevice::ReadOnly)) {
      string messg = "Couldn't open: ";
      messg += filename.toStdString();
      messg += "\nNo output written!";
      QMessageBox::warning ( this, tr("Error opening data"), tr( messg.c_str() ) );
      return;
    }
  clear_data();
  workingSet->fromByteArray(saveFile.readAll());
  saveFile.close();
  set_mode_normal();
  QList<int> thresholds = workingSet->getThresholds();
  for(int i = 0; i <thresholds.count();i++){
      reloadLayer(thresholds[i]);
    }
  return;
}

void Mpx3GUI::set_mode_integral(){
  if(mode != 1){
      mode = 1;
      emit summing_set(true);
    }
}

void Mpx3GUI::set_mode_normal(){
  if(0 != mode){
      mode = 0;
      emit summing_set(false);
    }
}

void Mpx3GUI::clear_configuration(){

  // Clear adjustement bits
  QMessageBox::StandardButton ans = QMessageBox::question(this, tr("Clear configuration"), tr("The adjustment matrix and the pixel mask will be cleared.  Continue ?") );
  if ( ans == QMessageBox::No ) return;

  bool noEqualization = false;
  if ( _ui->equalizationWidget ) {
      if ( _ui->equalizationWidget->GetEqualizationResults() ) {

          cout << "[INFO] clearing adjustment bits and mask." << endl;

          _ui->equalizationWidget->ClearAllAdjustmentBits();

        } else {
          noEqualization = true;
        }

    } else { noEqualization = true; }

  if ( noEqualization ) {
      cout << "[INFO] No equalization has been loaded. Nothing to clear." << endl;
    }

}

void Mpx3GUI::clear_data(){
  workingSet->clear();
  /*for(int i = 0; i < hists.size();i++)
    delete hists[i];
  hists.clear();*/
  emit(data_cleared());
}

QCstmEqualization * Mpx3GUI::getEqualization(){return _ui->equalizationWidget;}
QCstmGLVisualization * Mpx3GUI::getVisualization() { return _ui->visualizationGL; }
